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
#include <ctype.h>

#include "dshlib.h"
#include "rshlib.h"

static int g_threaded_server = 0;
static int g_server_running = 1;
static pthread_mutex_t g_server_mutex = PTHREAD_MUTEX_INITIALIZER;

struct thread_arg {
    int client_socket;
};

void handle_signal(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        pthread_mutex_lock(&g_server_mutex);
        g_server_running = 0;
        pthread_mutex_unlock(&g_server_mutex);
    }
}

void set_threaded_server(int val) {
    g_threaded_server = val;
}

void *handle_client(void *arg);
int exec_client_requests(int cli_socket);
int send_message_string(int cli_socket, char *buff);
int send_message_eof(int cli_socket);
int rsh_execute_pipeline(int cli_sock, command_list_t *clist);
Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd);
Built_In_Cmds rsh_match_command(const char *input);

int start_server(char *ifaces, int port, int is_threaded) {
    int svr_socket;
    int rc;

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    
    set_threaded_server(is_threaded);
    
    pthread_mutex_lock(&g_server_mutex);
    g_server_running = 1;
    pthread_mutex_unlock(&g_server_mutex);

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        int err_code = svr_socket;
        return err_code;
    }

    rc = process_cli_requests(svr_socket);

    stop_server(svr_socket);

    return rc;
}

int stop_server(int svr_socket) {
    return close(svr_socket);
}

int boot_server(char *ifaces, int port) {
    int svr_socket;
    int ret;
    struct sockaddr_in addr;
    int enable = 1;

    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }

    ret = setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (ret < 0) {
        perror("setsockopt");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    ret = bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    ret = listen(svr_socket, 20);
    if (ret == -1) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    return svr_socket;
}

int process_cli_requests(int svr_socket) {
    int cli_socket;
    int rc = OK;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;
    struct thread_arg *thread_arg;
    pthread_attr_t attr;
    int server_should_stop = 0;
    
    if (g_threaded_server) {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }

    while (1) {
        pthread_mutex_lock(&g_server_mutex);
        server_should_stop = !g_server_running;
        pthread_mutex_unlock(&g_server_mutex);
        
        if (server_should_stop) {
            printf("Server shutdown requested\n");
            break;
        }
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(svr_socket, &readfds);
        
        int select_result = select(svr_socket + 1, &readfds, NULL, NULL, &timeout);
        
        if (select_result < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            return ERR_RDSH_COMMUNICATION;
        } else if (select_result == 0) {
            continue;
        }
        
        cli_socket = accept(svr_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (cli_socket < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            return ERR_RDSH_COMMUNICATION;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        if (g_threaded_server) {
            thread_arg = malloc(sizeof(struct thread_arg));
            if (!thread_arg) {
                close(cli_socket);
                perror("malloc");
                continue;
            }
            
            thread_arg->client_socket = cli_socket;
            
            if (pthread_create(&tid, &attr, handle_client, thread_arg) != 0) {
                perror("pthread_create");
                free(thread_arg);
                close(cli_socket);
                continue;
            }
        } else {
            rc = exec_client_requests(cli_socket);
            close(cli_socket);
            
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

void *handle_client(void *arg) {
    struct thread_arg *thread_arg = (struct thread_arg *)arg;
    int cli_socket = thread_arg->client_socket;
    int rc;
    
    free(thread_arg);
    
    char thread_name[16];
    snprintf(thread_name, sizeof(thread_name), "client-%d", cli_socket);
    #ifdef _GNU_SOURCE
    pthread_setname_np(pthread_self(), thread_name);
    #endif
    
    rc = exec_client_requests(cli_socket);
    close(cli_socket);
    
    if (rc == OK_EXIT) {
        printf("%s", RCMD_MSG_SVR_STOP_REQ);
        
        pthread_mutex_lock(&g_server_mutex);
        g_server_running = 0;
        pthread_mutex_unlock(&g_server_mutex);
        
        kill(getpid(), SIGTERM);
    }
    
    return NULL;
}

int exec_client_requests(int cli_socket) {
    int io_size;
    command_list_t cmd_list;
    int rc;
    int cmd_rc;
    char *io_buff;
    char temp_buff[RDSH_COMM_BUFF_SZ];
    int total_recv = 0;
    int is_complete = 0;

    io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (io_buff == NULL) {
        return ERR_RDSH_SERVER;
    }

    while (1) {
        memset(io_buff, 0, RDSH_COMM_BUFF_SZ);
        memset(&cmd_list, 0, sizeof(command_list_t));
        total_recv = 0;
        is_complete = 0;
        
        while (!is_complete && total_recv < RDSH_COMM_BUFF_SZ - 1) {
            memset(temp_buff, 0, RDSH_COMM_BUFF_SZ);
            io_size = recv(cli_socket, temp_buff, RDSH_COMM_BUFF_SZ - 1 - total_recv, 0);
            
            if (io_size < 0) {
                perror("recv");
                free(io_buff);
                return ERR_RDSH_COMMUNICATION;
            }
            
            if (io_size == 0) {
                printf("Client disconnected unexpectedly\n");
                free(io_buff);
                return OK;
            }
            
            memcpy(io_buff + total_recv, temp_buff, io_size);
            total_recv += io_size;
            
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
        
        printf(RCMD_MSG_SVR_EXEC_REQ, io_buff);
        
        int end = total_recv - 1;
        while (end > 0 && isspace(io_buff[end-1])) {
            io_buff[--end] = '\0';
        }
        
        if (strlen(io_buff) == 0) {
            send_message_eof(cli_socket);
            continue;
        }
        
        if (strcmp(io_buff, "exit") == 0) {
            printf("%s", RCMD_MSG_CLIENT_EXITED);
            send_message_string(cli_socket, "Goodbye!\n");
            free(io_buff);
            return OK;
        } else if (strcmp(io_buff, "stop-server") == 0) {
            send_message_string(cli_socket, "Stopping server...\n");
            free(io_buff);
            return OK_EXIT;
        }
        
        cmd_buff_t temp_cmd;
        if (alloc_cmd_buff(&temp_cmd) != OK) {
            send_message_string(cli_socket, "Error: Failed to allocate memory for command\n");
            continue;
        }
        
        build_cmd_buff(io_buff, &temp_cmd);
        Built_In_Cmds bi_cmd_type = rsh_built_in_cmd(&temp_cmd);
        
        if (bi_cmd_type == BI_CMD_EXIT) {
            if (strcmp(temp_cmd.argv[0], "stop-server") == 0) {
                free_cmd_buff(&temp_cmd);
                send_message_string(cli_socket, "Stopping server...\n");
                free(io_buff);
                return OK_EXIT;
            }
            
            free_cmd_buff(&temp_cmd);
            printf("%s", RCMD_MSG_CLIENT_EXITED);
            send_message_string(cli_socket, "Goodbye!\n");
            free(io_buff);
            return OK;
        } else if (bi_cmd_type == BI_EXECUTED) {
            if (strcmp(temp_cmd.argv[0], "cd") == 0) {
                if (temp_cmd.argc < 2) {
                    send_message_string(cli_socket, "cd: missing argument\n");
                } else if (chdir(temp_cmd.argv[1]) != 0) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), "cd: %s: %s\n", 
                             temp_cmd.argv[1], strerror(errno));
                    send_message_string(cli_socket, error_msg);
                } else {
                    send_message_eof(cli_socket);
                }
            } else if (strcmp(temp_cmd.argv[0], "dragon") == 0) {
                int pipefd[2];
                if (pipe(pipefd) == -1) {
                    perror("pipe");
                    send_message_string(cli_socket, "Error creating pipe for dragon command\n");
                    free_cmd_buff(&temp_cmd);
                    continue;
                }
                
                int saved_stdout = dup(STDOUT_FILENO);
                dup2(pipefd[1], STDOUT_FILENO);
                
                print_dragon();
                fflush(stdout);
                
                dup2(saved_stdout, STDOUT_FILENO);
                close(saved_stdout);
                close(pipefd[1]);
                
                char dragon_output[4096];
                ssize_t bytes_read = read(pipefd[0], dragon_output, sizeof(dragon_output) - 1);
                close(pipefd[0]);
                
                if (bytes_read > 0) {
                    dragon_output[bytes_read] = '\0';
                    send(cli_socket, dragon_output, bytes_read, 0);
                }
                
                send_message_eof(cli_socket);
            } else {
                send_message_eof(cli_socket);
            }
            
            free_cmd_buff(&temp_cmd);
            continue;
        }
        
        free_cmd_buff(&temp_cmd);
        
        rc = build_cmd_list(io_buff, &cmd_list);
        
        if (rc == WARN_NO_CMDS) {
            send_message_string(cli_socket, CMD_WARN_NO_CMD);
            continue;
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            char error_msg[100];
            snprintf(error_msg, sizeof(error_msg), CMD_ERR_PIPE_LIMIT, CMD_MAX);
            send_message_string(cli_socket, error_msg);
            continue;
        } else if (rc != OK) {
            char error_msg[100];
            snprintf(error_msg, sizeof(error_msg), "Error parsing command: %d\n", rc);
            send_message_string(cli_socket, error_msg);
            continue;
        }
        
        cmd_rc = rsh_execute_pipeline(cli_socket, &cmd_list);
        printf(RCMD_MSG_SVR_RC_CMD, cmd_rc);
        
        if (cmd_rc == EXIT_SC) {
            free_cmd_list(&cmd_list);
            free(io_buff);
            return OK;
        } else if (cmd_rc == STOP_SERVER_SC) {
            free_cmd_list(&cmd_list);
            free(io_buff);
            return OK_EXIT;
        }
        
        send_message_eof(cli_socket);
        
        free_cmd_list(&cmd_list);
    }

    free(io_buff);
    return OK;
}

int send_message_eof(int cli_socket) {
    int bytes_sent;
    
    bytes_sent = send(cli_socket, &RDSH_EOF_CHAR, 1, 0);
    
    if (bytes_sent != 1) {
        perror("send EOF");
        return ERR_RDSH_COMMUNICATION;
    }
    
    return OK;
}

int send_message_string(int cli_socket, char *buff) {
    int send_len = strlen(buff);
    int sent_len;
    int remaining = send_len;
    int offset = 0;
    
    while (remaining > 0) {
        sent_len = send(cli_socket, buff + offset, remaining, 0);
        
        if (sent_len < 0) {
            perror("send");
            return ERR_RDSH_COMMUNICATION;
        }
        
        remaining -= sent_len;
        offset += sent_len;
    }
    
    return send_message_eof(cli_socket);
}

Built_In_Cmds rsh_match_command(const char *input) {
    if (strcmp(input, "exit") == 0)
        return BI_CMD_EXIT;
    if (strcmp(input, "dragon") == 0)
        return BI_CMD_DRAGON;
    if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    if (strcmp(input, "stop-server") == 0)
        return BI_CMD_EXIT;
    return BI_NOT_BI;
}

Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd) {
    Built_In_Cmds ctype = BI_NOT_BI;
    
    if (cmd == NULL || cmd->argc == 0 || cmd->argv[0] == NULL) {
        return BI_NOT_BI;
    }
    
    ctype = rsh_match_command(cmd->argv[0]);

    switch (ctype) {
    case BI_CMD_DRAGON:
        return BI_EXECUTED;
    case BI_CMD_EXIT:
        if (strcmp(cmd->argv[0], "stop-server") == 0) {
            return BI_CMD_EXIT;
        }
        return BI_CMD_EXIT;
    case BI_CMD_CD:
        return BI_EXECUTED;
    default:
        return BI_NOT_BI;
    }
}

int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    int pipes[CMD_MAX - 1][2];
    pid_t pids[CMD_MAX];
    int pids_st[CMD_MAX];
    Built_In_Cmds bi_cmd;
    int exit_code = 0;

    if (clist->num == 0) {
        return 0;
    }
    
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    for (int i = 0; i < clist->num; i++) {
        bi_cmd = rsh_built_in_cmd(&clist->commands[i]);
        
        if (bi_cmd == BI_CMD_EXIT) {
            if (strcmp(clist->commands[i].argv[0], "stop-server") == 0) {
                for (int j = 0; j < clist->num - 1; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                return STOP_SERVER_SC;
            }
            
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return EXIT_SC;
        } else if (bi_cmd == BI_EXECUTED) {
            pids[i] = -1;
            pids_st[i] = 0;
            continue;
        }

        pids[i] = fork();
        
        if (pids[i] < 0) {
            perror("fork");
            
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            return ERR_EXEC_CMD;
        } else if (pids[i] == 0) {
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            if (i < clist->num - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            } else {
                dup2(cli_sock, STDOUT_FILENO);
            }
            
            dup2(cli_sock, STDERR_FILENO);
            
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Error: '%s': %s\n", 
                     clist->commands[i].argv[0], strerror(errno));
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = s0; i < clist->num; i++) {
        if (pids[i] > 0) {
            waitpid(pids[i], &pids_st[i], 0);
            
            if (i == clist->num - 1) {
                exit_code = WEXITSTATUS(pids_st[i]);
            }
        }
    }
    
    return exit_code;
}