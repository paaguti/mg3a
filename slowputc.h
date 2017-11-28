#include <sys/time.h>
#include <unistd.h>


/*
 * Time as a floating point double, in seconds.
 */

static double
slowdown_dtime()
{
	struct timeval t;

	gettimeofday(&t, NULL);
	return (double) t.tv_sec + (double) t.tv_usec * 1e-6;
}


INT slowspeed = -1;				// Speed in baud, with 10 bits per byte, negative off


/*
 * Potentially wait and flush to simulate output speed.
 */

static void
slowdown(INT c)
{
	static INT count=0;
	static double timepoint = 0.0;
	INT buf;
	double bufwait, now, delayperchar;

	buf = slowspeed/1000;			// 1000 = 10 bits per byte * 100 Hz
	if (buf < 1) buf = 1;

	if (++count >= buf && ((c & 0xc0) != 0x80 || !termisutf8)) {
		delayperchar = 10.0/(double) slowspeed;
		timepoint = timepoint + count*delayperchar;

		now = slowdown_dtime();
		bufwait = timepoint - now;

		if (bufwait < -0.2) {
			/* There is an external wait of more than */
			/* 200 msec, reset */
			timepoint = now;
		} else if (bufwait > 0.0) {
			usleep((useconds_t)(1.0e6*bufwait));
		}

		ttflush();
		count = 0;
	}
}
