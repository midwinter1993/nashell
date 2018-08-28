#include "utils.h"

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "common.h"


/**
 * Count #space of string.
 * `str` should not starts with space.
 */
static int str_count_char(const char *str, char ch) {
    int cnt = 0;

    const char *ptr = str;
    while (*ptr) {
        if (ch == *ptr) {
            cnt += 1;
        }
        ptr += 1;
    }
    return cnt;
}

void str_free_array(char **arr) {
    //
    // NOTE we put a NULL pointer at last.
    //
    if (!arr) {
        return;
    }
    char **ptr = arr;
    while (*ptr) {
        str_free(*ptr);
        ptr += 1;
    }
    array_free(arr);
}


// ===============================================

char* str_trim_space(char *str) {
    //
    // Remove leading space
    //
    char *start = str;
    while (isspace((unsigned char)*start)) {
        start += 1;
    }

    // All spaces
    if(*start == 0) {
        *str = '\0';
        return NULL;
    }

    if (str != start) {
        str_move(str, start);
    }

    //
    // Remove trailing space
    //
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        // Replace space with null terminator character
        *end = '\0';
        end -= 1;
    }

    //
    // Remove continuous spaces
    //
    char *ptr = str;
    while (*ptr) {
        if (isspace((unsigned char)*ptr)) {
            // Find next non-space character.
            char *next = ptr+1;
            while (*next == ' ') {
                next += 1;
            }
            if (next != ptr+1) {
                str_move(ptr+1, next);
            }
        }
        ptr += 1;
    }

    return str;
}

/**
 * Split the string by `space` delimiter.
 */
char** str_split(const char *str, const char *delim) {

    assert(strlen(delim) == 1);

    // `strtok` will modify the str, we make a copy first.
    char *copy = str_copy(str);
    if (!str_trim_space(copy)) {
        return NULL;
    }

    // #string = #space + 1.
    // We put an extra NULL pointer in the result array
    // to indicate the end of array.
    int nr = str_count_char(copy, *delim) + 1 + 1;

    char **ret = array_new(char*, nr);

    int pos = 0;
    char *token = strtok(copy, delim);

    while (token) {
        assert(pos < nr);
        ret[pos] = str_copy(token);
        token = strtok(0, delim);
        pos += 1;
    }

    assert(pos == nr-1);
    ret[pos] = NULL;

    str_free(copy);
    return ret;
}

bool str_equal(const char *s1, const char *s2) {
    return strcmp(s1, s2) == 0;
}

char* str_move(char *dst, char *src) {
    memmove(dst, src, strlen(src)+1);
    return dst;
}

char* str_copy(const char *str) {
    return str_len_copy(str, strlen(str));
}

char* str_range_copy(const char *start, const char *end) {
    uint32_t len = end - start;
    char *ptr = str_new(len+1);
    for (uint32_t i = 0; i < len; ++i) {
        ptr[i] = start[i];
    }
    return ptr;
}

char* str_len_copy(const char *start, uint32_t len) {
    const char *end = start + len;
    return str_range_copy(start, end);
}

void* instance_new_impl(uint32_t type_size,
                const char *filename, int line_no) {

    void *ptr = malloc(type_size);

    if (!ptr) {
        fprintf(stderr, "OOM at %s:%d\n", filename, line_no);
        exit(EXIT_FAILURE);
    }

    memset(ptr, 0, type_size);

    return ptr;
}

void* array_new_impl(uint32_t type_size, uint32_t nr_elem,
                const char *filename, int line_no) {

    uint32_t len = type_size * nr_elem + sizeof(uint32_t);
    void *ptr = malloc(len);

    if (!ptr) {
        fprintf(stderr, "OOM at %s:%d\n", filename, line_no);
        exit(EXIT_FAILURE);
    }

    memset(ptr, 0, len);
    *(uint32_t*)ptr = nr_elem;

    return (char*)ptr + sizeof(uint32_t);
}

static void* array_header(void *ptr) {
    return (char*)ptr - sizeof(uint32_t);
}

void array_free(void *ptr) {
    free(array_header(ptr));
}

uint32_t array_size(void *ptr) {
    return *(uint32_t*)array_header(ptr);
}

char* readline() {
    #define BUF_LEN 512
    static char buf[BUF_LEN];

    int pos = 0;
    char ch;

    while ((ch=getchar()) != '\n' && pos < BUF_LEN-1) {
        if (ch == EOF) {
            break;
        }
        buf[pos] = ch;
        pos += 1;
    }
    buf[pos] = '\0';
    return str_copy(buf);
}

int* pipe_open(int nr_pipe) {
    if (!nr_pipe) {
        return NULL;
    }

    int *pipe_fds = array_new(int, nr_pipe*2);

    for (int i = 0; i < nr_pipe; i++) {
        if (pipe(pipe_fds + i*2) < 0) {
            perror("Open pipe failure");
            exit(EXIT_FAILURE);
        }
    }
    return pipe_fds;
}

void pipe_close(int *pipe_fds, int nr_pipe) {
    if (!nr_pipe) {
        assert(!pipe_fds);
        return;
    }
    for (int i = 0; i < nr_pipe*2; i++) {
        close(pipe_fds[i]);
    }
    array_free(pipe_fds);
}
