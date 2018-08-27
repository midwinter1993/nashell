#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define UNUSE(x) ((void)x)

#define DEBUG_ON

#ifdef DEBUG_ON
#define DBG_PRINTF(fmt, ...) \
    do { fprintf(stderr, fmt, ## __VA_ARGS__); } while(0)
#else
#define DBG_PRINTF(fmt, ...) \
    do { } while(0)
#endif

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


#endif /* ifndef __COMMON_H__ */


