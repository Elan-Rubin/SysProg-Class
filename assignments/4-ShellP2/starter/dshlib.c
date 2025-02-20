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

static int last_return_code = 0;

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    if (!cmd_line || !clist) return WARN_NO_CMDS;
    
    memset(clist, 0, sizeof(command_list_t));
    
    // Skip leading spaces
    while (isspace(*cmd_line)) cmd_line++;
    
    // Check for empty input
    if (!*cmd_line) return WARN_NO_CMDS;
    
    // Process input
    char *temp = strdup(cmd_line);
    if (!temp) return -1;
    
    char *curr = temp;
    char *start = curr;
    int in_quotes = 0;
    
    while (*curr) {
        if (*curr == '|' && !in_quotes) {
            *curr = '\0';
            if (clist->num >= CMD_MAX) {
                free(temp);
                return ERR_TOO_MANY_COMMANDS;
            }
            
            char *cmd_start = start;
            while (isspace(*cmd_start)) cmd_start++;
            
            char *space = strchr(cmd_start, ' ');
            if (space) {
                *space = '\0';
                strcpy(clist->commands[clist->num].exe, cmd_start);
                strcpy(clist->commands[clist->num].args, space + 1);
                char *args = clist->commands[clist->num].args;
                while (isspace(*args)) args++;
                memmove(clist->commands[clist->num].args, args, strlen(args) + 1);
            } else {
                strcpy(clist->commands[clist->num].exe, cmd_start);
            }
            clist->num++;
            start = curr + 1;
        } else if (*curr == '"') {
            in_quotes = !in_quotes;
        }
        curr++;
    }
    
    // Handle last command
    if (*start) {
        if (clist->num >= CMD_MAX) {
            free(temp);
            return ERR_TOO_MANY_COMMANDS;
        }
        
        char *cmd_start = start;
        while (isspace(*cmd_start)) cmd_start++;
        
        char *space = strchr(cmd_start, ' ');
        if (space) {
            *space = '\0';
            strcpy(clist->commands[clist->num].exe, cmd_start);
            strcpy(clist->commands[clist->num].args, space + 1);
            char *args = clist->commands[clist->num].args;
            while (isspace(*args)) args++;
            memmove(clist->commands[clist->num].args, args, strlen(args) + 1);
        } else {
            strcpy(clist->commands[clist->num].exe, cmd_start);
        }
        clist->num++;
    }
    
    free(temp);
    return clist->num > 0 ? OK : WARN_NO_CMDS;
}

int exec_local_cmd_loop(void) {
    char input_buffer[ARG_MAX];
    command_list_t clist;
    int rc;
    
    while (1) {
        printf("%s", SH_PROMPT);
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

        rc = build_cmd_list(input_buffer, &clist);

        if (rc == WARN_NO_CMDS) {
            printf(CMD_WARN_NO_CMD);
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
        } else if (rc == OK) {
            printf(CMD_OK_HEADER, clist.num);
            for (int i = 0; i < clist.num; i++) {
                printf("<%d>%s", i + 1, clist.commands[i].exe);
                if (strlen(clist.commands[i].args) > 0) {
                    printf("[%s]", clist.commands[i].args);
                }
                printf("\n");
            }
        }
    }

    return 0;
}