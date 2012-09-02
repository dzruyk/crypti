#ifndef __CRYPTI_H_
#define __CRYPTI_H_

#define IS_DEBUG 0

#define LOG_DEFAULT 0
#define LOG_VERBOSE 1

//WIP debug macro
#if IS_DEBUG == 1

#define DEBUG(LOG_LVL, fmt, arg...) \
do {\
    fprintf(stderr, "file %s, line %d:"fmt,\
    __FILE__, \
    __LINE__, \
    ##arg); \
} while (0)

#else

#define DEBUG(LOG_LVL, fmt, arg...)

#endif

#endif

