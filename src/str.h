#ifndef _STR_H_
#define _STR_H_

#define strlncmp(BIG, LITTLE) strncmp(BIG, LITTLE, strlen(LITTLE))
char **str_tokenize(char *);
size_t str_arraylen(char **);

#endif /* _STR_H_ */
