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

int exec_remote_cmd_loop(char *address, int port)
{
    char *cmd_buff;
    char *rsp_buff;
    int cli_socket;
    ssize_t io_size;
    int is_eof;

    // Allocate buffers for sending and receiving data
    cmd_buff = malloc(RDSH_COMM_BUFF_SZ);
    rsp_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (!cmd_buff || !rsp_buff) {
        perror("malloc");
        return client_cleanup(-1, cmd_buff, rsp_buff, ERR_MEMORY);
    }

    // Connect to the server
    cli_socket = start_client(address, port);
    if (cli_socket < 0) {
        perror("start client");
        return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_CLIENT);
    }

    while (1) 
    {
        // Print the prompt
        printf("%s", SH_PROMPT);

        // Get input from the user
        if (fgets(cmd_buff, RDSH_COMM_BUFF_SZ, stdin) == NULL) {
            printf("\n");
            break; // EOF or error, exit gracefully
        }

        // Remove newline and ensure null termination
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';
        if (strlen(cmd_buff) == 0) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }

        // Send the command to the server (include null terminator)
        io_size = strlen(cmd_buff) + 1;
        if (send(cli_socket, cmd_buff, io_size, 0) != io_size) {
            perror("send");
            return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
        }

        // Receive all results until EOF character
        size_t rsp_pos = 0;
        is_eof = 0;
        while (!is_eof) {
            io_size = recv(cli_socket, rsp_buff + rsp_pos, RDSH_COMM_BUFF_SZ - rsp_pos - 1, 0);
            if (io_size <= 0) {
                if (io_size == 0) {
                    printf("Server disconnected\n");
                } else {
                    perror("recv");
                }
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }

            rsp_pos += io_size;
            rsp_buff[rsp_pos] = '\0'; // Ensure null termination for safety

            // Check if the last byte is EOF
            if (rsp_pos > 0 && rsp_buff[rsp_pos - 1] == RDSH_EOF_CHAR) {
                is_eof = 1;
                rsp_buff[rsp_pos - 1] = '\0'; // Remove EOF for printing
            }

            // Print received data (handles non-null-terminated strings)
            printf("%.*s", (int)io_size, rsp_buff + (rsp_pos - io_size));
        }

        // Break on "exit" command (client-side termination)
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            break;
        }
    }

    return client_cleanup(cli_socket, cmd_buff, rsp_buff, OK);
}

int start_client(char *server_ip, int port)
{
    struct sockaddr_in addr;
    int cli_socket;
    int ret;

    // Create the client socket
    cli_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cli_socket == -1) {
        perror("socket");
        return ERR_RDSH_CLIENT;
    }

    // Set up the server address structure
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(server_ip);
    addr.sin_port = htons(port);

    // Connect to the server
    ret = connect(cli_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        perror("connect");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    printf("Connected to server %s:%d\n", server_ip, port);
    return cli_socket;
}

int client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc){
    //If a valid socket number close it.
    if(cli_socket > 0){
        close(cli_socket);
    }

    //Free up the buffers 
    free(cmd_buff);
    free(rsp_buff);

    //Echo the return value that was passed as a parameter
    return rc;
}