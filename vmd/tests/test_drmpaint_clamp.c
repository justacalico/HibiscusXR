// drmpaint_hooks.c is drmpaint.c with its ioct/open call sites and its
// entry points rewired to the hooks below by run.sh; the fake ioct
// acts like the DRM kernel and writes as many entries as the caller's
// count fields advertise.
static int tst_open(const char *p, int fl);
static long tst_ioct(int fd, unsigned long req, void *arg);

#include "out/drmpaint_hooks.c"

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

static long tst_ioct(int fd, unsigned long req, void *arg)
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
		return -1;
	}
	return -1;
}

static int tst_open(const char *p, int fl)
{
	(void)p; (void)fl;
	return (int)sys3(__NR_openat, -100, (long)"/dev/zero",
	                 O_RDWR|O_CLOEXEC);
}

int main(void)
{
	nconn_host = 200; ncrtc_host = 0; nenc_host = 90;
	nmodes_host = 900; nconnenc_host = 50; conn_link = 1;
	if (drmpaint_main() != 1 || setcrtc_seen || overflow) {
		ps("FAIL zero-crtc path\n");
		return 1;
	}
	ps("PASS no crtcs + hostile counts handled\n");

	overflow = 0; setcrtc_seen = 0;
	ncrtc_host = 50;
	if (drmpaint_main() != 1 || !setcrtc_seen || overflow) {
		ps("FAIL full path with hostile counts\n");
		return 1;
	}
	ps("PASS full modeset path, all clamps held\n");
	return 0;
}

void _start(void) { sys1(__NR_exit, main()); }
