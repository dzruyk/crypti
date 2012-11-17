#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mp.h"
#include "mp_common.h"

#define TESTS	30

int _mp_mul_toom_three(mp_int *c, const mp_int *a, const mp_int *b);
int _mp_mul_karatsuba(mp_int *c, const mp_int *a, const mp_int *b);

int
urandom(void *buf, size_t len)
{
	int fd, res;

	fd = open("/dev/urandom", O_RDONLY);
	if (fd == -1)
		return -1;
	res = read(fd, buf, len);
	close(fd);
	return res;
}

int
rnd(void *buf, size_t len, void *ctx)
{
	return urandom(buf, len);
}

int
main()
{
	mp_int a, c;
	struct rusage usage;
	struct timeval t0, t1;
	double ttoom, tkarat;
	unsigned int i, rc;
	int bits;

	rc = mp_initv(&a, &c, NULL);
	if (rc != MP_OK) {
		perror("can't mp_init");
		return 1;
	}

	rc = mp_random(&a, 7, rnd, NULL);
	if (rc != MP_OK) {
		perror("can't mp_random");
		goto err;
	}

	rc = mp_copy(&c, &a);
	if (rc != MP_OK) {
		perror("can't mp_random");
		goto err;
	}

	for (i = 2; i < TESTS; i++) {
		bits = mp_nr_bits(&a);
		printf("Test %i, %i bits\n", i, bits);

		rc = getrusage(RUSAGE_SELF, &usage);
		if (rc == -1) {
			perror("cat'n getrusage");
			goto err;
		}

		t0 = usage.ru_utime;

		rc = _mp_mul_toom_three(&a, &a, &a);
		if (rc != MP_OK) {
			perror("can't Toom-Cook");
			goto err;
		}

		rc = getrusage(RUSAGE_SELF, &usage);
		if (rc == -1) {
			perror("cat'n getrusage");
			goto err;
		}

		t1 = usage.ru_utime;
		ttoom  = (double)t1.tv_sec + ((double)t1.tv_usec)/1000000.0;
		ttoom -= (double)t0.tv_sec + ((double)t0.tv_usec)/1000000.0;

		printf("\ttoom:\t\t%lf\n", ttoom);

		rc = getrusage(RUSAGE_SELF, &usage);
		if (rc == -1) {
			perror("cat'n getrusage");
			goto err;
		}

		t0 = usage.ru_utime;

		rc = _mp_mul_karatsuba(&c, &c, &c);
		if (rc != MP_OK) {
			perror("can't Karatsuba");
			goto err;
		}

		rc = getrusage(RUSAGE_SELF, &usage);
		if (rc == -1) {
			perror("cat'n getrusage");
			goto err;
		}

		t1 = usage.ru_utime;
		tkarat  = (double)t1.tv_sec + ((double)t1.tv_usec)/1000000.0;
		tkarat -= (double)t0.tv_sec + ((double)t0.tv_usec)/1000000.0;

		printf("\tkaratsuba:\t%lf\n", tkarat);
	}

err:
	mp_clearv(&a, &c, NULL);

	return 0;
}

