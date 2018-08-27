#include "utils.h"

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
        free(*ptr);
        ptr += 1;
    }
    free(arr);
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
        memmove(str, start, strlen(start)+1);
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
                memmove(ptr+1, next, strlen(next)+1);
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
    char *str_copy = strdup(str);
    if (!str_trim_space(str_copy)) {
        return NULL;
    }

    // #string = #space + 1.
    // We put an extra NULL pointer in the result array
    // to indicate the end of array.
    int nr = str_count_char(str_copy, *delim) + 1 + 1;

    char **ret = NULL;
    NEW_ARRAY(ret, char*, nr);

    int pos = 0;
    char *token = strtok(str_copy, delim);

    while (token) {
        assert(pos < nr);
        ret[pos] = strdup(token);
        token = strtok(0, delim);
        pos += 1;
    }

    assert(pos == nr-1);
    ret[pos] = NULL;

    free(str_copy);
    return ret;
}

bool str_equal(const char *s1, const char *s2) {
    return strcmp(s1, s2) == 0;
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
    return strdup(buf);
}

int* pipe_open(int nr_pipe) {
    if (!nr_pipe) {
        return NULL;
    }

    int *pipe_fds = NULL;
    NEW_ARRAY(pipe_fds, int, nr_pipe*2);

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
    free(pipe_fds);
}
