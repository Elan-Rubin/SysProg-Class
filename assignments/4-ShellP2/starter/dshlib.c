#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"


// NEED TO IMPLEMENT THE built-in AND extern command stuff. More test cases in student_tests.sh

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    // Initialize clist with 0 using memset().
    memset(cmd_buff, 0, sizeof(cmd_buff_t));

    // Store the copy of the cmd buffer into _cmd_buff.
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (!cmd_buff->_cmd_buffer) 
    {
        return ERR_MEMORY;
    }

    bool inside_quotes = false;
    int arg_count = 0;

    while (*cmd_line) 
    {
        // Skip leading spaces until we hit a quote.
        while (*cmd_line == SPACE_CHAR && !inside_quotes)
        {
            cmd_line++;
        }

        // If we reach null terminator, and if arg_count is 0, then only spaces were given in the command.
        if (*cmd_line == '\0')
        {
            if (arg_count > 0)
            {
                break;
            } else
            {
                free(cmd_buff->_cmd_buffer);
                return WARN_NO_CMDS;    
            }
            
        }

        if (*cmd_line == QUOTES)
        {
            inside_quotes = !inside_quotes;
            cmd_line++;
        }

        // Store the pointer to the first char of the argument.
        cmd_buff->argv[arg_count] = cmd_line;
        arg_count++;

        // If not inside quotes, we stop iterating once we hit a space.
        // If inside quotes, we won't stop on space, but stop once we hit the second quotes.
        while (*cmd_line && (*cmd_line != SPACE_CHAR || inside_quotes))
        {
            if (*cmd_line == QUOTES)
            {
                inside_quotes = !inside_quotes;
                *cmd_line = '\0';
            }
            cmd_line++;
        }
        
        // Null terminate the argument.
        if (*cmd_line == SPACE_CHAR)
        {
            *cmd_line = '\0';
            cmd_line++;
        }

        // Terminate the program is arg_count or arg length is max.
        if (arg_count > 0 && (strlen(cmd_buff->argv[arg_count - 1]) >= ARG_MAX || arg_count >= CMD_ARGV_MAX))
        {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }        
    }

    cmd_buff->argc = arg_count;
    cmd_buff->argv[arg_count] = NULL;

    return OK;
}


int exec_cmd(cmd_buff_t *cmd)
{
    int pid = fork();

    if (pid < 0)
    {
        printf("fork failed\n");
        return ERR_MEMORY;
    }
    else if (pid == 0)
    {
        execvp(cmd->argv[0], cmd->argv);
        printf("command not found: %s\n", cmd->argv[0]);
        exit(ERR_EXEC_CMD);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
}


/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 *  
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */

// TODO IMPLEMENT MAIN LOOP

// TODO IMPLEMENT parsing input to cmd_buff_t *cmd_buff

// TODO IMPLEMENT if built-in command, execute builtin logic for exit, cd (extra credit: dragon)
// the cd command should chdir to the provided directory; if no directory is provided, do nothing

// TODO IMPLEMENT if not built-in command, fork/exec as an external command
// for example, if the user input is "ls -l", you would fork/exec the command "ls" with the arg "-l"

int exec_local_cmd_loop()
{
    char *cmd_buff = malloc(SH_CMD_MAX);
    int rc = 0;
    cmd_buff_t cmd;

    while (1)
    {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL)
        {
            printf("\n");
            break;
        }
        // remove the trailing \n from cmd_buff
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // IMPLEMENT THE REST OF THE REQUIREMENTS

        // If not user command given, continue the while loop.
        if (strlen(cmd_buff) == 0)
        {
            printf("%s", CMD_WARN_NO_CMD);
            continue;

        } else if (strcmp(cmd_buff, EXIT_CMD) == 0)     // If exit is given as the command, then exit the program.
        {
            free(cmd_buff);
            exit(OK);
        }

        if (strncmp(cmd_buff, "cd", 2) == 0 && (cmd_buff[2] == '\0' || cmd_buff[2] == ' '))
        {
            strtok(cmd_buff, " ");
            char* arg = strtok(NULL, " ");

            // do nothing if no args passed for cd.
            if (arg == NULL)
            {
                continue;
            } else
            {
                // cd can only handle one argument. So, check if there is more than one argument.
                char* extraArgs = strtok(NULL, " ");

                if (extraArgs != NULL)
                {
                    printf("cd: error too many arguments!");
                    return ERR_TOO_MANY_COMMANDS;
                }
                
                if (chdir(arg) != 0)
                {
                    printf(CMD_ERR_EXECUTE);
                    continue;
                }
            }

            continue;
        }

        rc = build_cmd_buff(cmd_buff, &cmd);

        if (rc == OK)
        {
            exec_cmd(&cmd);
            
        } else if (rc == WARN_NO_CMDS)
        {
            printf(CMD_WARN_NO_CMD);

        } else
        {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
        }
    }

    free(cmd_buff);
    return OK;
}
