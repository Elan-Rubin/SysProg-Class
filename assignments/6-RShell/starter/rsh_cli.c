#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>

#include "dshlib.h"
#include "rshlib.h"

/*
 * exec_remote_cmd_loop(server_ip, port)
 *      server_ip:  a string in ip address format, indicating the servers IP
 *                  address.  Note 127.0.0.1 is the default meaning the server
 *                  is running on the same machine as the client
 *              
 *      port:   The port the server will use.  Note the constant 
 *              RDSH_DEF_PORT which is 1234 in rshlib.h.
 */
int exec_remote_cmd_loop(char *address, int port)
{
    char *cmd_buff;
    char *rsp_buff;
    int cli_socket = -1;  // Initialize to invalid socket
    ssize_t io_size;
    int is_eof;
    
    // Allocate buffers for sending and receiving data
    cmd_buff = malloc(RDSH_COMM_BUFF_SZ);
    rsp_buff = malloc(RDSH_COMM_BUFF_SZ);
    
    if (cmd_buff == NULL || rsp_buff == NULL) {
        return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_MEMORY);
    }

    // Connect to the server
    cli_socket = start_client(address, port);
    if (cli_socket < 0) {
        perror("start client");
        return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_CLIENT);
    }

    while (1) {
        // Print prompt and get user input
        printf("%s", SH_PROMPT);
        fflush(stdout);
        
        if (fgets(cmd_buff, RDSH_COMM_BUFF_SZ, stdin) == NULL) {
            printf("\n");
            break;
        }
        
        // Remove the trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';
        
        // Skip if empty command
        if (strlen(cmd_buff) == 0) {
            continue;
        }
        
        // Send command to server (including null termination)
        io_size = send(cli_socket, cmd_buff, strlen(cmd_buff) + 1, 0);
        if (io_size < 0) {
            perror("send");
            return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
        }
        
        // Receive and display response from server
        while (1) {
            memset(rsp_buff, 0, RDSH_COMM_BUFF_SZ);
            io_size = recv(cli_socket, rsp_buff, RDSH_COMM_BUFF_SZ - 1, 0);
            
            if (io_size < 0) {
                perror("recv");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }
            
            if (io_size == 0) {
                printf("%s", RCMD_SERVER_EXITED);
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }
            
            // Check if this is the last chunk (ends with EOF character)
            is_eof = (rsp_buff[io_size - 1] == RDSH_EOF_CHAR);
            
            // Print received data
            if (is_eof) {
                // Don't print the EOF character
                printf("%.*s", (int)io_size - 1, rsp_buff);
            } else {
                printf("%.*s", (int)io_size, rsp_buff);
            }
            
            // Break if this is the last chunk
            if (is_eof) {
                break;
            }
        }
        
        // Check if the command was 'exit' or 'stop-server'
        if (strcmp(cmd_buff, "exit") == 0 || strcmp(cmd_buff, "stop-server") == 0) {
            break;
        }
    }

    return client_cleanup(cli_socket, cmd_buff, rsp_buff, OK);
}

/*
 * start_client(server_ip, port)
 *      server_ip:  a string in ip address format, indicating the servers IP
 *                  address.  Note 127.0.0.1 is the default meaning the server
 *                  is running on the same machine as the client
 *              
 *      port:   The port the server will use.
 */
int start_client(char *server_ip, int port) {
    struct sockaddr_in addr;
    int cli_socket;
    int ret;

    // Create socket
    cli_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cli_socket < 0) {
        perror("socket");
        return ERR_RDSH_CLIENT;
    }

    // Set up the server address structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    // Connect to the server
    ret = connect(cli_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("connect");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    return cli_socket;
}

/*
 * client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc)
 *      cli_socket:   The client socket
 *      cmd_buff:     The buffer that will hold commands to send to server
 *      rsp_buff:     The buffer that will hld server responses
 */
int client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc) {
    // If a valid socket number close it
    if (cli_socket > 0) {
        close(cli_socket);
    }

    // Free up the buffers 
    free(cmd_buff);
    free(rsp_buff);

    // Echo the return value that was passed as a parameter
    return rc;
}