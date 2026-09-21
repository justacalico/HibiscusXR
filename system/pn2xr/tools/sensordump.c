// sensordump: print raw accel + gyro values for axis-mapping work.
#include <android/sensor.h>
#include <android/looper.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef LOOPER_ID_USER
#define LOOPER_ID_USER 3
#endif

int
main(int argc, char **argv)
{
	int count = argc > 1 ? atoi(argv[1]) : 20;
	ASensorManager *sm = ASensorManager_getInstance();
	ALooper *looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
	ASensorEventQueue *q = ASensorManager_createEventQueue(sm, looper, LOOPER_ID_USER, NULL, NULL);
	const ASensor *acc = ASensorManager_getDefaultSensor(sm, ASENSOR_TYPE_ACCELEROMETER);
	const ASensor *gyr = ASensorManager_getDefaultSensor(sm, ASENSOR_TYPE_GYROSCOPE);
	ASensorEventQueue_enableSensor(q, acc);
	ASensorEventQueue_enableSensor(q, gyr);
	ASensorEventQueue_setEventRate(q, acc, 50000);
	ASensorEventQueue_setEventRate(q, gyr, 50000);

	float a[3] = {0}, g[3] = {0};
	int printed = 0;
	while (printed < count) {
		ASensorEvent ev[16];
		if (ALooper_pollOnce(200, NULL, NULL, NULL) < 0) {
			continue;
		}
		ssize_t n = ASensorEventQueue_getEvents(q, ev, 16);
		for (ssize_t i = 0; i < n; i++) {
			if (ev[i].type == ASENSOR_TYPE_ACCELEROMETER) {
				a[0] = ev[i].acceleration.x;
				a[1] = ev[i].acceleration.y;
				a[2] = ev[i].acceleration.z;
			} else if (ev[i].type == ASENSOR_TYPE_GYROSCOPE) {
				g[0] = ev[i].data[0];
				g[1] = ev[i].data[1];
				g[2] = ev[i].data[2];
				printf("acc=%+.3f %+.3f %+.3f  gyr=%+.4f %+.4f %+.4f\n", a[0], a[1], a[2],
				       g[0], g[1], g[2]);
				fflush(stdout);
				printed++;
			}
		}
	}
	return 0;
}
