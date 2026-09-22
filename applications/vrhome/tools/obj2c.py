#!/usr/bin/env python3
# embeds an OBJ file as a C array so the dash ships the controller meshes
# inside the .so - no assets plumbing, no runtime file IO
import sys

src, name, out = sys.argv[1], sys.argv[2], sys.argv[3]
data = open(src, "rb").read()
with open(out, "w") as f:
    f.write("static const unsigned char %s[] = {\n" % name)
    for i in range(0, len(data), 16):
        f.write(",".join(str(b) for b in data[i:i+16]) + ",\n")
    f.write("};\n")
    f.write("static const size_t %s_len = %d;\n" % (name, len(data)))
