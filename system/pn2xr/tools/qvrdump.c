// qvrdump: dump qvrservice head tracking poses for axis/calibration work.
// Reads the shared ring buffer like the pn2 driver does - no VR mode needed.
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct head_data {
	float quat[4];
	float position[3];
	uint32_t pad0;
	uint64_t timestamp_ns;
	uint32_t pad1[2];
	float angular_velocity[3];
	uint32_t pad2;
	float linear_velocity[3];
	uint32_t pad3;
	float reserved0[3];
	uint32_t pad4;
	float reserved1[3];
	uint32_t tracking_state;
	uint32_t warning_flags;
	float pose_quality;
	float sensor_quality;
	float camera_quality;
};

typedef void (*ctor_t)(void *);
typedef int (*head_t)(void *, struct head_data **);

int
main(int argc, char **argv)
{
	int count = argc > 1 ? atoi(argv[1]) : 200;
	int interval_us = argc > 2 ? atoi(argv[2]) : 50000;

	void *lib = dlopen("/system/lib64/libqvrservice_client.so", RTLD_NOW | RTLD_LOCAL);
	if (lib == NULL) {
		lib = dlopen("/vendor/lib64/libqvrservice_client.so", RTLD_NOW | RTLD_LOCAL);
	}
	if (lib == NULL) {
		fprintf(stderr, "dlopen failed: %s\n", dlerror());
		return 1;
	}
	ctor_t ctor = (ctor_t)dlsym(lib, "_ZN16QVRServiceClientC1Ev");
	head_t get_head = (head_t)dlsym(
	    lib, "_ZN20QVRServiceClientImpl19GetHeadTrackingDataEPP31qvrservice_head_tracking_data_t");
	if (ctor == NULL || get_head == NULL) {
		fprintf(stderr, "symbol resolution failed\n");
		return 1;
	}

	char client[16] = {0};
	ctor(client);
	void *impl = *(void **)client;
	if (impl == NULL) {
		fprintf(stderr, "null impl\n");
		return 1;
	}

	for (int i = 0; i < count; i++) {
		struct head_data *d = NULL;
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		long long mono = (long long)ts.tv_sec * 1000000000ll + ts.tv_nsec;
		if (get_head(impl, &d) < 0 || d == NULL) {
			printf("%lld read fail\n", mono);
		} else {
			printf("%lld q=%.6f %.6f %.6f %.6f p=%.5f %.5f %.5f st=%u pq=%.2f sq=%.2f cq=%.2f ts=%llu\n",
			       mono, d->quat[0], d->quat[1], d->quat[2], d->quat[3], d->position[0],
			       d->position[1], d->position[2], d->tracking_state, d->pose_quality,
			       d->sensor_quality, d->camera_quality, (unsigned long long)d->timestamp_ns);
		}
		fflush(stdout);
		usleep(interval_us);
	}
	return 0;
}
