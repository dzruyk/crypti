#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "crypti_io.h"
#include "macros.h"
#include "random.h"

int
rand_get_rand_bytes(unsigned char *dst, int nbytes)
{
	int fd;

	fd = open("/dev/urandom", O_RDONLY);
	if (fd == -1) {
		perror("urandom");
		error(1, "can't open urandom\n");
	}
	if (cio_full_read(fd, dst, nbytes) == -1)
		error(1, "read err\n");
	close(fd);

	return 0;
}

