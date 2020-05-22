#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include "sandbox.h"
#include "requests.h"
#include "sys.h"

/* r_handle() 
 * r_handle() expects two parameters in **toks:
 * [0]: the command
 * [1]: the parameter
 */
char *
r_handle(char **toks) {
	size_t ntok = 0;

	assert(NULL != toks);

	ntok = str_arraylen(toks);
	if (2 != ntok) {

	}
}
