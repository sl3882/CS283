
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>

// INCLUDES for extra credit
// #include <signal.h>
// #include <pthread.h>
//-------------------------

#include "dshlib.h"
#include "rshlib.h"

typedef struct
{
    int client_socket;
    struct sockaddr_in client_addr;
} client_args_t;



void* client_handler(void* args) {
    client_args_t* client_data = (client_args_t*)args;
    int cli_socket = client_data->client_socket;
    struct sockaddr_in client_addr = client_data->client_addr;
    free(client_data);  // Free the dynamically allocated client data

    printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    int result = exec_client_requests(cli_socket);
    
    close(cli_socket);

    if (result == STOP_SERVER_SC) {
        printf("%s", RCMD_MSG_SVR_STOP_REQ);
    }

    return NULL;
}









int start_server(char *ifaces, int port, int is_threaded)
{
    int svr_socket;
    int rc;

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0)
    {
        int err_code = svr_socket; // server socket will carry error code
        return err_code;
    }

    rc = process_cli_requests(svr_socket, is_threaded);
    stop_server(svr_socket);

    return rc;
}

int stop_server(int svr_socket)
{
    return close(svr_socket);
}

int boot_server(char *ifaces, int port)
{
    int svr_socket;
    int ret;

    struct sockaddr_in addr;

    // Create the server socket
    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket == -1)
    {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }

    // Set socket option to reuse address
    int enable = 1;
    if (setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("setsockopt");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Set up the address structure
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Bind the socket to the address and port
    ret = bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Listen for incoming connections with a backlog of 20
    ret = listen(svr_socket, 20);
    if (ret == -1)
    {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    return svr_socket;
}

int process_cli_requests(int svr_socket,int is_threaded)
{
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (1)
    {
        // Accept client connection
        client_socket = accept(svr_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0)
        {

            perror("accept");
            continue;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (is_threaded) {
            pthread_t thread_id;
            client_args_t* client_data = malloc(sizeof(client_args_t));
            client_data->client_socket = client_socket;
            client_data->client_addr = client_addr;

            pthread_create(&thread_id, NULL, client_handler, (void*)client_data);
            pthread_detach(thread_id);  // Automatically clean up after the thread
        } else {
            // Handle client requests in the main thread
            exec_client_requests(client_socket);
            close(client_socket);
        }
    }
    return OK;
}

int exec_client_requests(int cli_socket)
{
    char *io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (!io_buff)
    {
        perror("malloc");
        return ERR_MEMORY;
    }

    ssize_t bytes_received;

    while (1)
    {
        // Clear buffer
        memset(io_buff, 0, RDSH_COMM_BUFF_SZ);

        // Receive command from client
        bytes_received = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ - 1, 0);

        // Check for errors or disconnection
        if (bytes_received <= 0)
        {
            if (bytes_received < 0)
            {
                perror("recv");
            }
            free(io_buff);
            return OK; // Client disconnected or error
        }

        // Null-terminate the buffer
        io_buff[bytes_received] = '\0';

        printf(RCMD_MSG_SVR_EXEC_REQ, io_buff);

        // Check for exit command
        if (strcmp(io_buff, EXIT_CMD) == 0)
        {
            // Echo back the exit command
            send_message_string(cli_socket, io_buff);
            free(io_buff);
            return OK;
        }

        // Check for stop-server command
        if (strcmp(io_buff, "stop-server") == 0)
        {
            // Echo back the stop-server command
            send_message_string(cli_socket, io_buff);
            free(io_buff);
            return STOP_SERVER_SC;
        }

        // Echo back the received command
        send_message_string(cli_socket, io_buff);
    }

    free(io_buff);
    return OK;
}

int send_message_eof(int cli_socket)
{
    int send_len = (int)sizeof(RDSH_EOF_CHAR);
    send_len = send(cli_socket, &RDSH_EOF_CHAR, 1, 0);

    if (send_len != 1)
    {

        return ERR_RDSH_COMMUNICATION;
    }

    return OK;
}

int send_message_string(int cli_socket, char *buff)
{
    size_t msg_len = strlen(buff);
    ssize_t bytes_sent = send(cli_socket, buff, msg_len, 0);

    if (bytes_sent != msg_len)
    {
        perror("Error sending message");
        return ERR_RDSH_COMMUNICATION;
    }

    return send_message_eof(cli_socket); // Send EOF after message
}

int rsh_execute_pipeline(int cli_sock, command_list_t *clist)
{
    if (clist->num == 0)
    { // No commands given
        send_message_string(cli_sock, CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }

    int pipes[CMD_MAX - 1][2]; // For pipes between commands
    pid_t pids[CMD_MAX];       // Process IDs for each command

    // Create pipes for each command in the pipeline
    for (int i = 0; i < clist->num - 1; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            send_message_string(cli_sock, "Pipe creation failed\n");
            return ERR_MEMORY;
        }
    }

    // Fork processes for each command in the pipeline
    for (int i = 0; i < clist->num; i++)
    {
        pids[i] = fork();
        if (pids[i] == -1)
        { // Forking failed
            send_message_string(cli_sock, "Fork failed\n");
            for (int j = 0; j < i; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return ERR_MEMORY;
        }

        if (pids[i] == 0)
        { // Child process
            // Handle input/output for commands
            if (i == 0)
            { // First command
                if (dup2(cli_sock, STDIN_FILENO) == -1)
                {
                    perror("dup2 stdin");
                    exit(ERR_EXEC_CMD);
                }
            }

            if (i == clist->num - 1)
            { // Last command
                if (dup2(cli_sock, STDOUT_FILENO) == -1 || dup2(cli_sock, STDERR_FILENO) == -1)
                {
                    perror("dup2 stdout/stderr");
                    exit(ERR_EXEC_CMD);
                }
            }

            if (i > 0)
            { // Not the first command
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
                {
                    perror("dup2 input pipe");
                    exit(ERR_EXEC_CMD);
                }
            }

            if (i < clist->num - 1)
            { // Not the last command
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1)
                {
                    perror("dup2 output pipe");
                    exit(ERR_EXEC_CMD);
                }
            }

            // Close all pipes in the child process
            for (int j = 0; j < clist->num - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Try executing built-in commands first
            Built_In_Cmds bi_status = exec_built_in_cmd(&clist->commands[i]);
            if (bi_status == BI_EXECUTED)
            {
                exit(OK);
            }
            else if (bi_status == BI_CMD_EXIT)
            {
                exit(OK_EXIT);
            }

            // Execute non-built-in commands using execvp
            if (execvp(clist->commands[i].argv[0], clist->commands[i].argv) < 0)
            {
                perror("execvp");
                exit(ERR_EXEC_CMD); // Exit child process on exec failure
            }
        }
    }

    // Parent process closes all pipe file descriptors
    for (int i = 0; i < clist->num - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    int exit_status = OK; // Track overall status
    int last_status = OK; // Track last command status

    // Wait for all child processes to finish
    for (int i = 0; i < clist->num; i++)
    {
        int status;
        waitpid(pids[i], &status, 0);

        if (i == clist->num - 1)
        {
            last_status = WEXITSTATUS(status); // Save last command's exit status
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == OK_EXIT)
        {
            exit_status = OK_EXIT;
        }
    }

    // Return the exit status from the last command or OK_EXIT if any command ended with it
    return (exit_status == OK) ? last_status : exit_status;
}
