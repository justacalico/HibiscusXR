// libc-free DRM modeset smoke test for the vmd guest (aarch64).
// Paints a Hibiscus pattern to the first connected output of
// /dev/dri/card0 so the QEMU window shows real guest-driven pixels.
//
// Build (host):
//   aarch64-linux-gnu-gcc -static -nostdlib -ffreestanding \
//       -I<kernel-headers>/include/uapi -I<kernel-headers>/include \
//       -o drmpaint drmpaint.c
// Run (guest, after adb connect localhost:15555):
//   adb push drmpaint /dev/drmpaint
//   adb shell "chmod 755 /dev/drmpaint && /dev/drmpaint"
//
// A visible pink band + cyan square means the virtio-gpu display
// pipeline works end to end.

typedef unsigned char u8; typedef unsigned int u32; typedef unsigned long u64;

typedef unsigned long size_t;

#define O_RDWR 02
#define O_CLOEXEC 02000000
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 1

#define __NR_openat 56
#define __NR_ioctl 29
#define __NR_mmap 222
#define __NR_write 64
#define __NR_exit 93
#define __NR_nanosleep 101

static inline long sys1(long n,long a){register long x8 asm("x8")=n;register long x0 asm("x0")=a;asm volatile("svc 0":"=r"(x0):"r"(x8),"r"(x0):"memory");return x0;}
static inline long sys2(long n,long a,long b){register long x8 asm("x8")=n;register long x0 asm("x0")=a,x1 asm("x1")=b;asm volatile("svc 0":"=r"(x0):"r"(x8),"r"(x0),"r"(x1):"memory");return x0;}
static inline long sys3(long n,long a,long b,long c){register long x8 asm("x8")=n;register long x0 asm("x0")=a,x1 asm("x1")=b,x2 asm("x2")=c;asm volatile("svc 0":"=r"(x0):"r"(x8),"r"(x0),"r"(x1),"r"(x2):"memory");return x0;}
static inline long sys6(long n,long a,long b,long c,long d,long e,long f){register long x8 asm("x8")=n;register long x0 asm("x0")=a,x1 asm("x1")=b,x2 asm("x2")=c,x3 asm("x3")=d,x4 asm("x4")=e,x5 asm("x5")=f;asm volatile("svc 0":"=r"(x0):"r"(x8),"r"(x0),"r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5):"memory");return x0;}

static int fd_open(const char*p,int fl){return (int)sys3(__NR_openat,-100,(long)p,fl);}
static long ioct(int fd,unsigned long req,void*arg){return sys3(__NR_ioctl,fd,req,(long)arg);}
static void* mm(void*a,size_t l,int pr,int fl,int f,long o){return (void*)sys6(__NR_mmap,(long)a,l,pr,fl,f,o);}
static void wr(const char*s,int n){sys3(__NR_write,1,(long)s,n);}
static int sl(const char*s){int n=0;while(s[n])n++;return n;}
static void ps(const char*s){wr(s,sl(s));}
static void pn(u64 v){char b[20];int i=19;b[i]=0;if(!v){ps("0");return;}while(v){b[--i]='0'+v%10;v/=10;}ps(b+i);}
static void snooze(int s){long ts[2]={s,0};sys2(__NR_nanosleep,(long)ts,0);}

#include <drm/drm_mode.h>
#include <drm/drm_fourcc.h>

int main(void){
    int fd=fd_open("/dev/dri/card0",O_RDWR|O_CLOEXEC);
    if(fd<0){ps("open card0 fail\n");return 1;}
    struct drm_mode_card_res res={0};
    ioct(fd,DRM_IOCTL_MODE_GETRESOURCES,&res);
    u32 conns[32],crtcs[32],encs[32];
    res.connector_id_ptr=(u64)conns;res.crtc_id_ptr=(u64)crtcs;res.encoder_id_ptr=(u64)encs;
    ioct(fd,DRM_IOCTL_MODE_GETRESOURCES,&res);
    ps("conns=");pn(res.count_connectors);ps(" crtcs=");pn(res.count_crtcs);ps("\n");
    u32 conn=0;struct drm_mode_modeinfo mode={0};
    for(int i=0;i<res.count_connectors;i++){
        struct drm_mode_get_connector c={0};c.connector_id=conns[i];
        ioct(fd,DRM_IOCTL_MODE_GETCONNECTOR,&c);
        struct drm_mode_modeinfo m[64];u32 en[16];
        c.modes_ptr=(u64)m;c.encoders_ptr=(u64)en;
        ioct(fd,DRM_IOCTL_MODE_GETCONNECTOR,&c);
        ps("conn ");pn(conns[i]);ps(" link=");pn(c.connection);ps(" modes=");pn(c.count_modes);ps("\n");
        if(c.connection==1&&c.count_modes){conn=conns[i];mode=m[0];break;}
    }
    if(!conn){ps("no connected connector\n");return 1;}
    u32 crtc=crtcs[0];
    ps("mode ");pn(mode.hdisplay);ps("x");pn(mode.vdisplay);ps("\n");

    struct drm_mode_create_dumb cd={0};
    cd.width=mode.hdisplay;cd.height=mode.vdisplay;cd.bpp=32;
    if(ioct(fd,DRM_IOCTL_MODE_CREATE_DUMB,&cd)){ps("dumb fail\n");return 1;}
    struct drm_mode_fb_cmd2 a={0};
    a.width=mode.hdisplay;a.height=mode.vdisplay;a.pixel_format=DRM_FORMAT_XRGB8888;
    a.handles[0]=cd.handle;a.pitches[0]=cd.pitch;
    if(ioct(fd,DRM_IOCTL_MODE_ADDFB2,&a)){ps("addfb fail\n");return 1;}
    struct drm_mode_map_dumb md={0};md.handle=cd.handle;
    if(ioct(fd,DRM_IOCTL_MODE_MAP_DUMB,&md)){ps("mapdumb fail\n");return 1;}
    u8*pix=mm(0,cd.size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,md.offset);
    if(pix==(u8*)-1){ps("mmap fail\n");return 1;}

    u32*p=(u32*)pix;
    for(u32 y=0;y<mode.vdisplay;y++)for(u32 x=0;x<mode.hdisplay;x++){
        u32 c=0xff1a1a2e;
        if((y/32)%2==0&&(x/32)%2==0)c=0xff2a2a3e;           /* subtle grid */
        if(y>60&&y<180)c=0xffc2185b;                        /* hibiscus pink band */
        if(x>mode.hdisplay/2-120&&x<mode.hdisplay/2+120&&y>mode.vdisplay/2-70&&y<mode.vdisplay/2+70)c=0xff00d0d0;
        p[y*mode.hdisplay+x]=c;
    }

    struct drm_mode_crtc s={0};
    s.crtc_id=crtc;s.fb_id=a.fb_id;s.set_connectors_ptr=(u64)&conn;s.count_connectors=1;
    s.mode=mode;s.mode_valid=1;
    if(ioct(fd,DRM_IOCTL_MODE_SETCRTC,&s)){ps("setcrtc fail\n");return 1;}
    ps("modeset OK ");pn(mode.hdisplay);ps("x");pn(mode.vdisplay);ps("\n");
    for(;;)snooze(60);
    return 0;
}
void _start(void){int r=main();sys1(__NR_exit,r);}
