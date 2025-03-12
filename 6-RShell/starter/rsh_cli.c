
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
    cmd_buff = (char *)malloc(RDSH_COMM_BUFF_SZ);
    rsp_buff = (char *)malloc(RDSH_COMM_BUFF_SZ);
    
    if (cmd_buff == NULL || rsp_buff == NULL) {
        return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_MEMORY);
    }

    // Connect to server
    cli_socket = start_client(address, port);
    if (cli_socket < 0) {
        perror("start client");
        return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_CLIENT);
    }

    while (1) {
        // Print prompt
        printf("%s", SH_PROMPT);
        
        // Get user input
        if (fgets(cmd_buff, RDSH_COMM_BUFF_SZ, stdin) == NULL) {
            printf("\n");
            break;
        }
        
        // Remove newline character
        size_t len = strlen(cmd_buff);
        if (len > 0 && cmd_buff[len-1] == '\n') {
            cmd_buff[len-1] = '\0';
            len--;
        }
        
        // Skip empty commands
        if (len == 0) {
            continue;
        }
        
        // Check for local exit command
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            // Send exit command to server
            io_size = send(cli_socket, cmd_buff, len, 0);
            if (io_size < 0) {
                perror("send");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }
            
            // Receive response (for proper shutdown)
            while ((io_size = recv(cli_socket, rsp_buff, RDSH_COMM_BUFF_SZ - 1, 0)) > 0) {
                // Check for EOF character
                is_eof = (rsp_buff[io_size-1] == RDSH_EOF_CHAR) ? 1 : 0;
                
                // Print response (excluding EOF if present)
                if (is_eof) {
                    printf("%.*s", (int)io_size - 1, rsp_buff);
                } else {
                    printf("%.*s", (int)io_size, rsp_buff);
                }
                
                if (is_eof) {
                    break;
                }
            }
            
            // Exit the loop
            break;
        }
        
        // Send command to server
        io_size = send(cli_socket, cmd_buff, len, 0);
        if (io_size < 0) {
            perror("send");
            return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
        }
        
        // Receive response from server
        while (1) {
            memset(rsp_buff, 0, RDSH_COMM_BUFF_SZ);
            io_size = recv(cli_socket, rsp_buff, RDSH_COMM_BUFF_SZ - 1, 0);
            
            if (io_size < 0) {
                perror("recv");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }
            
            if (io_size == 0) {
                // Server closed connection
                printf("%s", RCMD_SERVER_EXITED);
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }
            
            // Check for EOF character
            is_eof = (rsp_buff[io_size-1] == RDSH_EOF_CHAR) ? 1 : 0;
            
            // Print response (excluding EOF if present)
            if (is_eof) {
                printf("%.*s", (int)io_size - 1, rsp_buff);
            } else {
                printf("%.*s", (int)io_size, rsp_buff);
            }
            
            // If EOF received, break the receive loop
            if (is_eof) {
                break;
            }
        }
    }

    return client_cleanup(cli_socket, cmd_buff, rsp_buff, OK);
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