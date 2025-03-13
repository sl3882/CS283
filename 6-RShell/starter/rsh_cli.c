#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dshlib.h"
#include "rshlib.h"

int exec_remote_cmd_loop(char *address, int port) {
    int cli_socket = -1; // Client socket descriptor
    char *cmd_buff = (char *)malloc(RDSH_COMM_BUFF_SZ); // Buffer for command input
    char *rsp_buff = (char *)malloc(RDSH_COMM_BUFF_SZ); // Buffer for server response

    // Check if memory allocation was successful
    if (cmd_buff == NULL || rsp_buff == NULL) {
        return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_MEMORY); // Return error if allocation fails
    }

    // Connect to the server
    cli_socket = start_client(address, port);
    if (cli_socket < 0) {
        fprintf(stderr, "Error: Failed to connect to server at %s:%d\n", address, port);
        return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_CLIENT); // Return error if connection fails
    }

    // Main loop for command execution
    while (1) {
        printf("%s", SH_PROMPT); // Display the shell prompt

        // Get user input (command)
        if (fgets(cmd_buff, RDSH_COMM_BUFF_SZ, stdin) == NULL) {
            printf("\n"); // Print newline if fgets returns NULL (e.g., EOF)
            break; // Exit the loop
        }

        // Remove the newline character from the command
        size_t len = strlen(cmd_buff);
        if (len > 0 && cmd_buff[len - 1] == '\n') {
            cmd_buff[len - 1] = '\0';
            len--;
        }

        // Skip empty commands
        if (len == 0) {
            continue; // Go to the next iteration of the loop
        }

        // Check for the exit command
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            // Send the exit command to the server
            ssize_t io_size = send(cli_socket, cmd_buff, len + 1, 0); // Include null terminator
            if (io_size < 0) {
                perror("send");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION); // Return error if send fails
            }
            break; // Exit the loop
        }

        // Send the command to the server
        ssize_t io_size = send(cli_socket, cmd_buff, len + 1, 0); // Include null terminator
        if (io_size < 0) {
            perror("send");
            return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION); // Return error if send fails
        }

        // Receive the response from the server
        while (1) {
            memset(rsp_buff, 0, RDSH_COMM_BUFF_SZ); // Clear the response buffer
            io_size = recv(cli_socket, rsp_buff, RDSH_COMM_BUFF_SZ - 1, 0); // Receive data

            if (io_size < 0) {
                perror("recv");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION); // Return error if recv fails
            }

            if (io_size == 0) {
                // Server closed the connection
                printf("%s\n", RCMD_SERVER_EXITED);
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION); // Return error
            }

            // Check for the end-of-file (EOF) character from the server
            if (rsp_buff[io_size - 1] == RDSH_EOF_CHAR) {
                rsp_buff[io_size - 1] = '\0'; // Replace EOF with null terminator
                printf("%s", rsp_buff); // Print the response
                break; // Exit the inner loop
            } else {
                printf("%s", rsp_buff); // Print the partial response
            }
        }
    }

    return client_cleanup(cli_socket, cmd_buff, rsp_buff, OK); // Clean up and return OK
}

// Function to start the client and connect to the server
int start_client(char *server_ip, int port) {
    int client_socket; // Client socket descriptor
    struct sockaddr_in server_addr; // Server address structure

    // Create a socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Error creating socket");
        return ERR_RDSH_CLIENT; // Return error if socket creation fails
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert the IP address from string to binary format
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP address");
        close(client_socket);
        return ERR_RDSH_CLIENT; // Return error if IP address is invalid
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        close(client_socket);
        return ERR_RDSH_CLIENT; // Return error if connection fails
    }

    return client_socket; // Return the client socket descriptor
}

// Function to clean up resources
int client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc) {
    // Close the socket if it's valid
    if (cli_socket > 0) {
        close(cli_socket);
    }

    // Free the command buffer if it's allocated
    if (cmd_buff != NULL) {
        free(cmd_buff);
    }
    // Free the response buffer if it's allocated
    if (rsp_buff != NULL) {
        free(rsp_buff);
    }

    return rc; // Return the return code
}