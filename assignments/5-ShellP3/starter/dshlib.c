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
    char *p = NULL;
    int i;
    int start_token = 0;
    int start_quote = 0;

    for (i = 0, p = cmd_line; *p != '\0'; p++) {
        if (*p == '"') {
            if (start_quote) {
                start_quote = 0;
                start_token = 0;
            } else {
                start_quote = 1;
            }
            *p = '\0';
        } else if (*p == SPACE_CHAR && !start_quote) {
            start_token = 0;
            *p = '\0';
        } else if (*p == PIPE_CHAR && !start_quote) {
            *p = '\0';
            p++;
            break;
        } else if (!start_token) {
            start_token = 1;
            cmd_buff->argv[i] = p;
            i++;
        }
    }
    cmd_buff->argc = i;
    cmd_buff->_cmd_buffer = cmd_line;

    return p - cmd_line;
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    cmd_buff_t *cmd_buf = clist->commands;
    int rc = OK;
    int i = 0;
    int num_cmds = 0;

    while (*cmd_line != '\0' && rc == OK) {
        if (num_cmds == CMD_MAX) {
            rc = ERR_TOO_MANY_COMMANDS;
            break;
        }

        i = build_cmd_buff(cmd_line, cmd_buf);
        cmd_buf++;
        num_cmds++;
        cmd_line += i;
    }

    if (num_cmds == 0) {
        rc = WARN_NO_CMDS;
    }

    clist->num = num_cmds;

    return rc;
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

Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    } else if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    } else if (strcmp(input, "dragon") == 0) {
        return BI_CMD_DRAGON;
    } else {
        return BI_NOT_BI;
    }
}

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    Built_In_Cmds bi_cmd;
    bi_cmd = match_command(cmd->argv[0]);
    int rc;

    switch (bi_cmd) {
    case BI_CMD_EXIT:
        printf("exiting...\n");
        return BI_CMD_EXIT;
    case BI_CMD_CD:
        rc = chdir(cmd->argv[1]);
        if (rc != 0) {
            return BI_CMD_CD;
        }
        return BI_EXECUTED;
    case BI_NOT_BI:
        return BI_NOT_BI;
    case BI_CMD_DRAGON:
        return BI_CMD_DRAGON;
    default:
        fprintf(stderr, "Fatal error");
        exit(EXIT_FAILURE);
    }
}

int execute_pipeline(command_list_t *clist) {
    int pipes[clist->num - 1][2];
    pid_t pids[clist->num];

    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            return ERR_EXEC_CMD;
        }

        if (pids[i] == 0) {
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            if (i < clist->num - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            exit(ERR_EXEC_CMD);
        }
    }

    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], NULL, 0);
    }

    return OK;
}

int exec_local_cmd_loop() {
    char *cmd_input;
    int rc = 0;
    command_list_t cmd_list = {0};
    cmd_buff_t *cmd_buff = cmd_list.commands;
    Built_In_Cmds bi_cmd;

    cmd_input = malloc(SH_CMD_MAX);
    if (cmd_input == NULL) {
        perror("malloc:");
        return ERR_MEMORY;
    }
    memset(cmd_input, 0, SH_CMD_MAX);

    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_input, ARG_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        cmd_input[strcspn(cmd_input, "\n")] = '\0';

        rc = build_cmd_list(cmd_input, &cmd_list);
        if (rc == WARN_NO_CMDS) {
            printf(CMD_WARN_NO_CMD);
            continue;
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        }

        if (cmd_list.num == 1) {
            bi_cmd = exec_built_in_cmd(cmd_buff);
            if (bi_cmd == BI_CMD_EXIT) {
                free(cmd_input);
                return OK;
            } else if (bi_cmd == BI_NOT_BI) {
                rc = exec_cmd(cmd_buff);
                if (rc == ERR_EXEC_CMD) {
                    printf("Error executing command %s\n", cmd_buff->argv[0]);
                }
            }
        } else {
            rc = execute_pipeline(&cmd_list);
        }
    }

    free(cmd_input);
    return OK;
}