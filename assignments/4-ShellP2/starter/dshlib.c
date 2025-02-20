#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#include "dshlib.h"

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    (void)cmd_line;
    (void)clist;
    return WARN_NO_CMDS;
}

int handle_cd_command(cmd_buff_t *cmd) {
    if (cmd->argc <= 1) {
        return 0;
    }

    if (chdir(cmd->argv[1]) != 0) {
        perror("cd");
        return -1;
    }

    return 0;
}

int parse_command(char *input, cmd_buff_t *cmd) {
    memset(cmd, 0, sizeof(cmd_buff_t));
    cmd->_cmd_buffer = strdup(input);
    if (!cmd->_cmd_buffer) {
        return -1;
    }

    char *trimmed = cmd->_cmd_buffer;
    while (isspace(*trimmed)) trimmed++;
    
    if (*trimmed == '\0') {
        free(cmd->_cmd_buffer);
        cmd->_cmd_buffer = NULL;
        return WARN_NO_CMDS;
    }

    char *end = trimmed + strlen(trimmed) - 1;
    while (end > trimmed && isspace(*end)) end--;
    *(end + 1) = '\0';

    // Parse the command line
    cmd->argc = 0;
    char *token = strtok(trimmed, " ");
    while (token != NULL && cmd->argc < CMD_ARGV_MAX - 1) {
        cmd->argv[cmd->argc++] = token;
        token = strtok(NULL, " ");
    }
    cmd->argv[cmd->argc] = NULL;

    return OK;
}

int execute_external_command(cmd_buff_t *cmd) {
    pid_t pid = fork();
    
    if (pid < 0) {
        fprintf(stderr, CMD_ERR_EXECUTE);
        return errno;
    }
    
    if (pid == 0) {
        execvp(cmd->argv[0], cmd->argv);
        fprintf(stderr, CMD_ERR_EXECUTE);
        exit(errno);
    } else {
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
}

int exec_local_cmd_loop(void) {
    char input_buffer[ARG_MAX];
    cmd_buff_t cmd;
    int last_return_code = 0;

    while (1) {
        printf(SH_PROMPT);
        fflush(stdout);
        
        if (fgets(input_buffer, ARG_MAX, stdin) == NULL) {
            printf("\ncmd loop returned 0\n");
            break;
        }

        input_buffer[strcspn(input_buffer, "\n")] = '\0';

        if (strlen(input_buffer) == 0) {
            continue;
        }

        if (strcmp(input_buffer, EXIT_CMD) == 0) {
            printf("cmd loop returned 0\n");
            exit(0);
        }

        int parse_result = parse_command(input_buffer, &cmd);
        
        if (parse_result == WARN_NO_CMDS || cmd.argc == 0) {
            continue;
        }

        if (strcmp(cmd.argv[0], "cd") == 0) {
            last_return_code = handle_cd_command(&cmd);
        } else if (strcmp(cmd.argv[0], "rc") == 0) {
            printf("%d\n", last_return_code);
        } else {
            last_return_code = execute_external_command(&cmd);
        }

        if (cmd._cmd_buffer) {
            free(cmd._cmd_buffer);
            cmd._cmd_buffer = NULL;
        }
    }

    return 0;
}