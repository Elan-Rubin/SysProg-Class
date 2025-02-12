#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 * Implement your main function by building a loop that prompts the
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.  Since we want fgets to also handle
 * end of file so we can run this headless for testing we need to check
 * the return code of fgets.  I have provided an example below of how
 * to do this assuming you are storing user input inside of the cmd_buff
 * variable.
 *
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 *
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 *
 *   Also, use the constants in the dshlib.h in this code.
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *
 *   Expected output:
 *
 *      CMD_OK_HEADER      if the command parses properly. You will
 *                         follow this by the command details
 *
 *      CMD_WARN_NO_CMD    if the user entered a blank command
 *      CMD_ERR_PIPE_LIMIT if the user entered too many commands using
 *                         the pipe feature, e.g., cmd1 | cmd2 | ... |
 *
 *  See the provided test cases for output expectations.
 */

void print_dragon_line(const char *encoded);

int main()
{
    // i used ai to generate this dragon art encoded from the drawing. the e is empty and f is filled
    const char *dragon_art[] = {
        "72e5f23e",
        "69e6f25e",
        "68e6f26e",
        "65e1f1e7f11e1f14e",
        "64e10f8e7f11e",
        "39e7f2e5f9e13f4e6f2e5f8e",
        "34e22f6e28f10e",
        "32e26f3e12f1e15f11e",
        "31e29f1e19f5e3f12e",
        "29e29f1e19f8e2f12e",
        "28e33f1e22f16e",
        "28e58f14e",
        "28e58f14e",
        "6e9f11e16f8e26f6e2f16e",
        "4e13f9e15f11e11f1e12f6e2f16e",
        "2e10f3e3f8e14f12e24f24e",
        "1e9f7e1f9e13f13e24f23e",
        "0e10f16e1f1e13f12e26f21e",
        "0e9f17e15f12e29f18e",
        "0e8f19e15f11e33f14e",
        "0e10f18e15f10e35f6e4f2e",
        "0e10f19e15f9e13f1e4f1e17f3e8f",
        "0e10f18e17f8e13f6e18f1e9f",
        "0e13f16e17f7e14f5e24f2e2f",
        "1e10f18e1f1e15f8e14f3e26f1e2f",
        "2e12f2e1f11e18f8e40f2e3f1e",
        "3e13f1e2f2e1f2e2f1e18f10e37f4e3f1e",
        "4e18f1e22f11e32f4e7f1e",
        "5e39f14e28f8e3f3e",
        "6e36f18e25f15e",
        "8e32f22e19f2e7f10e",
        "11e26f27e15f2e10f9e",
        "14e20f11e4f18e19f3e3f8e",
        "18e15f8e10f20e15f4e1f9e",
        "16e36f22e14f12e",
        "16e26f2e4f1e3f22e10f2e4f10e",
        "21e19f1e6f1e2f26e14f10e",
        "81e8f"};

    char *cmd_buff = (char *)malloc(SH_CMD_MAX * sizeof(char));

    int rc = 0;
    command_list_t clist;

    while (1)
    {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL)
        {
            printf("\n");
            break;
        }

        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        if (strcmp(cmd_buff, EXIT_CMD) == 0)
        {
            exit(0);
        }

        if (strcmp(cmd_buff, "dragon") == 0)
        {
            int drag_height = (sizeof(dragon_art) / sizeof(dragon_art[0]));
            for (int i = 0; i < drag_height; i++)
            {
                print_dragon_line(dragon_art[i]);
            }

            continue;
        }

        if (strlen(cmd_buff) == 0)
        {
            printf(CMD_WARN_NO_CMD);
            continue;
        }

        rc = build_cmd_list(cmd_buff, &clist);

        if (rc == OK)
        {
            printf(CMD_OK_HEADER, clist.num);

            for (int i = 0; i < clist.num; i++)
            {
                printf("<%d>%s", i + 1, clist.commands[i].exe);
                if (strlen(clist.commands[i].args) > 0)
                {
                    printf("[%s]", clist.commands[i].args);
                }
                printf("\n");
            }
        }
        else if (rc == WARN_NO_CMDS)
        {
            printf(CMD_WARN_NO_CMD);
        }
        else if (rc == ERR_TOO_MANY_COMMANDS)
        {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
        }
    }

    return 0;
}

void print_dragon_line(const char *encoded)
{
    int count = 0;
    int i = 0;

    while (encoded[i] != '\0')
    {
        count = 0;
        while (isdigit(encoded[i]))
        {
            count = count * 10 + (encoded[i] - '0');
            i++;
        }

        char c = encoded[i];
        for (int j = 0; j < count; j++)
        {
            if (c == 'e')
            {
                putchar(' ');
            }
            else if (c == 'f')
            {
                putchar('%');
            }
        }
        i++;
    }
    putchar('\n');
}