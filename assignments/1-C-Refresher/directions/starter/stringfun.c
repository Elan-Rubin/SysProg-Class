#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SZ 50

// prototypes
void usage(char *);
void print_buff(char *, int);
int setup_buff(char *, char *, int);

// prototypes for functions to handle required functionality
int count_words(char *, int, int);
// add additional prototypes here
int print_words(char *, int, int);
int reverse_str(char *, int, int);

int setup_buff(char *buff, char *user_str, int len)
{
    // TODO: #4:  Implement the setup buff as per the directions

    char *src = user_str;
    char *dst = buff;
    int count = 0;
    int space_flag = 0;

    while (*src != '\0')
    {
        if (count >= len)
        {
            return -1;
        }
        if (*src == ' ')
        {
            if (!space_flag && count > 0)
            {
                *dst = ' ';
                dst++;

                count++;
                space_flag = 1;
            }
        }
        else
        {
            *dst= *src;
            dst++;
            space_flag = 0;
        }
        src++;
    }

    while(count < len){
        *dst = '.';
        dst++;
        count++;
    }

    //return 0; // for now just so the code compiles.
    return count;
}

void print_buff(char *buff, int len)
{
    printf("Buffer:  ");
    for (int i = 0; i < len; i++)
    {
        putchar(*(buff + i));
    }
    putchar('\n');
}

void usage(char *exename)
{
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);
}

int count_words(char *buff, int len, int str_len)
{
    int count = 0;
    int within = 0;
    char *ptr = buff;
    while (ptr < buff + str_len)
    {
        if (*ptr != ' ' && !within)
        {
            within = 1;
            count++;
        }
        else if (*ptr == ' ')
        {
            within = 0;
        }
        ptr++;
    }

    return count;
}

int reverse_string(char *buff, int len, int str_len){

}

int print_words(char *buff, int len, int str_len){

}

// ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS

int main(int argc, char *argv[])
{

    char *buff;         // placehoder for the internal buffer
    char *input_string; // holds the string provided by the user on cmd line
    char opt;           // used to capture user option from cmd line
    int rc;             // used for return codes
    int user_str_len;   // length of user supplied string

    // TODO:  #1. WHY IS THIS SAFE, aka what if argv[1] does not exist?
    //  This is safe because the if condition checks if argc < 2
    //  before attempting to access argv[1]. If argv[1] doesn't exist, the first
    //  condition will fail and prevent accessing invalid memory.

    if ((argc < 2) || (*argv[1] != '-'))
    {
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1] + 1); // get the option flag

    // handle the help flag and then exit normally
    if (opt == 'h')
    {
        usage(argv[0]);
        exit(0);
    }

    // WE NOW WILL HANDLE THE REQUIRED OPERATIONS

    // TODO:  #2 Document the purpose of the if statement below
    //  This check ensures there's a string argument provided
    //  after the option flag. Without this, commands like "./stringfun -c"
    //  would try to access argv[2] which doesn't exist.
    if (argc < 3)
    {
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; // capture the user input string

    // TODO:  #3 Allocate space for the buffer using malloc and
    //           handle error if malloc fails by exiting with a
    //           return code of 99
    //  CODE GOES HERE FOR #3

    buff = (char *)malloc(BUFFER_SZ);
    if (buff == NULL)
    {
        printf("Failed allocation");
        exit(99);
    }

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ); // see todos
    if (user_str_len < 0)
    {
        printf("Error setting up buffer, error = %d", user_str_len);
        exit(2);
    }

    switch (opt)
    {
    case 'c':
        rc = count_words(buff, BUFFER_SZ, user_str_len); // you need to implement
        if (rc < 0)
        {
            printf("Error counting words, rc = %d", rc);
            exit(2);
        }
        printf("Word Count: %d\n", rc);
        break;

    case 'r':
    rc = count_words(buff, BUFFER_SZ, user_str_len); // you need to implement
        if (rc < 0)
        {
            printf("Error reversing, rc = %d", rc);
            exit(2);
        }
        break;

    case 'w':
        rc = count_words(buff, BUFFER_SZ, user_str_len); // you need to implement
        if (rc < 0)
        {
            printf("Error printing, rc = %d", rc);
            exit(2);
        }
        break;

    // TODO:  #5 Implement the other cases for 'r' and 'w' by extending
    //        the case statement options
    default:
        usage(argv[0]);
        exit(1);
    }

    // TODO:  #6 Dont forget to free your buffer before exiting
    print_buff(buff, BUFFER_SZ);
    exit(0);
}

// TODO:  #7  Notice all of the helper functions provided in the
//           starter take both the buffer as well as the length.  Why
//           do you think providing both the pointer and the length
//           is a good practice, after all we know from main() that
//           the buff variable will have exactly 50 bytes?
//
//           PLACE YOUR ANSWER HERE