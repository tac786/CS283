#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>  
#include "dshlib.h"
#include "rshlib.h"

// You can change this if your tests or instructions require a different prompt.
#define REMOTE_PROMPT "dsh> "

/*
 * client_cleanup:
 *  Closes the client socket (if valid) and frees the allocated buffers.
 */
int client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc) {
    if (cli_socket > 0) {
        close(cli_socket);
    }
    free(cmd_buff);
    free(rsp_buff);
    return rc;
}

/*
 * start_client:
 *  Creates a socket, connects to the server using the provided IP address and port,
 *  and returns the socket file descriptor.
 *
 *  Returns:
 *    cli_socket: on success
 *    ERR_RDSH_CLIENT: on failure
 */
int start_client(char *server_ip, int port) {
    int cli_socket;
    struct sockaddr_in server_addr;

    // Create socket (IPv4, TCP)
    cli_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cli_socket < 0) {
        perror("socket");
        return ERR_RDSH_CLIENT;
    }

    // Set up the server address struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    // Connect to the server
    if (connect(cli_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    return cli_socket;
}

/*
 * exec_remote_cmd_loop:
 *  Implements the remote client command loop:
 *   - Allocates send/receive buffers.
 *   - Connects to the server using start_client().
 *   - In a loop, prints a prompt, reads a command from the user,
 *     sends it to the server (including the null byte), and then
 *     receives and prints the server's response chunk-by-chunk.
 *   - Exits when the user types "exit".
 *
 *  Returns:
 *    OK if the loop completes normally
 *    ERR_MEMORY if buffer allocation fails
 *    ERR_RDSH_CLIENT if socket creation/connection fails
 *    ERR_RDSH_COMMUNICATION if there's a send/recv error
 */
int exec_remote_cmd_loop(char *address, int port) {
    char *cmd_buff = malloc(RDSH_COMM_BUFF_SZ);
    char *rsp_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (cmd_buff == NULL || rsp_buff == NULL) {
        perror("malloc");
        return client_cleanup(-1, cmd_buff, rsp_buff, ERR_MEMORY);
    }

    int cli_socket = start_client(address, port);
    if (cli_socket < 0) {
        perror("start_client");
        return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_CLIENT);
    }

    while (1) {
        // Print prompt for remote shell
        printf("%s", REMOTE_PROMPT);
        fflush(stdout);

        // Read user input (ensure null-termination)
        if (fgets(cmd_buff, RDSH_COMM_BUFF_SZ, stdin) == NULL) {
            // Typically indicates EOF or Ctrl+D from user
            printf("\n");
            break;
        }
        // Remove trailing newline character
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // If the user enters an empty command, skip to next iteration
        if (strlen(cmd_buff) == 0) {
            continue;
        }

        // If the command is "exit", break out of loop
        if (strcmp(cmd_buff, "exit") == 0) {
            // Optionally send "exit" to the server, but typically just break
            break;
        }

        // Calculate send length (include null byte)
        int send_len = strlen(cmd_buff) + 1;
        int bytes_sent = send(cli_socket, cmd_buff, send_len, 0);
        if (bytes_sent != send_len) {
            fprintf(stderr, "rdsh-error: partial send. Sent %d, expected %d\n", bytes_sent, send_len);
            return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
        }

        // Now, receive the response from the server
        while (1) {
            ssize_t recv_size = recv(cli_socket, rsp_buff, RDSH_COMM_BUFF_SZ, 0);
            if (recv_size < 0) {
                perror("recv");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }
            if (recv_size == 0) {
                // Server closed connection
                fprintf(stderr, "rdsh-error: server closed connection unexpectedly.\n");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }

            // Check if this is the last chunk by looking for the EOF marker
            int is_eof = (rsp_buff[recv_size - 1] == RDSH_EOF_CHAR) ? 1 : 0;
            if (is_eof) {
                // Replace the EOF marker with a null terminator
                rsp_buff[recv_size - 1] = '\0';
            }

            // Print the received data
            // Using "%.*s" so we don't accidentally read beyond the buffer
            printf("%.*s", (int)recv_size, rsp_buff);
            fflush(stdout);

            // If last chunk, break out of recv loop
            if (is_eof) {
                break;
            }
        }
    }

    // If we exit the loop gracefully, clean up and return OK
    int rc = client_cleanup(cli_socket, cmd_buff, rsp_buff, OK);
    // Optionally, print a final line if your tests expect it
    // e.g., "cmd loop returned 0"
    printf("cmd loop returned %d\n", rc);
    return rc;
}