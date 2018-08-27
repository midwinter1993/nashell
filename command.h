#ifndef __COMMAND_H__
#define __COMMAND_H__

typedef struct command_t {
    const char *cmd_name_;
    //
    // The first element of `cmd_args_` must be `cmd_name_`
    //
    char **cmd_args_;
} command_t;

command_t* parse_command(const char *str);

int count_command(command_t *cmds);
int run_command(command_t *cmds);

#endif /* ifndef __COMMAND_H__ */
