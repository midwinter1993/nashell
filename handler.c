#include "handler.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "utils.h"

static list_head g_handlers;

void add_handler(const char *name, handle_func_t f) {
    handler_t *h = NULL;
    NEW_INSTANCE(h, handler_t);

    h->name_ = strdup(name);
    h->handle_func = f;

    list_add_tail(&h->link_, &g_handlers);
}

handler_t* find_handler(const char *name) {
    list_head *pos = NULL;
    list_for_each(pos, &g_handlers) {
        handler_t *h = list_entry(pos, handler_t, link_);
        if (str_equal(h->name_, name)) {
            return h;
        }
    }
    return NULL;
}

static int args_nr(char **args) {
    char **ptr = args;
    int nr = 0;
    while (*ptr) {
        ptr += 1;
        nr +=1;
    }
    return nr;
}


#define CHECK_BUILTIN(name, n) \
    do { \
        assert(str_equal(name, args[0])); \
        assert(args_nr(args) == n); \
    } while(0);


static int do_exit(char **args) {
    UNUSE(args);
    CHECK_BUILTIN("exit", 1);

    exit(EXIT_SUCCESS);
}

static int do_cd(char **args) {
    UNUSE(args);
    CHECK_BUILTIN("cd", 2);

    return chdir(args[1]);
}

void add_builtin_handlers() {
    list_init(&g_handlers);
    add_handler("exit", do_exit);
    add_handler("cd", do_cd);
}
