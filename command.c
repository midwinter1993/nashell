#include "command.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "utils.h"
#include "common.h"
#include "handler.h"

/*
 * `str` can be modified in place, i.e., the redirection info will be erased.
 * `pat` is one of "&>", ">", "2>"
 */
static char* cmd_parse_redirect(char *str, const char *pat) {
    char *ptr = strstr(str, pat);
    if (!ptr) {
        return NULL;
    }

    *ptr = '\0';

    char *filename_start = ptr + strlen(pat);
    while (*filename_start == ' ') {
        filename_start += 1;
    }

    if (*filename_start != '\0') {
        char *filename_end = filename_start;
        while (*filename_end && *filename_end != ' ' && *filename_end != '>') {
            filename_end += 1;
        }

        char *filename = str_range_copy(filename_start, filename_end);
        str_move(ptr, filename_end);

        return filename;
    }

    return NULL;
}

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
    command_t *cmds = array_new(command_t, nr_cmd+1);

    for (size_t i = 0; i < nr_cmd-1; ++i) {
        cmds[i].cmd_args_ = str_split(sub_cmds[i], " ");
        cmds[i].cmd_name_ = cmds[i].cmd_args_[0];
    }

    //
    // Parse the last command for redirection;
    // Add output redirection info to the last extra command
    //
    cmds[nr_cmd].cmd_name_ = NULL;
    cmds[nr_cmd].cmd_args_ = array_new(char*, 3);
    cmds[nr_cmd].cmd_args_[0] = NULL;
    cmds[nr_cmd].cmd_args_[1] = NULL;
    cmds[nr_cmd].cmd_args_[2] = NULL;

    char *last_cmd = sub_cmds[nr_cmd-1];

    //
    // Both redirect to the same file
    //
    char *filename = NULL;
    do {
        filename = cmd_parse_redirect(last_cmd, "&>");

        if (filename) {
            assert(!str_equal(filename, "&1") && !str_equal(filename, "&2"));
            cmds[nr_cmd].cmd_args_[0] = str_copy(filename);
            cmds[nr_cmd].cmd_args_[1] = str_copy(filename);

            DBG_PRINTF("&> to %s; rest command: %s\n", filename, last_cmd);
            str_free(filename);
        }
    } while (filename);

    //
    // stderr redirection
    //
    do {
        filename = cmd_parse_redirect(last_cmd, "2>");
        if (filename) {
            if (str_equal(filename, "&1")) {
                cmds[nr_cmd].cmd_args_[1] = str_copy("stdout");
            } else {
                cmds[nr_cmd].cmd_args_[1] = str_copy(filename);
            }
        }
        if (filename) {
            DBG_PRINTF("2> to %s; rest command: %s\n", filename, last_cmd);
            str_free(filename);
        }
    } while (filename);

    //
    // stdout redirection
    //
    do {
        filename = cmd_parse_redirect(last_cmd, ">");
        if (filename) {
            if (str_equal(filename, "&2")) {
                cmds[nr_cmd].cmd_args_[0] = str_copy("stderr");
            } else {
                cmds[nr_cmd].cmd_args_[0] = str_copy(filename);
            }
        }
        if (filename) {
            DBG_PRINTF("1> to %s; rest command: %s\n", filename, last_cmd);
            str_free(filename);
        }
    } while (filename);

    //
    // The actual last command has been processed (redirection info removed ).
    //
    cmds[nr_cmd-1].cmd_args_ = str_split(last_cmd, " ");
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
    return array_size(cmds) - 1;
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

static void redirect(const char *out_filename, const char *err_filename) {
    if (out_filename) {
        int out_fd = STDOUT_FILENO;
        if (str_equal(out_filename, "stderr")) {
            out_fd = STDERR_FILENO;
        } else {
            out_fd = open(out_filename, O_WRONLY|O_CREAT, 0666);
        }
        dup2(out_fd, STDOUT_FILENO);
    }
    if (err_filename) {
        int err_fd = STDERR_FILENO;
        if (str_equal(err_filename, "stdout")) {
            err_fd = STDOUT_FILENO;
        } else {
            err_fd = open(out_filename, O_WRONLY|O_CREAT, 0666);
        }
        dup2(err_fd, STDERR_FILENO);
    }
}

static int wait_children(pid_t *child_pids) {
    int status = 0;
    for (uint32_t i = 0; i < array_size(child_pids); i++) {
        wait(&status);
        if (WIFEXITED(status)) {
            DBG_PRINTF("child exited with = %d %s\n",
                       WEXITSTATUS(status),
                       strerror(errno));
        }
    }
    return status;
}

int cmd_exec_pipe(command_t *cmds) {
    int nr_cmd = cmd_count(cmds);
    assert(nr_cmd > 0);

    int nr_pipe = nr_cmd-1;
    int *pipe_fds = pipe_open(nr_pipe);
    pid_t *child_pids = array_new(pid_t, nr_cmd);

    DBG_PRINTF("#pipe %d\n", nr_pipe);

    for (int i = 0; i < nr_cmd; ++i) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            perror("error");
            exit(EXIT_FAILURE);
        } else if (child_pid == 0) {
            //
            // Child process
            //
            if (nr_pipe) {
                dup_pipe(pipe_fds, i, nr_cmd);
                pipe_close(pipe_fds, nr_pipe);
            }
            //
            // handle redirection for last command
            //
            if (i == nr_cmd-1) {
                const char *out_filename = cmds[nr_cmd].cmd_args_[0];
                const char *err_filename = cmds[nr_cmd].cmd_args_[1];
                redirect(out_filename, err_filename);
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

    array_free(child_pids);
    return wait_children(child_pids);
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

    return cmd_exec_pipe(cmds);
}
