// The fake ioct below acts like the DRM kernel: it writes as many
// entries as the caller's count fields advertise. Renaming the
// vmdhal.h helpers away lets drm_setup() run on the test stubs while
// real syscalls still work under qemu-aarch64.
#define ioct real_ioct
#define fd_open real_open
#define dbg real_dbg
#include "../hal/vmdhal.h"
#undef ioct
#undef fd_open
#undef dbg

static int fd_open(const char *p, int fl);
static long ioct(int fd, unsigned long req, void *arg);
static void dbg(const char *s) { (void)s; }

#include "../hal/hwcomposer_vmd.c"

static u32 nconn_host, ncrtc_host, nenc_host;
static u32 nmodes_host, nconnenc_host, conn_link;
static int overflow, setcrtc_seen;

static void fake_getres(struct drm_mode_card_res *r)
{
	if (r->connector_id_ptr) {
		u32 n = r->count_connectors < nconn_host ?
			r->count_connectors : nconn_host;
		if (n > 32) overflow = 1;
		u32 *p = (u32 *)r->connector_id_ptr;
		for (u32 i = 0; i < n; i++) p[i] = 100 + i;
	}
	if (r->crtc_id_ptr) {
		u32 n = r->count_crtcs < ncrtc_host ?
			r->count_crtcs : ncrtc_host;
		if (n > 32) overflow = 1;
		u32 *p = (u32 *)r->crtc_id_ptr;
		for (u32 i = 0; i < n; i++) p[i] = 200 + i;
	}
	if (r->encoder_id_ptr) {
		u32 n = r->count_encoders < nenc_host ?
			r->count_encoders : nenc_host;
		if (n > 32) overflow = 1;
		u32 *p = (u32 *)r->encoder_id_ptr;
		for (u32 i = 0; i < n; i++) p[i] = 300 + i;
	}
	r->count_connectors = nconn_host;
	r->count_crtcs = ncrtc_host;
	r->count_encoders = nenc_host;
}

static void fake_conn(struct drm_mode_get_connector *c)
{
	if (c->modes_ptr) {
		u32 n = c->count_modes < nmodes_host ?
			c->count_modes : nmodes_host;
		if (n > 64) overflow = 1;
		struct drm_mode_modeinfo *m = (void *)c->modes_ptr;
		for (u32 i = 0; i < n; i++) {
			m[i].hdisplay = 1280;
			m[i].vdisplay = 720;
		}
		u32 ne = c->count_encoders < nconnenc_host ?
			c->count_encoders : nconnenc_host;
		if (ne > 16) overflow = 1;
		u32 *e = (u32 *)c->encoders_ptr;
		for (u32 i = 0; i < ne; i++) e[i] = 400 + i;
	}
	c->count_modes = nmodes_host;
	c->count_encoders = nconnenc_host;
	c->connection = conn_link;
}

static long ioct(int fd, unsigned long req, void *arg)
{
	(void)fd;
	if (req == DRM_IOCTL_MODE_GETRESOURCES) {
		fake_getres(arg);
		return 0;
	}
	if (req == DRM_IOCTL_MODE_GETCONNECTOR) {
		fake_conn(arg);
		return 0;
	}
	if (req == DRM_IOCTL_MODE_CREATE_DUMB) {
		struct drm_mode_create_dumb *d = arg;
		d->handle = 1;
		d->pitch = d->width * 4;
		d->size = (u64)d->pitch * d->height;
		return 0;
	}
	if (req == DRM_IOCTL_MODE_ADDFB2) {
		((struct drm_mode_fb_cmd2 *)arg)->fb_id = 7;
		return 0;
	}
	if (req == DRM_IOCTL_MODE_MAP_DUMB) {
		((struct drm_mode_map_dumb *)arg)->offset = 0;
		return 0;
	}
	if (req == DRM_IOCTL_MODE_SETCRTC) {
		setcrtc_seen = 1;
		return 0;
	}
	return -1;
}

static int fd_open(const char *p, int fl)
{
	(void)p; (void)fl;
	return (int)sys3(__NR_openat, -100, (long)"/dev/zero",
	                 O_RDWR|O_CLOEXEC);
}

static void ps(const char *s) { wr(1, s, sl(s)); }

int main(void)
{
	hw_device_t *d = 0;
	nconn_host = 100; ncrtc_host = 100; nenc_host = 100;
	nmodes_host = 500; nconnenc_host = 64; conn_link = 1;
	if (hwc_open(0, "hwcomposer", &d) || !d) {
		ps("FAIL hwc_open with hostile counts\n");
		return 1;
	}
	if (overflow) {
		ps("FAIL kernel wrote past stack arrays\n");
		return 1;
	}
	if (!setcrtc_seen || g.conn != 100 || g.crtc != 200 ||
	    g.mode.hdisplay != 1280 || g.mode.vdisplay != 720) {
		ps("FAIL display state after clamped probe\n");
		return 1;
	}
	ps("PASS res 100/100/100 modes 500 clamped, modeset done\n");

	g.conn = 0; g.crtc = 0; overflow = 0; setcrtc_seen = 0;
	ncrtc_host = 0;
	if (hwc_open(0, "hwcomposer", &d) != -1 || setcrtc_seen) {
		ps("FAIL zero crtcs not rejected\n");
		return 1;
	}
	if (overflow) {
		ps("FAIL kernel wrote past stack arrays\n");
		return 1;
	}
	ps("PASS zero crtcs rejected before crtcs[0]\n");
	return 0;
}

void _start(void) { sys1(__NR_exit, main()); }
