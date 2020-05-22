#ifndef _SANDBOX_H_
#define _SANDBOX_H_
#include <time.h>
#include <stdarg.h>

/* ISO 8601 format plus null-terminator -> 25 chars */
#define TIMELEN 25
static char TIME[TIMELEN] = {0};
static inline char *
GETTIME(void) {
	time_t t = {0};
	struct tm *tmp = NULL;
	
	t = time(NULL);
	tmp = localtime(&t);
	if (NULL == tmp) {
		fprintf(stderr, "WARNING: localtime() failed with error %s!",
		        strerror(errno));
		return NULL;
	}

	/* strftime(3) expects the inclusion of the terminating null byte, and
	 * does not set errno as defined by POSIX.1-2001.
	 */
	size_t rv = strftime(TIME, TIMELEN, "%FT%T%z", tmp);
	if (0 == rv) {
		fprintf(stderr, "WARNING: strftime in GETTIME() returned 0!\n");
	}
	return TIME;
}
#undef TIMELEN

/* all logging facilities. */
static inline void 
_LOG(FILE *file, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(file, fmt, ap);

	va_end(ap);
}

#define LOG(file, fmt, ...) \
	_LOG(file, "%s " fmt "\n", GETTIME() __VA_OPT__(,) __VA_ARGS__)

#define die(msg) do {                                                   \
	LOG(stderr, "FATAL: %s: %s. Exiting.", msg, strerror(errno)); \
	exit(EXIT_FAILURE);                                             \
} while (0)

/* enable __DEBUG__ with compiler (e.g. -D __DEBUG__) when building. */
#ifdef __DEBUG__
#define DEBUG(...) LOG(stderr, "DEBUG: " __VA_ARGS__)
#else
#define DEBUG(...)
#endif /* __DEBUG__ */

#define INFO(...) LOG(stderr, "INFO: " __VA_ARGS__)

#define NOTICE(...) LOG(stderr, "NOTICE: " __VA_ARGS__)

#define ERR(...) LOG(stderr, "ERR: " __VA_ARGS__)

#define WARN(...) LOG(stderr, "WARN: " __VA_ARGS__)

#define FATAL(...) LOG(stderr, "FATAL: " __VA_ARGS__)

#endif /* _SANDBOX_H_ */
