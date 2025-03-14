#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#include "dshlib.h"
#include "rshlib.h"

// Forward declarations for functions used in this file.
int process_cli_requests_single(int svr_socket);
int process_cli_requests_threaded(int svr_socket);
void *handle_client(void *arg);

// Global flag used by the multi-threaded server to signal shutdown.
// If any client sends the "stop-server" command, this flag is set.
volatile int server_stop_flag = 0;

/*
 * start_server:
 *   1. Resets server_stop_flag to 0 so each new run starts fresh.
 *   2. Boots the server via boot_server().
 *   3. Depending on is_threaded, calls process_cli_requests_single()
 *      or process_cli_requests_threaded().
 *   4. Closes the server socket when done.
 */
int start_server(char *ifaces, int port, int is_threaded) {
    // Reset the stop flag on each server start
    server_stop_flag = 0;

    int svr_socket;
    int rc;

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        return svr_socket;  // This will be one of the ERR_RDSH_* codes
    }

    if (is_threaded) {
        rc = process_cli_requests_threaded(svr_socket);
    } else {
        rc = process_cli_requests_single(svr_socket);
    }

    stop_server(svr_socket);
    return rc;
}

/*
 * stop_server:
 *  Closes the server socket.
 */
int stop_server(int svr_socket) {
    return close(svr_socket);
}

/*
 * boot_server:
 *  Creates a server socket, sets SO_REUSEADDR, binds to the provided interface
 *  and port, and begins listening. Returns the server socket fd on success,
 *  or ERR_RDSH_COMMUNICATION on failure.
 */
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

    if (setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
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
    if (ret < 0) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    return svr_socket;
}

/*
 * process_cli_requests_single:
 *  Single-threaded loop that accepts client connections in a while(1).
 *  For each accepted client, we call exec_client_requests().
 *  If exec_client_requests() returns OK_EXIT (stop-server), break the loop.
 */
int process_cli_requests_single(int svr_socket) {
    int cli_socket;
    int rc = OK;

    while (1) {
        cli_socket = accept(svr_socket, NULL, NULL);
        if (cli_socket < 0) {
            perror("accept");
            rc = ERR_RDSH_COMMUNICATION;
            break;
        }

        rc = exec_client_requests(cli_socket);
        close(cli_socket);

        if (rc == OK_EXIT) { // "stop-server" received
            break;
        }
    }
    return rc;
}

/*
 * process_cli_requests_threaded:
 *  Multi-threaded loop that spawns a new thread for each accepted client connection.
 *  Each client is handled by handle_client(). If any client sends "stop-server",
 *  the global server_stop_flag is set, and this loop will exit.
 */
int process_cli_requests_threaded(int svr_socket) {
    int rc = OK;

    while (!server_stop_flag) {
        int *cli_socket_ptr = malloc(sizeof(int));
        if (!cli_socket_ptr) {
            perror("malloc");
            rc = ERR_RDSH_COMMUNICATION;
            break;
        }

        *cli_socket_ptr = accept(svr_socket, NULL, NULL);
        if (*cli_socket_ptr < 0) {
            perror("accept");
            free(cli_socket_ptr);
            rc = ERR_RDSH_COMMUNICATION;
            break;
        }

        // Create a thread to handle the client
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, cli_socket_ptr) != 0) {
            perror("pthread_create");
            close(*cli_socket_ptr);
            free(cli_socket_ptr);
            rc = ERR_RDSH_COMMUNICATION;
            break;
        }
        pthread_detach(tid);
    }
    return rc;
}

/*
 * handle_client:
 *  Thread function to handle client requests. Calls exec_client_requests().
 *  If "stop-server" is received, set server_stop_flag = 1.
 */
void *handle_client(void *arg) {
    int cli_socket = *(int *)arg;
    free(arg);

    int rc = exec_client_requests(cli_socket);
    close(cli_socket);

    if (rc == OK_EXIT) {
        server_stop_flag = 1;
    }

    return NULL;
}

/*
 * exec_client_requests:
 *  Reads commands from the client socket until the client disconnects or
 *  we detect a built-in command that indicates an exit condition.
 *
 *  Returns:
 *    OK       - if client sent "exit"
 *    OK_EXIT  - if client sent "stop-server"
 *    ERR_RDSH_COMMUNICATION - on a socket or memory error
 */
int exec_client_requests(int cli_socket) {
    command_list_t cmd_list;
    cmd_list.num = 0;

    char *io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (io_buff == NULL) {
        return ERR_RDSH_SERVER;
    }

    while (1) {
        memset(io_buff, 0, RDSH_COMM_BUFF_SZ);

        ssize_t io_size = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ, 0);
        if (io_size < 0) {
            perror("recv");
            free(io_buff);
            return ERR_RDSH_COMMUNICATION;
        }
        if (io_size == 0) {
            // Client closed connection
            break;
        }

        // If the last byte is EOF marker, remove it
        if (io_buff[io_size - 1] == RDSH_EOF_CHAR) {
            io_buff[io_size - 1] = '\0';
        }

        // Check for built-in commands
        if (strcmp(io_buff, EXIT_CMD) == 0) {
            free(io_buff);
            return OK;       // client wants to exit
        }
        if (strcmp(io_buff, "stop-server") == 0) {
            free(io_buff);
            return OK_EXIT; // signal to stop the server
        }

        // Parse commands into a pipeline
        int rc = build_cmd_list(io_buff, &cmd_list);
        if (rc != OK) {
            send_message_string(cli_socket, "rdsh-error: failed to parse command\n");
            send_message_eof(cli_socket);
            continue;
        }

        // Execute pipeline
        rc = rsh_execute_pipeline(cli_socket, &cmd_list);
        if (rc != OK) {
            send_message_string(cli_socket, CMD_ERR_RDSH_EXEC); // "rdsh-error: command execution error\n"
        }

        // Clean up and send EOF marker
        free_cmd_list(&cmd_list);
        send_message_eof(cli_socket);
    }

    free(io_buff);
    return OK;
}

/*
 * send_message_eof:
 *  Sends the EOF character to the client to indicate the server is done.
 */
int send_message_eof(int cli_socket) {
    int send_len = (int)sizeof(RDSH_EOF_CHAR);
    int sent_len = send(cli_socket, &RDSH_EOF_CHAR, send_len, 0);
    if (sent_len != send_len) {
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

/*
 * send_message_string:
 *  Sends a null-terminated string and then the EOF marker.
 */
int send_message_string(int cli_socket, char *buff) {
    int total_len = strlen(buff) + 1; // include null terminator
    int sent_len = send(cli_socket, buff, total_len, 0);
    if (sent_len != total_len) {
        fprintf(stderr, CMD_ERR_RDSH_SEND, sent_len, total_len);
        return ERR_RDSH_COMMUNICATION;
    }
    return send_message_eof(cli_socket);
}

/*
 * rsh_execute_pipeline:
 *  Adapts the local execute_pipeline() logic for networked I/O:
 *    - The first command’s stdin is dup2’d to cli_sock.
 *    - The last command’s stdout is dup2’d to cli_sock.
 *    - Intermediate commands use pipes.
 */
int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    int num = clist->num;
    if (num < 1) {
        return WARN_NO_CMDS;
    }
    if (num > CMD_MAX) {
        fprintf(stderr, CMD_ERR_PIPE_LIMIT, CMD_MAX);
        fprintf(stderr, "\n");
        return ERR_TOO_MANY_COMMANDS;
    }

    // Create pipes
    int pipes[CMD_MAX - 1][2];
    for (int i = 0; i < num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pids[CMD_MAX];
    int status_arr[CMD_MAX];

    for (int i = 0; i < num; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            // Child
            // Redirect stdin
            if (i == 0) {
                if (dup2(cli_sock, STDIN_FILENO) < 0) {
                    perror("dup2 cli_sock -> STDIN");
                    exit(errno);
                }
            } else {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2 pipe -> STDIN");
                    exit(errno);
                }
            }

            // Redirect stdout
            if (i == num - 1) {
                if (dup2(cli_sock, STDOUT_FILENO) < 0) {
                    perror("dup2 cli_sock -> STDOUT");
                    exit(errno);
                }
            } else {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2 STDOUT -> pipe");
                    exit(errno);
                }
            }

            // Close all pipes in child
            for (int j = 0; j < num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Setup any file redirection
            setup_redirection(&clist->commands[i]);

            // Execute command
            if (execvp(clist->commands[i].argv[0], clist->commands[i].argv) == -1) {
                perror("execvp");
                exit(errno);
            }
        } else {
            // Parent
            pids[i] = pid;
        }
    }

    // Parent closes all pipes
    for (int i = 0; i < num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children
    for (int i = 0; i < num; i++) {
        waitpid(pids[i], &status_arr[i], 0);
    }

    // Determine the exit code of the last process
    int exit_code = WEXITSTATUS(status_arr[num - 1]);
    for (int i = 0; i < num; i++) {
        if (WEXITSTATUS(status_arr[i]) == EXIT_SC) {
            exit_code = EXIT_SC;
        }
    }
    return exit_code;
}

/***************************  Extra Credit Stubs  ******************************/

/*
 * set_threaded_server:
 *  Optionally used to configure a threaded server. Currently a stub.
 */
void set_threaded_server(int val) {
    (void)val; // Suppress unused parameter warning
}

/*
 * rsh_match_command:
 *  Optional function that returns a Built_In_Cmds enum for the given input.
 *  If not used, we suppress warnings by referencing 'input' and returning a default.
 */
Built_In_Cmds rsh_match_command(const char *input) {
    // If this function is unused or incomplete, do the following to avoid warnings:
    (void)input; // suppress unused-parameter warning
    // Or add your actual logic here
    return BI_NOT_BI; // ensure we always return something
}

/*
 * rsh_built_in_cmd:
 *  Optional function that handles remote built-in commands.
 *  If not used, we suppress warnings similarly.
 */
Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd) {
    (void)cmd; // suppress unused-parameter warning
    // Or add your actual logic here
    return BI_NOT_BI; // ensure we always return something
}