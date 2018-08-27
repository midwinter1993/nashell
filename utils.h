#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdbool.h>

char* readline();

char* str_trim_space(char *str);
char** str_split(const char *str, const char *delim);
bool str_equal(const char *s1, const char *s2);

#endif /* ifndef __UTILS_H__ */
