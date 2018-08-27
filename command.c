#include "command.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#include "utils.h"
#include "common.h"
#include "handler.h"

command_t* cmd_parse(const char *str) {
    char ** sub_cmds = str_split(str, "|");
    if (!sub_cmds) {
        return NULL;
    }

    size_t nr_cmd = 0;
    char **ptr = sub_cmds;
    while (*ptr) {
        str_trim_space(*ptr);
        ptr += 1;
        nr_cmd += 1;
    }

    //
    // We add an extra fake `command` in the end
    // to indicate the output redirection where:
    //
    // cmd_args_[0] = filename stdout redirect to
    // cmd_args_[1] = filename stderr redirect to
    //
    command_t *cmds = NULL;
    NEW_ARRAY(cmds, command_t, nr_cmd+1);

    for (size_t i = 0; i < nr_cmd-1; ++i) {
        cmds[i].cmd_args_ = str_split(sub_cmds[i], " ");
        cmds[i].cmd_name_ = cmds[i].cmd_args_[0];
    }

    //
    // Parse the last command for redirection;
    // Add output redirection info to extra command
    //
    cmds[nr_cmd].cmd_name_ = NULL;
    NEW_ARRAY(cmds[nr_cmd].cmd_args_, char*, 3);
    cmds[nr_cmd].cmd_args_[0] = NULL;
    cmds[nr_cmd].cmd_args_[1] = NULL;
    cmds[nr_cmd].cmd_args_[2] = NULL;

    char *last_cmd = sub_cmds[nr_cmd-1];

    //
    // Both redirect to the same file
    //
    char *pos = strstr(last_cmd, "&>");
    if(pos != NULL) {
        *pos = '\0';
        cmds[nr_cmd-1].cmd_args_ = str_split(sub_cmds[nr_cmd-1], " ");
        cmds[nr_cmd-1].cmd_name_ = cmds[nr_cmd-1].cmd_args_[0];

        pos += 2;

        cmds[nr_cmd].cmd_args_[0] = strdup(pos);
        cmds[nr_cmd].cmd_args_[1] = strdup(pos);
        return cmds;
    }

    //
    // stderr redirect to stdout
    //
    pos = strstr(last_cmd, "2>&1");
    if (pos != NULL) {
        memmove(pos, pos+4, strlen(pos+4)+1);
        cmds[nr_cmd].cmd_args_[1] = strdup("stdout");
    }

    //
    // stdout redirect
    //
    pos = strstr(last_cmd, ">");
    if (pos != NULL) {
        *pos = '\0';

        char *start = pos+1;
        while(*start == ' ') {
            start += 1;
        }

        if (*start != '\0') {
            char *end = start;
            while (*end && *end != ' ') {
                end += 1;
            }
            char ch = *end;
            *end = '\0';
            cmds[nr_cmd].cmd_args_[0] = strdup(start);
            *end = ch;
            memmove(pos, end, strlen(end)+1);
        }
    }

    pos = strstr(last_cmd, "2>");
    if (pos != NULL) {
        *pos = '\0';

        char *start = pos+2;
        while(*start == ' ') {
            start += 1;
        }

        if (*start != '\0') {
            char *end = start;
            while (*end && *end != ' ') {
                end += 1;
            }
            char ch = *end;
            *end = '\0';
            cmds[nr_cmd].cmd_args_[1] = strdup(start);
            *end = ch;
            memmove(pos, end, strlen(end)+1);
        }
    }

    cmds[nr_cmd-1].cmd_args_ = str_split(sub_cmds[nr_cmd-1], " ");
    cmds[nr_cmd-1].cmd_name_ = cmds[nr_cmd-1].cmd_args_[0];
    return cmds;
}

void cmd_free(command_t *cmds) {
    int nr_cmd = cmd_count(cmds);
    for (int i = 0; i < nr_cmd; ++i) {
        //
        // NOTE cmd.cmd_name_ is pointing to cmd.cmd_args_[0]
        //
        str_free_array(cmds[i].cmd_args_);
    }
}

int cmd_count(command_t *cmds) {
    int nr = 0;
    command_t *ptr = cmds;
    while (ptr->cmd_name_) {
        ptr += 1;
        nr += 1;
    }
    return nr;
}

int cmd_exec_one(command_t cmd) {
    handler_t *h = find_handler(cmd.cmd_name_);
    if (h) {
        return h->handle_func(cmd.cmd_args_);
    } else {
        execvp(cmd.cmd_name_, cmd.cmd_args_);

        printf("Unknown command %s\n", cmd.cmd_name_);
        exit(errno);
    }
}

/**
 *   cmd0    cmd1   cmd2   cmd3   cmd4
 *      pipe0   pipe1  pipe2  pipe3
 *      [0,1]   [2,3]  [4,5]  [6,7]
 */
static void dup_pipe(int *pipe_fds, int cmd_order, int nr_cmd) {
    //
    // if not last command
    //
    if (cmd_order != nr_cmd-1) {
        if (dup2(pipe_fds[cmd_order*2 + 1], STDOUT_FILENO) < 0) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
    }

    //
    // if not first command
    //
    if (cmd_order != 0) {
        if (dup2(pipe_fds[cmd_order*2 - 2], STDIN_FILENO) < 0) {
            perror(" dup2");
            exit(EXIT_FAILURE);
        }
    }
}

int cmd_exec_pipe(command_t *cmds) {
    int nr_cmd = cmd_count(cmds);
    assert(nr_cmd > 0);

    int nr_pipe = nr_cmd-1;
    int *pipe_fds = pipe_open(nr_pipe);
    pid_t child_pids[nr_cmd];

    DBG_PRINTF("#pipe %d\n", nr_pipe);

    for (int i = 0; i < nr_cmd; ++i) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            perror("error");
            exit(EXIT_FAILURE);
        } else if (child_pid == 0) {
            if (nr_pipe) {
                dup_pipe(pipe_fds, i, nr_cmd);
                pipe_close(pipe_fds, nr_pipe);
            }
            cmd_exec_one(cmds[i]);
        } else {
            child_pids[i] = child_pid;
        }
    }

    //
    // Parent closes the pipes and wait for children
    //
    if (nr_pipe) {
        pipe_close(pipe_fds, nr_pipe);
    }

    int status;
    for (int i = 0; i < nr_cmd; i++) {
        wait(&status);
        if (WIFEXITED(status)) {
            DBG_PRINTF("child exited with = %d %s\n",
                       WEXITSTATUS(status),
                       strerror(errno));
        }
    }
    return status;
}

int cmd_exec(command_t *cmds) {
    int nr_cmd = cmd_count(cmds);
    assert(nr_cmd > 0);

    if (nr_cmd == 1) {
        handler_t *h = find_handler(cmds[0].cmd_name_);
        if (h) {
            return h->handle_func(cmds[0].cmd_args_);
        }
    }

    cmd_exec_pipe(cmds);
    return 0;
}
