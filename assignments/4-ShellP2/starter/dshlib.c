#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    memset(cmd_buff, 0, sizeof(cmd_buff_t));
    
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }

    char *current_pos = cmd_line;
    int arg_count = 0;
    bool in_quotes = false;

    while (*current_pos == ' ') {
        current_pos++;
    }

    if (*current_pos == '\0') {
        free(cmd_buff->_cmd_buffer);
        return WARN_NO_CMDS;
    }

    cmd_buff->argv[arg_count] = current_pos;
    
    while (*current_pos != '\0') {
        if (*current_pos == '"') {
            in_quotes = !in_quotes;
            *current_pos = '\0';
            current_pos++;
            continue;
        }

        if (*current_pos == ' ' && !in_quotes) {
            *current_pos = '\0';
            current_pos++;
            
            while (*current_pos == ' ') {
                current_pos++;
            }
            
            if (*current_pos != '\0') {
                arg_count++;
                if (arg_count >= CMD_ARGV_MAX) {
                    return ERR_CMD_OR_ARGS_TOO_BIG;
                }
                cmd_buff->argv[arg_count] = current_pos;
            }
            continue;
        }
        
        current_pos++;
    }

    cmd_buff->argc = arg_count + 1;
    cmd_buff->argv[arg_count + 1] = NULL;
    
    return OK;
}

int exec_cmd(cmd_buff_t *cmd) {
    int child_pid = fork();
    
    if (child_pid < 0) {
        printf("Error: Could not create process\n");
        return ERR_MEMORY;
    }
    
    if (child_pid == 0) {
        execvp(cmd->argv[0], cmd->argv);
        printf("Error: Command not found: %s\n", cmd->argv[0]);
        exit(ERR_EXEC_CMD);
    }
    
    int status;
    waitpid(child_pid, &status, 0);
    return WEXITSTATUS(status);
}

int exec_local_cmd_loop() {
    char *input = malloc(SH_CMD_MAX);
    cmd_buff_t command;
    int result;

    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(input, ARG_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }

        if (strcmp(input, EXIT_CMD) == 0) {
            free(input);
            exit(OK);
        }

        if (strncmp(input, "cd", 2) == 0 && (input[2] == '\0' || input[2] == ' ')) {
            char *dir = strtok(input + 2, " ");
            if (dir == NULL) {
                continue;
            }
            
            while (*dir == ' ') dir++;
            
            if (*dir == '\0') {
                continue;
            }
            
            if (strtok(NULL, " ") != NULL) {
                printf("cd: too many arguments\n");
                continue;
            }
            
            if (chdir(dir) != 0) {
                printf("%s", CMD_ERR_EXECUTE);
            }
            continue;
        }

        result = build_cmd_buff(input, &command);
        
        if (result == OK) {
            exec_cmd(&command);
        }
        else if (result == WARN_NO_CMDS) {
            printf("%s", CMD_WARN_NO_CMD);
        }
        else {
            printf("%s", CMD_ERR_PIPE_LIMIT);
        }
    }

    free(input);
    return OK;
}
