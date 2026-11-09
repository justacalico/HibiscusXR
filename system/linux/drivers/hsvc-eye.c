// Copyright 2026, HibiscusXR
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  hsvc-eye - generic eye tracking camera service.
 *         hardware: any V4L2 grayscale camera (--device /dev/videoN),
 *         or a raw-frame file for replay (--file), or --sim.
 *         Publishes kind=eye: pupil centroids -> normalized gaze.
 */
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <linux/videodev2.h>

#include "hsvc/hsvc.h"
#include "gaze.h"

struct eye
{
	int fd;              /* v4l2 dev or -1 */
	void *buf;           /* mmap'd frame buffer */
	uint32_t buf_len;
	uint32_t w, h;
	uint8_t thresh;
	struct hsvc_eye_cal cal[2]; /* 0 left, 1 right */
	FILE *replay;        /* --file backend */
	int sim;
	struct hsvc_service *svc;
	uint64_t seq;
};

static uint64_t
now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static int
v4l_open(struct eye *e, const char *path)
{
	e->fd = open(path, O_RDWR | O_NONBLOCK);
	if (e->fd < 0)
		return -1;
	struct v4l2_format fmt = {0};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = e->w;
	fmt.fmt.pix.height = e->h;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
	if (ioctl(e->fd, VIDIOC_S_FMT, &fmt) < 0)
		return -1;
	e->w = fmt.fmt.pix.width;
	e->h = fmt.fmt.pix.height;
	struct v4l2_requestbuffers req = {0};
	req.count = 1;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (ioctl(e->fd, VIDIOC_REQBUFS, &req) < 0 || req.count < 1)
		return -1;
	struct v4l2_buffer vb = {0};
	vb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vb.memory = V4L2_MEMORY_MMAP;
	vb.index = 0;
	if (ioctl(e->fd, VIDIOC_QUERYBUF, &vb) < 0)
		return -1;
	e->buf_len = vb.length;
	e->buf = mmap(NULL, vb.length, PROT_READ | PROT_WRITE, MAP_SHARED,
	              e->fd, vb.m.offset);
	if (e->buf == MAP_FAILED)
		return -1;
	if (ioctl(e->fd, VIDIOC_QBUF, &vb) < 0)
		return -1;
	enum v4l2_buf_type t = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	return ioctl(e->fd, VIDIOC_STREAMON, &t);
}

static int
v4l_grab(struct eye *e)
{
	struct v4l2_buffer vb = {0};
	vb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vb.memory = V4L2_MEMORY_MMAP;
	if (ioctl(e->fd, VIDIOC_DQBUF, &vb) < 0)
		return -1;
	vb.index = 0;
	if (ioctl(e->fd, VIDIOC_QBUF, &vb) < 0)
		return -1;
	return 0;
}

/* One eye half -> gaze. Cameras typically expose both eyes in one
 * frame side by side; `half` selects which. */
static int
frame_gaze(struct eye *e, const uint8_t *img, int half, float *gx,
           float *gy)
{
	uint32_t w = e->w / 2;
	const uint8_t *sub = img + (half ? w : 0);
	uint8_t *tmp = malloc(w * e->h);
	for (uint32_t y = 0; y < e->h; y++)
		memcpy(tmp + y * w, sub + y * e->w, w);
	float px, py;
	int rc = hsvc_pupil_centroid(tmp, w, e->h, e->thresh, &px, &py);
	free(tmp);
	if (rc < 0)
		return -1;
	return hsvc_gaze(&e->cal[half], px, py, gx, gy);
}

static void
emit(struct hsvc_service *s, void *ud)
{
	struct eye *e = ud;
	struct hsvc_gaze gz = {0};
	uint64_t ts = now_ns();
	if (e->sim) {
		double t = (double)e->seq++ / 120.0;
		gz.lx = gz.rx = (float)sin(t);
		gz.ly = gz.ry = (float)cos(t);
		gz.valid = 3;
	} else {
		const uint8_t *img = e->buf;
		if (e->replay) {
			static uint8_t *fb;
			if (!fb)
				fb = malloc(e->w * e->h);
			if (fread(fb, 1, e->w * e->h, e->replay) != e->w * e->h) {
				rewind(e->replay);
				if (fread(fb, 1, e->w * e->h, e->replay) != e->w * e->h)
					return;
			}
			img = fb;
		} else if (e->fd >= 0) {
			if (v4l_grab(e) < 0)
				return;
		}
		if (frame_gaze(e, img, 0, &gz.lx, &gz.ly) == 0)
			gz.valid |= 1;
		if (frame_gaze(e, img, 1, &gz.rx, &gz.ry) == 0)
			gz.valid |= 2;
	}
	hsvc_stream(s, 0, ts, &gz, sizeof(gz));
}

int
main(int argc, char **argv)
{
	signal(SIGPIPE, SIG_IGN);
	struct eye e = {.fd = -1, .w = 640, .h = 400, .thresh = 200};
	const char *dev = NULL, *file = NULL, *name = "eye";
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--device") && i + 1 < argc)
			dev = argv[++i];
		else if (!strcmp(argv[i], "--file") && i + 1 < argc)
			file = argv[++i];
		else if (!strcmp(argv[i], "--sim"))
			e.sim = 1;
		else if (!strcmp(argv[i], "--width") && i + 1 < argc)
			e.w = (uint32_t)atoi(argv[++i]);
		else if (!strcmp(argv[i], "--height") && i + 1 < argc)
			e.h = (uint32_t)atoi(argv[++i]);
		else if (!strcmp(argv[i], "--thresh") && i + 1 < argc)
			e.thresh = (uint8_t)atoi(argv[++i]);
		else if (!strcmp(argv[i], "--name") && i + 1 < argc)
			name = argv[++i];
	}
	/* default calibration: frame center, half-frame travel */
	for (int i = 0; i < 2; i++) {
		e.cal[i].cx = e.w / 4.0f;
		e.cal[i].cy = e.h / 2.0f;
		e.cal[i].sx = e.w / 4.0f;
		e.cal[i].sy = e.h / 2.0f;
		e.cal[i].present = 1;
	}
	if (dev && v4l_open(&e, dev) < 0) {
		fprintf(stderr, "hsvc-eye: %s: %s\n", dev, strerror(errno));
		return 1;
	}
	if (file) {
		e.replay = fopen(file, "rb");
		if (!e.replay)
			return 1;
	}
	if (!e.sim && e.fd < 0 && !e.replay) {
		fprintf(stderr,
		        "hsvc-eye: need --device P, --file F or --sim\n");
		return 2;
	}
	char desc[128];
	snprintf(desc, sizeof(desc), "kind=3\ndrv=v4l2\nw=%u\nh=%u\n", e.w,
	         e.h);
	struct hsvc_service *s = hsvc_publish(name, HSVC_KIND_EYE, desc);
	if (!s)
		return 1;
	return hsvc_serve(s, 8, emit, &e);
}
