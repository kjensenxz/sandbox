#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "str.h"

/* split a message into a null-ptr-terminated string array. */
char **
str_tokenize(char *msg) {
	char **tok = malloc(2 * sizeof(char *));
	size_t i = 1;
	
	assert(NULL != tok);

	tok[0] = strtok(msg, " \t\r\n");

	while (NULL != (tok[i] = strtok(NULL, " \t\r\n"))) {
		++i;
		tok = realloc(tok, (i+1) * sizeof(char *));
		assert(NULL != tok);
	}

	tok[i] = NULL;

	return tok;
}

size_t
str_arraylen(char **arr) {
	size_t c = 0;

	if (NULL == arr) {
		return 0;
	}
	if (NULL == arr[0]) {
		return 0;
	}
	
	for (c = 0; NULL != arr[c]; ++c) {
		;
	}

	return c;
}
