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


#endif /* ifndef __COMMON_H__ */


