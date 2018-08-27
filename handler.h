#ifndef __HANDLER_H__
#define __HANDLER_H__

#include "list.h"

typedef int (*handle_func_t)(char **args);

typedef struct handler_t {
    const char *name_;
    int (*handle_func)(char **args);
    list_head link_;
} handler_t;


handler_t* find_handler(const char *name);
void add_builtin_handlers();

#endif /* ifndef __HANDLER_H__ */
