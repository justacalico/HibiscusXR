#!/bin/bash
# Disassemble PVR::serviceStatusCallback around the crashing log call.
#
# pvrservice dies at __android_log_print <- serviceStatusCallback+472 with a null
# pointer (fault addr 0x0, x1 = 0). Either the format string or a %s argument is
# null. 8.1's bionic tolerated it; Q's does not. Find the call site, the format
# string it passes, and where the null operand comes from.
set -u
L=/mnt/f/PN2Lineage/notes/lib64/libpvrservice.so
PY=$HOME/.pn2venv/bin/python
LOG=/mnt/f/PN2Lineage/notes/174_status_cb.txt
exec >"$LOG" 2>&1

ls -l "$L" 2>/dev/null || { echo "libpvrservice.so not pulled"; exit 1; }
echo
echo "=== symbol ==="
nm -D --defined-only "$L" 2>/dev/null | grep -i serviceStatusCallback

"$PY" - "$L" <<'PY'
import struct, subprocess, sys
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN
path = sys.argv[1]
data = open(path,'rb').read()
e_phoff, = struct.unpack_from("<Q", data, 0x20)
e_phentsize, = struct.unpack_from("<H", data, 0x36)
e_phnum, = struct.unpack_from("<H", data, 0x38)
loads=[]
for i in range(e_phnum):
    o=e_phoff+i*e_phentsize
    t,=struct.unpack_from("<I",data,o)
    po,pv,_,pf=struct.unpack_from("<QQQQ",data,o+8)
    if t==1: loads.append((pv,po,pf))
def v2o(v):
    for pv,po,pf in loads:
        if pv<=v<pv+pf: return po+(v-pv)
    return None

sym=None
out=subprocess.run(["nm","-D","--defined-only",path],capture_output=True,text=True).stdout
for line in out.splitlines():
    p=line.split()
    if len(p)>=3 and "serviceStatusCallback" in p[2]:
        sym=int(p[0],16); name=p[2]; break
if sym is None:
    print("symbol not found"); sys.exit()
print(f"\n{name} @ {hex(sym)}   crash at +472 = {hex(sym+472)}\n")
md=Cs(CS_ARCH_ARM64, CS_MODE_LITTLE_ENDIAN); md.detail=False
off=v2o(sym)
code=data[off:off+700]
# collect adrp/add pairs so we can resolve string literals
pend={}
for ins in md.disasm(code, sym):
    d=ins.address-sym
    mark=""
    if d==472: mark="   <=========== CRASH RETURNS HERE (+472)"
    line=f"  +{d:<4} {ins.address:08x}  {ins.mnemonic:<8} {ins.op_str}{mark}"
    # resolve string operands
    if ins.mnemonic=="adrp":
        r,imm=[x.strip() for x in ins.op_str.split(",")]
        pend[r]=int(imm,16)
    elif ins.mnemonic=="add" and ins.op_str.count(",")==2:
        parts=[x.strip() for x in ins.op_str.split(",")]
        if parts[1] in pend and parts[2].startswith("#"):
            addr=pend[parts[1]]+int(parts[2][1:],16)
            o=v2o(addr)
            if o and o < len(data):
                end=data.find(b"\0",o,o+200)
                if end>o:
                    try:
                        s=data[o:end].decode('utf-8','replace')
                        if s.isprintable() and len(s)>1:
                            line+=f"        ; \"{s}\""
                    except Exception: pass
    print(line)
PY
echo
echo DONE
