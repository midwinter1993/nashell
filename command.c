#include "command.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "utils.h"
#include "common.h"

static void str_free_array(char **arr) {
    //
    // NOTE we put a NULL pointer at last.
    //
    char **ptr = arr;
    while (*ptr) {
        free(*ptr);
        ptr += 1;
    }
    free(arr);
}

command_t* parse_command(const char *str) {
    char ** sub_cmds = str_split(str, "|");
    if (!sub_cmds) {
        return NULL;
    }

    size_t nr = 0;
    char **ptr = sub_cmds;
    while (*ptr) {
        str_trim_space(*ptr);
        ptr += 1;
        nr += 1;
    }

    //
    // We add an extra fake `command` in the end
    // to indicate the output redirection.
    //
    command_t *cmds = NULL;
    NEW_ARRAY(cmds, command_t, nr+1);

    for (size_t i = 0; i < nr; ++i) {
        cmds[i].cmd_args_ = str_split(sub_cmds[i], " ");
        cmds[i].cmd_name_ = cmds[i].cmd_args_[0];
    }

    // TODO: add output redirection
    cmds[nr].cmd_name_ = NULL;
    cmds[nr].cmd_args_ = NULL;

    return cmds;
}

int count_command(command_t *cmds) {
    int nr = 0;
    command_t *ptr = cmds;
    while (ptr->cmd_name_) {
        ptr += 1;
        nr += 1;
    }
    return nr;
}

int run_one_command(command_t cmd) {
    pid_t child_pid;
    int child_status;

    child_pid = fork();

    if (child_pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child_pid == 0) {
        // Child process
        execvp(cmd.cmd_name_, cmd.cmd_args_);

        printf("Unknown command %s\n", cmd.cmd_name_);
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        while(child_pid != wait(&child_status)) {
            ;
        }
        return child_status;
    }
}

/**
 *   cmd0    cmd1   cmd2   cmd3   cmd4
 *      pipe0   pipe1  pipe2  pipe3
 *      [0,1]   [2,3]  [4,5]  [6,7]
 */
int run_command(command_t *cmds) {
    int nr_cmd = count_command(cmds);
    assert(nr_cmd > 0);

    if (nr_cmd == 1) {
        return run_one_command(cmds[0]);
    }

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

            command_t *cmd = &cmds[i];
            if (execvp(cmd->cmd_name_, cmd->cmd_args_) < 0) {
                printf("Unknown command %s\n", cmd->cmd_name_);
                exit(EXIT_FAILURE);
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
    }
    return status;
}
