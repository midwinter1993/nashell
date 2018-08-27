#include <stdio.h>

#include "command.h"
#include "utils.h"
#include "handler.h"
#include "sys.h"

static void prompt() {
    printf("$ %s >>> ", sys_curr_dir());
}

int main() {
    add_builtin_handlers();

    while (1) {
        prompt();
        char *line = readline();
        command_t *cmds = cmd_parse(line);

        if (!cmds) {
            fprintf(stderr, "Command error %s\n", line);
            continue;
        }

        cmd_exec(cmds);

        cmd_free(cmds);
    }

    return 0;
}
