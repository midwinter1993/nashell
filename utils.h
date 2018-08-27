#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdbool.h>

#define NEW_STRING(name, len) NEW_ARRAY(name, char, len)

#define NEW_ARRAY(name, type, len) \
    name = (type*)malloc(sizeof(type) * (len)); \
    if (!name) { \
        DBG_PRINTF("OOM\n"); \
        exit(EXIT_FAILURE); \
    }

#define NEW_INSTANCE(name, type) \
    name = (type*)malloc(sizeof(type)); \
    if (!name) { \
        DBG_PRINTF("OOM\n"); \
        exit(EXIT_FAILURE); \
    }

#define NOT_REACH_HERE() \
    do { assert("Don't reach here" && false); } while(0)


char* readline();

char* str_trim_space(char *str);
char** str_split(const char *str, const char *delim);
bool str_equal(const char *s1, const char *s2);
void str_free_array(char **arr);
char* str_move(char *dst, char *src);

int* pipe_open(int nr_pipe);
void pipe_close(int *pipe_fds, int nr_pipe);

#endif /* ifndef __UTILS_H__ */
