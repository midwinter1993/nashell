#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdbool.h>
#include <stdint.h>

#define NOT_REACH_HERE() \
    do { assert("Don't reach here" && false); } while(0)

// ===============================================

#define instance_new(type) \
    (type*)instance_new_impl(sizeof(type), __FILE__, __LINE__)

void* instance_new_impl(uint32_t type_size,
                const char *filename, int line_no);

// ===============================================

void array_free(void *ptr);
uint32_t array_size(void *ptr);

#define array_new(type, nr_elem) \
    (type*)array_new_impl(sizeof(type), (nr_elem), __FILE__, __LINE__)

void* array_new_impl(uint32_t type_size, uint32_t nr_elem,
                const char *filename, int line_no);

// ===============================================

#define str_new(len) \
    array_new(char, (len))

#define str_free array_free

char* str_trim_space(char *str);
char** str_split(const char *str, const char *delim);
bool str_equal(const char *s1, const char *s2);
void str_free_array(char **arr);
char* str_move(char *dst, char *src);
char* str_copy(const char *str);
char* str_range_copy(const char *start, const char *end);
char* str_len_copy(const char *start, uint32_t len);

// ===============================================

int* pipe_open(int nr_pipe);
void pipe_close(int *pipe_fds, int nr_pipe);

// ===============================================

char* readline();

#endif /* ifndef __UTILS_H__ */
