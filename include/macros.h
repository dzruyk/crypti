#ifndef MACROS_H_
#define MACROS_H_

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#ifndef NULL
#define NULL (void*)0
#endif

#define CRLF	"\r\n"

#define STREQ(s1, s2)	(strcmp(s1, s2) == 0)

#define SHOULDNT_REACH() \
do { \
	fprintf(stderr, "file %s line %d %s(): should not reach this" CRLF, \
		__FILE__, \
		__LINE__, \
		__FUNCTION__); \
	abort(); \
} while (0)

#define return_if_fail(expr) \
do { \
	if (!(expr)) { \
		fprintf(stderr, "file %s line %d %s(): expression `%s' failed" CRLF, \
			__FILE__, \
			__LINE__, \
			__FUNCTION__, \
			#expr); \
		return; \
	} \
} while (0)

#define return_val_if_fail(expr, val) \
do { \
	if (!(expr)) { \
		fprintf(stderr, "file %s line %d %s(): expression `%s' failed" CRLF, \
			__FILE__, \
			__LINE__, \
			__FUNCTION__, \
			#expr); \
		return (val); \
	} \
} while (0)

#define FREE(addr)	\
do { \
	free(addr);	\
	addr = NULL;	\
} while (0)


#define error(status, fmt, arg...) \
do { \
	fprintf(stderr, "error: %s:%d %s: "fmt CRLF, \
			__FILE__, \
			__LINE__, \
			__FUNCTION__, \
			##arg); \
	fflush(stderr); \
	exit(status);	\
} while (0)


#define warning(fmt, arg...) \
do { \
	fprintf(stderr, "warning: %s:%d %s: "fmt CRLF, \
			__FILE__, \
			__LINE__, \
			__FUNCTION__, \
			##arg); \
	fflush(stderr); \
} while (0)

#define ARRSZ(arr) (sizeof(arr) / sizeof(arr[0]))

#endif
