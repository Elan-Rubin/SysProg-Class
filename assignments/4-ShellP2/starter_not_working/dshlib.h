#ifndef __DSHLIB_H__
#define __DSHLIB_H__

// Constants for command structure sizes
#define CMD_ARGV_MAX 64
#define EXE_MAX 64
#define ARG_MAX 256
#define CMD_MAX 8
#define SH_CMD_MAX EXE_MAX + ARG_MAX

typedef struct cmd_buff {
    int argc;
    char *argv[CMD_ARGV_MAX];
    char *_cmd_buffer;
} cmd_buff_t;

typedef struct command {
    char exe[EXE_MAX];
    char args[ARG_MAX];
} command_t;

typedef struct command_list {
    int num;
    command_t commands[CMD_MAX];
} command_list_t;

// Special character defines
#define SPACE_CHAR ' '
#define PIPE_CHAR '|'
#define PIPE_STRING "|"

#define SH_PROMPT "dsh2> "
#define EXIT_CMD "exit"

// Standard Return Codes
#define OK 0
#define WARN_NO_CMDS -1
#define ERR_TOO_MANY_COMMANDS -2
#define ERR_CMD_OR_ARGS_TOO_BIG -3

// Output constants
#define CMD_OK_HEADER "PARSED COMMAND LINE - TOTAL COMMANDS %d\n"
#define CMD_WARN_NO_CMD "warning: no commands provided\n"
#define CMD_ERR_PIPE_LIMIT "error: piping limited to %d commands\n"
#define CMD_ERR_EXECUTE "error: command failed to execute\n"

// Function prototypes
int exec_local_cmd_loop(void);
int build_cmd_list(char *cmd_line, command_list_t *clist);
int parse_command(char *input, cmd_buff_t *cmd);
int handle_cd_command(cmd_buff_t *cmd);
int execute_external_command(command_t *cmd);

#endif