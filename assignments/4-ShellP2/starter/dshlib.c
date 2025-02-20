#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include "dshlib.h"

static char *trim(char *str)
{
    while (isspace(*str))
        str++;

    if (*str == 0)
        return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end))
        end--;
    end[1] = '\0';

    return str;
}

int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    memset(clist, 0, sizeof(command_list_t));

    char *copy = strdup(cmd_line);
    if (!copy)
        return WARN_NO_CMDS;

    char *saveptr1;
    char *cmd = strtok_r(copy, "|", &saveptr1);

    while (cmd != NULL)
    {
        if (clist->num >= CMD_MAX)
        {
            free(copy);
            return ERR_TOO_MANY_COMMANDS;
        }

        cmd = trim(cmd);

        if (strlen(cmd) > 0)
        {
            char *cmd_copy = strdup(cmd);
            if (!cmd_copy)
            {
                free(copy);
                return WARN_NO_CMDS;
            }

            char *token = strtok(cmd_copy, " \t");
            if (token)
            {
                if (strlen(token) >= EXE_MAX)
                {
                    free(cmd_copy);
                    free(copy);
                    return ERR_CMD_OR_ARGS_TOO_BIG;
                }

                strcpy(clist->commands[clist->num].exe, token);

                token = strtok(NULL, "\0");
                if (token)
                {
                    token = trim(token);
                    if (strlen(token) >= ARG_MAX)
                    {
                        free(cmd_copy);
                        free(copy);
                        return ERR_CMD_OR_ARGS_TOO_BIG;
                    }
                    strcpy(clist->commands[clist->num].args, token);
                }

                clist->num++;
            }

            free(cmd_copy);
        }

        cmd = strtok_r(NULL, "|", &saveptr1);
    }

    free(copy);

    if (clist->num == 0)
    {
        return WARN_NO_CMDS;
    }

    return OK;
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

    cmd->argc = 0;
    char *token = trimmed;
    char *next_token = NULL;
    char quote_char = '\0';

    while (*token && cmd->argc < CMD_ARGV_MAX) {
        while (*token && isspace(*token)) token++;
        if (!*token) break;

        if (*token == '"' || *token == '\'') {
            quote_char = *token;
            token++;
            next_token = strchr(token, quote_char);
            if (!next_token) {
                next_token = token + strlen(token);
            }
            cmd->argv[cmd->argc++] = token;
            if (*next_token) {
                *next_token = '\0';
                token = next_token + 1;
            } else {
                token = next_token;
            }
        } else {
            cmd->argv[cmd->argc++] = token;
            next_token = strchr(token, ' ');
            if (next_token) {
                *next_token = '\0';
                token = next_token + 1;
            } else {
                token += strlen(token);
            }
        }
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
        printf("dsh2> ");
        
        if (fgets(input_buffer, ARG_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        input_buffer[strcspn(input_buffer, "\n")] = '\0';

        if (strcmp(input_buffer, EXIT_CMD) == 0) {
            exit(0);
        }

        if (strcmp(input_buffer, "dragon") == 0) {
            continue;
        }

        int parse_result = parse_command(input_buffer, &cmd);
        
        if (parse_result == WARN_NO_CMDS || cmd.argc == 0) {
            printf(CMD_WARN_NO_CMD);
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

    printf("cmd loop returned 0\n");
    return 0;
}