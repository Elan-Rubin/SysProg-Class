#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"

/*
 * Implementation of the helper functions needed for command parsing
 */

int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX);
    if (cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }
    
    memset(cmd_buff->_cmd_buffer, 0, SH_CMD_MAX);
    cmd_buff->argc = 0;
    
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer != NULL) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    
    return OK;
}

int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer != NULL) {
        memset(cmd_buff->_cmd_buffer, 0, SH_CMD_MAX);
    }
    
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    return OK;
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    char *token;
    int i = 0;
    
    if (cmd_line == NULL || cmd_buff == NULL) {
        return ERR_MEMORY;
    }
    
    // Copy command line to buffer
    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';
    
    // Parse command and arguments
    token = strtok(cmd_buff->_cmd_buffer, " \t\n");
    while (token != NULL && i < CMD_ARGV_MAX - 1) {
        cmd_buff->argv[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    
    cmd_buff->argv[i] = NULL;
    cmd_buff->argc = i;
    
    return OK;
}

int close_cmd_buff(cmd_buff_t *cmd_buff) {
    return free_cmd_buff(cmd_buff);
}

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    char *cmd_start;
    char *pipe_loc;
    char cmd_segment[SH_CMD_MAX];
    int cmd_idx = 0;
    
    if (cmd_line == NULL || clist == NULL) {
        return ERR_MEMORY;
    }
    
    // Skip leading spaces
    cmd_start = cmd_line;
    while (*cmd_start && isspace(*cmd_start)) {
        cmd_start++;
    }
    
    // Check if command line is empty
    if (*cmd_start == '\0') {
        return WARN_NO_CMDS;
    }
    
    // Initialize command list
    clist->num = 0;
    
    // Process each command in the pipeline
    while (*cmd_start != '\0') {
        // Find pipe or end of string
        pipe_loc = strchr(cmd_start, PIPE_CHAR);
        
        if (pipe_loc == NULL) {
            // Last or only command
            strncpy(cmd_segment, cmd_start, SH_CMD_MAX - 1);
            cmd_segment[SH_CMD_MAX - 1] = '\0';
            cmd_start += strlen(cmd_start);
        } else {
            // Command followed by pipe
            int segment_len = pipe_loc - cmd_start;
            if (segment_len >= SH_CMD_MAX) {
                segment_len = SH_CMD_MAX - 1;
            }
            
            strncpy(cmd_segment, cmd_start, segment_len);
            cmd_segment[segment_len] = '\0';
            cmd_start = pipe_loc + 1;
            
            // Skip spaces after the pipe
            while (*cmd_start && isspace(*cmd_start)) {
                cmd_start++;
            }
        }
        
        // Check if we've reached the maximum number of commands
        if (cmd_idx >= CMD_MAX) {
            return ERR_TOO_MANY_COMMANDS;
        }
        
        // Allocate and build command buffer
        alloc_cmd_buff(&clist->commands[cmd_idx]);
        build_cmd_buff(cmd_segment, &clist->commands[cmd_idx]);
        
        cmd_idx++;
    }
    
    clist->num = cmd_idx;
    
    return OK;
}

int free_cmd_list(command_list_t *cmd_lst) {
    if (cmd_lst == NULL) {
        return ERR_MEMORY;
    }
    
    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&cmd_lst->commands[i]);
    }
    
    return OK;
}

/*
 * Built-in command functions
 */

void print_dragon() {
    printf("                ______________\n");
    printf("               /             /|\n");
    printf("              /             / |\n");
    printf("             /____________ /  |\n");
    printf("            | ___________ |   |\n");
    printf("            ||           ||   |\n");
    printf("            ||           ||   |\n");
    printf("            ||           ||   |\n");
    printf("            ||___________||   |\n");
    printf("            |   _______   |  /\n");
    printf("           /|  (_______)  | /\n");
    printf("          ( |_____________|/\n");
    printf("           \\\n");
    printf(" ___________)      _(_______________\n");
    printf("(              )   (                )\n");
    printf("(_____________)    (_________________)\n\n");
}

Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, "exit") == 0)
        return BI_CMD_EXIT;
    else if (strcmp(input, "dragon") == 0)
        return BI_CMD_DRAGON;
    else if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    
    return BI_NOT_BI;
}

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    Built_In_Cmds ctype = BI_NOT_BI;
    
    if (cmd == NULL || cmd->argc == 0 || cmd->argv[0] == NULL) {
        return BI_NOT_BI;
    }
    
    ctype = match_command(cmd->argv[0]);
    
    switch (ctype) {
        case BI_CMD_DRAGON:
            print_dragon();
            return BI_EXECUTED;
        case BI_CMD_CD:
            if (cmd->argc < 2) {
                printf("cd: missing argument\n");
            } else if (chdir(cmd->argv[1]) != 0) {
                perror("cd");
            }
            return BI_EXECUTED;
        case BI_CMD_EXIT:
            return BI_CMD_EXIT;
        default:
            return BI_NOT_BI;
    }
}

/*
 * execute_pipeline - Execute a pipeline of commands
 */
int execute_pipeline(command_list_t *clist) {
    int pipes[CMD_MAX - 1][2];
    pid_t pids[CMD_MAX];
    int status[CMD_MAX];
    int exit_code = 0;
    Built_In_Cmds bi_cmd;

    // Create necessary pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    // Process each command in the pipeline
    for (int i = 0; i < clist->num; i++) {
        // Check for built-in commands
        bi_cmd = exec_built_in_cmd(&clist->commands[i]);
        
        if (bi_cmd == BI_CMD_EXIT) {
            // Handle exit command
            return EXIT_SC;
        } else if (bi_cmd == BI_EXECUTED) {
            // Built-in command already executed
            pids[i] = -1;  // Mark as not a real process
            continue;
        }

        // Fork a child process for this command
        pids[i] = fork();
        
        if (pids[i] < 0) {
            perror("fork");
            return ERR_EXEC_CMD;
        } else if (pids[i] == 0) {
            // Child process
            
            // Set up stdin
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            // Set up stdout
            if (i < clist->num - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe fds
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            
            // If execvp fails
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process
    
    // Close all pipe fds in parent
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all child processes
    for (int i = 0; i < clist->num; i++) {
        if (pids[i] > 0) {
            waitpid(pids[i], &status[i], 0);
            
            // Get exit status of the last command in the pipeline
            if (i == clist->num - 1) {
                exit_code = WEXITSTATUS(status[i]);
            }
        }
    }
    
    return exit_code;
}

/*
 * exec_local_cmd_loop - Main shell loop for local execution
 */
int exec_local_cmd_loop() {
    char cmd_buff[SH_CMD_MAX];
    command_list_t cmd_list;
    int rc;
    
    while (1) {
        // Print prompt
        printf("%s", SH_PROMPT);
        fflush(stdout);
        
        // Get user input
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        
        // Remove trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';
        
        // Skip empty commands
        if (strlen(cmd_buff) == 0) {
            continue;
        }
        
        // Exit command
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            break;
        }
        
        // Parse command line
        rc = build_cmd_list(cmd_buff, &cmd_list);
        
        if (rc == WARN_NO_CMDS) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        } else if (rc != OK) {
            printf("Error parsing command: %d\n", rc);
            continue;
        }
        
        // Execute the command pipeline
        rc = execute_pipeline(&cmd_list);
        
        // Handle exit command
        if (rc == EXIT_SC) {
            free_cmd_list(&cmd_list);
            break;
        }
        
        // Free command list
        free_cmd_list(&cmd_list);
    }
    
    return OK;
}