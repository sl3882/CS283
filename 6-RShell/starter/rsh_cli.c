
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
    int cli_socket;
    char *request_buff = NULL;
    char *resp_buff = NULL;
    char cmd_line[SH_CMD_MAX];
    ssize_t recv_bytes;
    int is_eof;

    // Allocate buffers for sending and receiving data
    request_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (request_buff == NULL) {
        perror("Memory allocation failed for request buffer");
        return ERR_MEMORY;
    }

    resp_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (resp_buff == NULL) {
        perror("Memory allocation failed for response buffer");
        free(request_buff);
        return ERR_MEMORY;
    }

    // Create network connection to the server
    cli_socket = start_client(address, port);
    if (cli_socket < 0) {
        return client_cleanup(cli_socket, request_buff, resp_buff, ERR_RDSH_CLIENT);
    }

    // Enter infinite command loop
    while (1) {
        printf("%s", SH_PROMPT); // Print shell prompt

        // Read command line from standard input
        if (fgets(cmd_line, sizeof(cmd_line), stdin) == NULL) {
            printf("\n");
            break;
        }

        // Remove newline character
        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        // Check for empty command
        if (strlen(cmd_line) == 0) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }

        // Check for exit command
        if (strcmp(cmd_line, EXIT_CMD) == 0) {
            break;
        }

        // Special handling for stop-server command
        if (strcmp(cmd_line, "stop-server") == 0) {
            // Send the command and expect a specific return status
            int exec_status = STOP_SERVER_SC;
            return client_cleanup(cli_socket, request_buff, resp_buff, exec_status);
        }

        // Send command to server (including null byte)
        if (send(cli_socket, cmd_line, strlen(cmd_line) + 1, 0) < 0) {
            perror("Error sending command to server");
            return client_cleanup(cli_socket, request_buff, resp_buff, ERR_RDSH_COMMUNICATION);
        }

        // Receive response loop
        while (1) {
            recv_bytes = recv(cli_socket, resp_buff, RDSH_COMM_BUFF_SZ - 1, 0);

            if (recv_bytes < 0) {
                // Communication error
                perror("Error receiving response from server");
                return client_cleanup(cli_socket, request_buff, resp_buff, ERR_RDSH_COMMUNICATION);
            }

            if (recv_bytes == 0) {
                // Server disconnected
                printf("%s", RCMD_SERVER_EXITED);
                return client_cleanup(cli_socket, request_buff, resp_buff, ERR_RDSH_CLIENT);
            }

            // Null-terminate the received buffer for safety
            resp_buff[recv_bytes] = '\0';

            // Print received data
            printf("%.*s", (int)recv_bytes, resp_buff);

            // Check if this is the last transmission
            is_eof = (resp_buff[recv_bytes - 1] == RDSH_EOF_CHAR) ? 1 : 0;

            if (is_eof) {
                break; // Exit receive loop
            }
        }
    }

    // Clean up and exit
    return client_cleanup(cli_socket, request_buff, resp_buff, OK);
}

int start_client(char *server_ip, int port)
{
    int client_socket;
    struct sockaddr_in server_addr;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Error creating socket");
        return ERR_RDSH_CLIENT;
    }

    // Set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert IP address from string to network format
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        close(client_socket);
        return ERR_RDSH_CLIENT;
    }

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        close(client_socket);
        return ERR_RDSH_CLIENT;
    }

    return client_socket;
}
/*
 * client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc)
 *      cli_socket:   The client socket
 *      cmd_buff:     The buffer that will hold commands to send to server
 *      rsp_buff:     The buffer that will hld server responses
 * 
 *   This function does the following: 
 *      1. If cli_socket > 0 it calls close(cli_socket) to close the socket
 *      2. It calls free() on cmd_buff and rsp_buff
 *      3. It returns the value passed as rc
 *  
 *   Note this function is intended to be helper to manage exit conditions
 *   from the exec_remote_cmd_loop() function given there are several
 *   cleanup steps.  We provide it to you fully implemented as a helper.
 *   You do not have to use it if you want to develop an alternative
 *   strategy for cleaning things up in your exec_remote_cmd_loop()
 *   implementation. 
 * 
 *   returns:
 *          rc:   This function just returns the value passed as the 
 *                rc parameter back to the caller.  This way the caller
 *                can just write return client_cleanup(...)
 *      
 */
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