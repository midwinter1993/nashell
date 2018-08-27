#ifndef __COMMAND_H__
#define __COMMAND_H__

typedef struct command_t {
    const char *cmd_name_;
    //
    // The first element of `cmd_args_` must be `cmd_name_`
    //
    char **cmd_args_;
} command_t;

command_t* cmd_parse(const char *str);
void cmd_free(command_t *cmds);

int cmd_count(command_t *cmds);
// int cmd_exec_pipe(command_t *cmds);
int cmd_exec(command_t *cmds);

#endif /* ifndef __COMMAND_H__ */
