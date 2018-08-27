#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "command.h"
#include "utils.h"
#include "handler.h"


#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
static int handle_commands(command_t *cmds) {

    int nr_cmd = count_command(cmds);
    assert(nr_cmd > 0);

    if (nr_cmd == 1) {
        handler_t *h = find_handler(cmds[0].cmd_name_);
        if (h) {
            return h->handle_func(cmds[0].cmd_args_);
        }
    }

    run_command(cmds);
    return 0;
}

static int test() {
    int nr_cmd = 2;
    assert(nr_cmd > 0);

    int nr_pipe = nr_cmd-1;

    DBG_PRINTF("#pipe %d\n", nr_pipe);

    int pipe_fds[2 * nr_pipe];
    pid_t child_pids[nr_cmd];

    for (int i = 0; i < nr_pipe; i++) {
        if (pipe(pipe_fds + i*2) < 0) {
            perror("Open pipe failure");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < nr_cmd; ++i) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            perror("error");
            exit(EXIT_FAILURE);
        } else if (child_pid == 0) {
            //
            // if not last command
            //
            if (i != nr_cmd-1) {
                if (dup2(pipe_fds[i*2 + 1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            //
            // if not first command
            //
            if (i != 0) {
                if (dup2(pipe_fds[i*2 - 2], STDIN_FILENO) < 0) {
                    perror(" dup2");
                    exit(EXIT_FAILURE);
                }
            }

            for (int j = 0; j < nr_pipe*2; j++) {
                close(pipe_fds[j]);
            }

            if (i == 0) {
                char *args[] = {"ls"};
                if (execvp("ls", args) < 0) {
                    printf("Unknown command ls\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                char *args[] = {"echo"};
                if (execvp("echo", args) < 0) {
                    printf("Unknown command echo\n");
                    exit(errno);
                }
            }
        } else {
            child_pids[i] = child_pid;
        }
    }

    //
    // Parent closes the pipes and wait for children
    //
    for (int i = 0; i < 2*nr_pipe; i++) {
        close(pipe_fds[i]);
    }

    int status;
    for (int i = 0; i < nr_cmd; i++) {
        /* wait(&status); */

        // Parent process
        while(child_pids[i] != wait(&status)) {
            ;
        }
        if(WIFEXITED(status)) {
            printf("child exited with = %d\n",WEXITSTATUS(status));
            printf("Oh dear, %s\n", strerror(errno));

        }
    }
    return status;
}

int main() {
    add_builtin_handlers();

    test();
    return 0;

    while (1) {
        char *line = readline();
        command_t *cmds = parse_command(line);

        if (!cmds) {
            fprintf(stderr, "Command error %s\n", line);
            continue;
        }

        handle_commands(cmds);

        free(cmds);
    }

    return 0;
}
