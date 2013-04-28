#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include "crypti_io.h"
	
int
cio_full_read(int fd, void *buf, size_t size)
{
        size_t left;
        int res;

        left = size;

        while (left) {
                res = read(fd, buf, left);
                if (res == -1) {
                        if (errno != EINTR)
                                return -1; 
                        continue;
                }   
                if (res == 0)
                        break;
                left -= res;
                buf += res;
        }   

        return left;
}

