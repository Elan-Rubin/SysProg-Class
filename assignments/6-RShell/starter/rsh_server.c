#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "dshlib.h"
#include "rshlib.h"

// Global variables for threaded mode
static int g_threaded_server = 0;
static int g_server_running = 1;
static pthread_mutex_t g_server_mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread argument structure
struct thread_arg {
    int client_socket;
};

// Signal handler for graceful shutdown
void handle_signal(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        pthread_mutex_lock(&g_server_mutex);
        g_server_running = 0;
        pthread_mutex_unlock(&g_server_mutex);
    }
}

// Set the threaded server flag
void set_threaded_server(int val) {
    g_threaded_server = val;
}

/*
 * Forward declarations of functions
 */
void *handle_client(void *arg);
int exec_client_requests(int cli_socket);
int send_message_string(int cli_socket, char *buff);
int send_message_eof(int cli_socket);
int rsh_execute_pipeline(int cli_sock, command_list_t *clist);
Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd);
Built_In_Cmds rsh_match_command(const char *input);

/*
 * start_server(ifaces, port, is_threaded)
 */
int start_server(char *ifaces, int port, int is_threaded) {
    int svr_socket;
    int rc;

    // Set up signal handling for graceful shutdown
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    
    // Set the threaded flag for extra credit
    set_threaded_server(is_threaded);
    
    // Initialize server state
    pthread_mutex_lock(&g_server_mutex);
    g_server_running = 1;
    pthread_mutex_unlock(&g_server_mutex);

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        int err_code = svr_socket;  // server socket will carry error code
        return err_code;
    }

    rc = process_cli_requests(svr_socket);

    stop_server(svr_socket);

    return rc;
}

/*
 * stop_server(svr_socket)
 */
int stop_server(int svr_socket) {
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 */
int boot_server(char *ifaces, int port) {
    int svr_socket;
    int ret;
    struct sockaddr_in addr;
    int enable = 1;

    // Create socket
    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }

    // Set socket option to reuse address
    ret = setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (ret < 0) {
        perror("setsockopt");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Set up the server address structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Bind the socket to the address
    ret = bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Listen for connections
    ret = listen(svr_socket, 20);
    if (ret == -1) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    return svr_socket;
}

/*
 * process_cli_requests(svr_socket) - Handle client connections
 */
int process_cli_requests(int svr_socket) {
    int cli_socket;
    int rc = OK;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;
    struct thread_arg *thread_arg;
    pthread_attr_t attr;
    int server_should_stop = 0;
    
    // For multi-threaded mode, initialize thread attributes
    if (g_threaded_server) {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }

    // Main server loop
    while (1) {
        // Check if server should stop
        pthread_mutex_lock(&g_server_mutex);
        server_should_stop = !g_server_running;
        pthread_mutex_unlock(&g_server_mutex);
        
        if (server_should_stop) {
            printf("Server shutdown requested\n");
            break;
        }
        
        // Set a timeout for accept to allow checking g_server_running periodically
        struct timeval timeout;
        timeout.tv_sec = 1;  // 1 second timeout
        timeout.tv_usec = 0;
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(svr_socket, &readfds);
        
        int select_result = select(svr_socket + 1, &readfds, NULL, NULL, &timeout);
        
        if (select_result < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted by signal, just retry
            }
            perror("select");
            return ERR_RDSH_COMMUNICATION;
        } else if (select_result == 0) {
            continue;  // Timeout, check g_server_running again
        }
        
        // Accept client connection if there's one available
        cli_socket = accept(svr_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (cli_socket < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted by signal, just retry
            }
            perror("accept");
            return ERR_RDSH_COMMUNICATION;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        if (g_threaded_server) {
            // Extra credit: Multi-threaded server
            thread_arg = malloc(sizeof(struct thread_arg));
            if (!thread_arg) {
                close(cli_socket);
                perror("malloc");
                continue;
            }
            
            thread_arg->client_socket = cli_socket;
            
            // Create a new thread to handle this client
            if (pthread_create(&tid, &attr, handle_client, thread_arg) != 0) {
                perror("pthread_create");
                free(thread_arg);
                close(cli_socket);
                continue;
            }
        } else {
            // Single-threaded server
            rc = exec_client_requests(cli_socket);
            
            // If client sent stop-server command, break out of the loop
            if (rc == OK_EXIT) {
                printf("%s", RCMD_MSG_SVR_STOP_REQ);
                break;
            }
        }
    }

    if (g_threaded_server) {
        pthread_attr_destroy(&attr);
    }

    return rc;
}

/*
 * Thread handler function for multi-threaded server (extra credit)
 */
void *handle_client(void *arg) {
    struct thread_arg *thread_arg = (struct thread_arg *)arg;
    int cli_socket = thread_arg->client_socket;
    int rc;
    
    free(thread_arg);  // Free the argument structure
    
    // Set thread name for debugging
    char thread_name[16];
    snprintf(thread_name, sizeof(thread_name), "client-%d", cli_socket);
    #ifdef _GNU_SOURCE
    pthread_setname_np(pthread_self(), thread_name);
    #endif
    
    rc = exec_client_requests(cli_socket);
    
    // If client sent stop-server command, notify the main thread
    if (rc == OK_EXIT) {
        printf("%s", RCMD_MSG_SVR_STOP_REQ);
        
        // Set the global server running flag to false
        pthread_mutex_lock(&g_server_mutex);
        g_server_running = 0;
        pthread_mutex_unlock(&g_server_mutex);
        
        // Signal the main process to shut down
        kill(getpid(), SIGTERM);
    }
    
    return NULL;
}

/*
 * exec_client_requests(cli_socket) - Handle client commands
 */
int exec_client_requests(int cli_socket) {
    int io_size;
    command_list_t cmd_list;
    int rc;
    int cmd_rc;
    char *io_buff;
    char temp_buff[RDSH_COMM_BUFF_SZ];
    int total_recv = 0;
    int is_complete = 0;

    // Allocate buffer for receiving commands
    io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (io_buff == NULL) {
        return ERR_RDSH_SERVER;
    }

    while (1) {
        // Reset buffer and command list
        memset(io_buff, 0, RDSH_COMM_BUFF_SZ);
        memset(&cmd_list, 0, sizeof(command_list_t));
        total_recv = 0;
        is_complete = 0;
        
        // Receive command from client - keep receiving until we get a null byte
        while (!is_complete && total_recv < RDSH_COMM_BUFF_SZ - 1) {
            memset(temp_buff, 0, RDSH_COMM_BUFF_SZ);
            io_size = recv(cli_socket, temp_buff, RDSH_COMM_BUFF_SZ - 1 - total_recv, 0);
            
            if (io_size < 0) {
                perror("recv");
                free(io_buff);
                close(cli_socket);
                return ERR_RDSH_COMMUNICATION;
            }
            
            if (io_size == 0) {
                // Client disconnected unexpectedly
                printf("Client disconnected unexpectedly\n");
                free(io_buff);
                close(cli_socket);
                return OK;
            }
            
            // Copy received data to our buffer
            memcpy(io_buff + total_recv, temp_buff, io_size);
            total_recv += io_size;
            
            // Check if we've received the null terminator
            for (int i = 0; i < io_size; i++) {
                if (temp_buff[i] == '\0') {
                    is_complete = 1;
                    break;
                }
            }
        }
        
        if (!is_complete) {
            printf("Error: Command too long or missing null terminator\n");
            send_message_string(cli_socket, "Error: Command too long or missing null terminator\n");
            continue;
        }
        
        // Log the command we're about to execute
        printf(RCMD_MSG_SVR_EXEC_REQ, io_buff);
        
        // Check for simple built-in commands before full parsing
        if (strcmp(io_buff, "exit") == 0) {
            printf("%s", RCMD_MSG_CLIENT_EXITED);
            send_message_string(cli_socket, "Goodbye!\n");
            free(io_buff);
            close(cli_socket);
            return OK;
        } else if (strcmp(io_buff, "stop-server") == 0) {
            send_message_string(cli_socket, "Stopping server...\n");
            free(io_buff);
            close(cli_socket);
            return OK_EXIT;
        }
        
        // Check for other built-in commands
        cmd_buff_t temp_cmd;
        if (alloc_cmd_buff(&temp_cmd) != OK) {
            send_message_string(cli_socket, "Error: Failed to allocate memory for command\n");
            continue;
        }
        
        build_cmd_buff(io_buff, &temp_cmd);
        Built_In_Cmds bi_cmd_type = rsh_built_in_cmd(&temp_cmd);
        
        if (bi_cmd_type == BI_CMD_EXIT) {
            // Check if this is actually the stop-server command
            if (strcmp(temp_cmd.argv[0], "stop-server") == 0) {
                free_cmd_buff(&temp_cmd);
                send_message_string(cli_socket, "Stopping server...\n");
                free(io_buff);
                close(cli_socket);
                return OK_EXIT;
            }
            
            free_cmd_buff(&temp_cmd);
            printf("%s", RCMD_MSG_CLIENT_EXITED);
            send_message_string(cli_socket, "Goodbye!\n");
            free(io_buff);
            close(cli_socket);
            return OK;
        } else if (bi_cmd_type == BI_EXECUTED) {
            // Built-in command was executed directly
            send_message_string(cli_socket, "Command executed.\n");
            free_cmd_buff(&temp_cmd);
            continue;
        }
        
        free_cmd_buff(&temp_cmd);
        
        // Parse command and build command list
        rc = build_cmd_list(io_buff, &cmd_list);
        
        if (rc == WARN_NO_CMDS) {
            send_message_string(cli_socket, CMD_WARN_NO_CMD);
            continue;
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            char error_msg[100];
            sprintf(error_msg, CMD_ERR_PIPE_LIMIT, CMD_MAX);
            send_message_string(cli_socket, error_msg);
            continue;
        } else if (rc != OK) {
            char error_msg[100];
            sprintf(error_msg, CMD_ERR_RDSH_ITRNL, rc);
            send_message_string(cli_socket, error_msg);
            continue;
        }
        
        // Execute the command pipeline
        cmd_rc = rsh_execute_pipeline(cli_socket, &cmd_list);
        printf(RCMD_MSG_SVR_RC_CMD, cmd_rc);
        
        // Send EOF to indicate end of command output
        send_message_eof(cli_socket);
        
        // Free command list
        free_cmd_list(&cmd_list);
    }

    free(io_buff);
    close(cli_socket);
    return OK;
}

/*
 * send_message_eof(cli_socket)
 */
int send_message_eof(int cli_socket) {
    int send_len = (int)sizeof(RDSH_EOF_CHAR);
    int sent_len;
    sent_len = send(cli_socket, &RDSH_EOF_CHAR, send_len, 0);

    if (sent_len != send_len) {
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

/*
 * send_message_string(cli_socket, char *buff)
 */
int send_message_string(int cli_socket, char *buff) {
    int send_len = strlen(buff);
    int sent_len;
    int remaining = send_len;
    int offset = 0;
    
    // Send the message in chunks if needed
    while (remaining > 0) {
        sent_len = send(cli_socket, buff + offset, remaining, 0);
        
        if (sent_len < 0) {
            perror("send");
            return ERR_RDSH_COMMUNICATION;
        }
        
        remaining -= sent_len;
        offset += sent_len;
    }
    
    // Send the EOF marker
    return send_message_eof(cli_socket);
}

/*
 * rsh_match_command(const char *input)
 */
Built_In_Cmds rsh_match_command(const char *input) {
    if (strcmp(input, "exit") == 0)
        return BI_CMD_EXIT;
    if (strcmp(input, "dragon") == 0)
        return BI_CMD_DRAGON;
    if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    if (strcmp(input, "stop-server") == 0)
        return BI_CMD_EXIT; // Use BI_CMD_EXIT as a placeholder
    return BI_NOT_BI;
}

/*
 * rsh_built_in_cmd(cmd_buff_t *cmd)
 */
Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd) {
    Built_In_Cmds ctype = BI_NOT_BI;
    ctype = rsh_match_command(cmd->argv[0]);

    switch (ctype) {
    case BI_CMD_DRAGON:
        // Uncomment if you have print_dragon function
        // print_dragon();
        return BI_EXECUTED;
    case BI_CMD_EXIT:
        // Check if this is actually the stop-server command
        if (strcmp(cmd->argv[0], "stop-server") == 0) {
            return BI_CMD_EXIT; // We'll handle differently in calling function
        }
        return BI_CMD_EXIT;
    case BI_CMD_CD:
        chdir(cmd->argv[1]);
        return BI_EXECUTED;
    default:
        return BI_NOT_BI;
    }
}

/*
 * rsh_execute_pipeline(int cli_sock, command_list_t *clist)
 */
int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    int pipes[CMD_MAX - 1][2];  // Array of pipes
    pid_t pids[CMD_MAX];
    int pids_st[CMD_MAX];       // Array to store process status
    Built_In_Cmds bi_cmd;
    int exit_code;

    // If no commands, return
    if (clist->num == 0) {
        return 0;
    }
    
    // Create all necessary pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    for (int i = 0; i < clist->num; i++) {
        // Check for built-in commands
        bi_cmd = rsh_built_in_cmd(&clist->commands[i]);
        
        if (bi_cmd == BI_CMD_EXIT) {
            // Check if this is actually the stop-server command
            if (strcmp(clist->commands[i].argv[0], "stop-server") == 0) {
                // Clean up pipes before returning
                for (int j = 0; j < clist->num - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                return STOP_SERVER_SC;
            }
            
            // Clean up pipes before returning
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return EXIT_SC;
        } else if (bi_cmd == BI_EXECUTED) {
            // Built-in command already executed, skip fork/exec
            pids[i] = -1;
            pids_st[i] = 0;
            continue;
        }

        // Create child process for this command
        pids[i] = fork();
        
        if (pids[i] < 0) {
            perror("fork");
            
            // Clean up pipes
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            return ERR_EXEC_CMD;
        } else if (pids[i] == 0) {
            // Child process
            
            // Set up stdin
            if (i == 0) {
                // First command: stdin comes from the socket
                dup2(cli_sock, STDIN_FILENO);
            } else {
                // Not first command: stdin comes from previous pipe
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            // Set up stdout
            if (i < clist->num - 1) {
                // Not last command: stdout goes to next pipe
                dup2(pipes[i][1], STDOUT_FILENO);
            } else {
                // Last command: stdout goes to the socket
                dup2(cli_sock, STDOUT_FILENO);
                dup2(cli_sock, STDERR_FILENO);
            }
            
            // Close all pipe file descriptors in the child
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

    // Parent process: close all pipe ends
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children that were actually forked
    for (int i = 0; i < clist->num; i++) {
        if (pids[i] > 0) {
            waitpid(pids[i], &pids_st[i], 0);
        }
    }

    // Get exit code of last command
    exit_code = WEXITSTATUS(pids_st[clist->num - 1]);
    
    // Check for special exit codes
    for (int i = 0; i < clist->num; i++) {
        if (pids[i] > 0 && WEXITSTATUS(pids_st[i]) == EXIT_SC) {
            exit_code = EXIT_SC;
        }
    }
    
    return exit_code;
}