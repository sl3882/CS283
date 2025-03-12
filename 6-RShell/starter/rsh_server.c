
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>

//INCLUDES for extra credit
//#include <signal.h>
//#include <pthread.h>
//-------------------------

#include "dshlib.h"
#include "rshlib.h"


typedef struct {
    int client_socket;
} client_args_t;



int start_server(char *ifaces, int port, int is_threaded){
    int svr_socket;
    int rc;

 

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0){
        int err_code = svr_socket;  //server socket will carry error code
        return err_code;
    }

    rc = process_cli_requests(svr_socket);

    stop_server(svr_socket);


    return rc;
}


int stop_server(int svr_socket){
    return close(svr_socket);
}


int boot_server(char *ifaces, int port){
    int svr_socket;
    int ret;
    
    struct sockaddr_in addr;
    
    // Create the server socket
    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket == -1) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }
    
    // Set socket option to reuse address
    int enable = 1;
    if (setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }
    
    // Set up the address structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }
    
    // Bind the socket to the address and port
    ret = bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }
    
    // Listen for incoming connections with a backlog of 20
    ret = listen(svr_socket, 20);
    if (ret == -1) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }
    
    return svr_socket;
}


int process_cli_requests(int svr_socket) {
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    while (1) {
        // Accept client connection
        client_socket = accept(svr_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }
        
        printf("Client connected: %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        // Handle client requests
        int result = exec_client_requests(client_socket);
        
        // Close client socket
        close(client_socket);
        
        // Check if server should be stopped
        if (result == STOP_SERVER_SC) {
            printf("%s", RCMD_MSG_SVR_STOP_REQ);
            return OK;
        }
        
        printf("%s", RCMD_MSG_CLIENT_EXITED);
    }
    
    return OK;
}



int exec_client_requests(int cli_socket) {
    char * io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (! io_buff) {
        perror("malloc");
        return ERR_MEMORY;
    }
    
    ssize_t bytes_received;
    
    while (1) {
        // Clear buffer
        memset( io_buff, 0, RDSH_COMM_BUFF_SZ);
        
        // Receive command from client
        bytes_received = recv(cli_socket,  io_buff, RDSH_COMM_BUFF_SZ - 1, 0);
        
        // Check for errors or disconnection
        if (bytes_received <= 0) {
            if (bytes_received < 0) {
                perror("recv");
            }
            free( io_buff);
            return OK;  // Client disconnected or error
        }
        
        // Null-terminate the buffer
         io_buff[bytes_received] = '\0';
        
        printf(RCMD_MSG_SVR_EXEC_REQ,  io_buff);
        
        // Check for exit command
        if (strcmp( io_buff, EXIT_CMD) == 0) {
            // Echo back the exit command
            send_message_string(cli_socket,  io_buff);
            free( io_buff);
            return OK;
        }
        
        // Check for stop-server command
        if (strcmp( io_buff, "stop-server") == 0) {
            // Echo back the stop-server command
            send_message_string(cli_socket,  io_buff);
            free( io_buff);
            return STOP_SERVER_SC;
        }
        
        // Echo back the received command
        send_message_string(cli_socket,  io_buff);
    }
    
    free( io_buff);
    return OK;
}


int send_message_eof(int cli_socket) {
    int send_len = (int)sizeof(RDSH_EOF_CHAR);
    send_len = send(cli_socket, &RDSH_EOF_CHAR, 1, 0);
    
    if (send_len != 1) {
        
        return ERR_RDSH_COMMUNICATION;
    }
    
    return OK;
}

int send_message_string(int cli_socket, char *buff) {
    // Send the message string
    size_t msg_len = strlen(buff);
    ssize_t bytes_sent = send(cli_socket, buff, msg_len, 0);
    
    if (bytes_sent != msg_len) {
        perror("Error sending message");
        return ERR_RDSH_COMMUNICATION;
    }
    
    // Send EOF character
    return send_message_eof(cli_socket);
}


int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    int pipes[clist->num - 1][2];  // Array of pipes
    pid_t pids[clist->num];
    int  pids_st[clist->num];         // Array to store process IDs
    Built_In_Cmds bi_cmd;
    int exit_code;

    // Create all necessary pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        
        if (pids[i] == 0) {  // Child process
            // First command in pipeline
            if (i == 0) {
                // Read from client socket
                dup2(cli_sock, STDIN_FILENO);
                
                // If there are more commands, set up pipe for output
                if (clist->num > 1) {
                    dup2(pipes[0][1], STDOUT_FILENO);
                } else {
                    // If only one command, output goes back to client socket
                    dup2(cli_sock, STDOUT_FILENO);
                }
            }
            // Last command in pipeline
            else if (i == clist->num - 1) {
                // Read from previous pipe
                dup2(pipes[i-1][0], STDIN_FILENO);
                
                // Write to client socket
                dup2(cli_sock, STDOUT_FILENO);
            }
            // Middle commands in pipeline
            else {
                // Read from previous pipe
                dup2(pipes[i-1][0], STDIN_FILENO);
                
                // Write to next pipe
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe ends in child process
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Also redirect stderr to client socket
            dup2(cli_sock, STDERR_FILENO);
            
            // Check if it's a built-in command
            bi_cmd = check_built_in(clist->cmd[i].argv[0]);
            if (bi_cmd != INVALID_CMD) {
                exit(execute_built_in(bi_cmd, clist->cmd[i].argv));
            }
            
            // Execute the command
            execvp(clist->cmd[i].argv[0], clist->cmd[i].argv);
            
            // If execvp returns, it means an error occurred
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    
    // Parent process: close all pipe ends
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all children
    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], &pids_st[i], 0);
    }
    
    //by default get exit code of last process
    //use this as the return value
    exit_code = WEXITSTATUS(pids_st[clist->num - 1]);
    
    for (int i = 0; i < clist->num; i++) {
        //if any commands in the pipeline are EXIT_SC
        //return that to enable the caller to react
        if (WEXITSTATUS(pids_st[i]) == EXIT_SC)
            exit_code = EXIT_SC;
    }
    
    return exit_code;
}