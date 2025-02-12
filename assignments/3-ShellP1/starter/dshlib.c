/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */


 #include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

static char* trim(char* str)
{
    while (isspace(*str)) str++;
    
    if (*str == 0) return str;
    
    char* end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    end[1] = '\0';
    
    return str;
}

int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    memset(clist, 0, sizeof(command_list_t));
    
    char *copy = strdup(cmd_line);
    if (!copy) return WARN_NO_CMDS;
    
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