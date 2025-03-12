
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
    int cli_socket;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int rc;

    while (1) {
        // Accept a client connection
        cli_socket = accept(svr_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (cli_socket < 0) {
            perror("Error accepting client connection");
            return ERR_RDSH_COMMUNICATION;
        }

        // Execute client requests
        rc = exec_client_requests(cli_socket);

        // Close the client socket
        close(cli_socket);

        // Check if we should stop the server
        if (rc == OK_EXIT) {
            return OK_EXIT;
        }
    }
}
/*
 * exec_client_requests(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client
 *   
 *  This function handles accepting remote client commands. The function will
 *  loop and continue to accept and execute client commands.  There are 2 ways
 *  that this ongoing loop accepting client commands ends:
 * 
 *      1.  When the client executes the `exit` command, this function returns
 *          to process_cli_requests() so that we can accept another client
 *          connection. 
 *      2.  When the client executes the `stop-server` command this function
 *          returns to process_cli_requests() with a return code of OK_EXIT
 *          indicating that the server should stop. 
 * 
 *  Note that this function largely follows the implementation of the
 *  exec_local_cmd_loop() function that you implemented in the last 
 *  shell program deliverable. The main difference is that the command will
 *  arrive over the recv() socket call rather than reading a string from the
 *  keyboard. 
 * 
 *  This function also must send the EOF character after a command is
 *  successfully executed to let the client know that the output from the
 *  command it sent is finished.  Use the send_message_eof() to accomplish 
 *  this. 
 * 
 *  Of final note, this function must allocate a buffer for storage to 
 *  store the data received by the client. For example:
 *     io_buff = malloc(RDSH_COMM_BUFF_SZ);
 *  And since it is allocating storage, it must also properly clean it up
 *  prior to exiting.
 * 
 *  Returns:
 * 
 *      OK:       The client sent the `exit` command.  Get ready to connect
 *                another client. 
 *      OK_EXIT:  The client sent `stop-server` command to terminate the server
 * 
 *      ERR_RDSH_COMMUNICATION:  A catch all for any socket() related send
 *                or receive errors. 
 */
int exec_client_requests(int cli_socket) {
    char *io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (io_buff == NULL) {
        perror("Memory allocation failed");
        return ERR_RDSH_COMMUNICATION;
    }

    while (1) {
        // Clear the buffer
        memset(io_buff, 0, RDSH_COMM_BUFF_SZ);

        // Receive command from client
        ssize_t bytes_received = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ - 1, 0);
        
        if (bytes_received <= 0) {
            // Connection closed or error
            free(io_buff);
            return ERR_RDSH_COMMUNICATION;
        }

        // Ensure null-termination
        io_buff[bytes_received] = '\0';

        // Create command buffer to parse the received command
        cmd_buff_t cmd_buff;
        if (alloc_cmd_buff(&cmd_buff) != OK) {
            send_message_string(cli_socket, "Memory allocation error\n");
            continue;
        }

        // Build command buffer
        if (build_cmd_buff(io_buff, &cmd_buff) != OK) {
            send_message_string(cli_socket, "Invalid command\n");
            free_cmd_buff(&cmd_buff);
            continue;
        }

        // Check for stop-server command
        if (strcmp(cmd_buff.argv[0], "stop-server") == 0) {
            free_cmd_buff(&cmd_buff);
            free(io_buff);
            return OK_EXIT;
        }

        // Check for exit command
        if (strcmp(cmd_buff.argv[0], "exit") == 0) {
            free_cmd_buff(&cmd_buff);
            send_message_eof(cli_socket);
            break;
        }

        // Execute command
        pid_t pid = fork();
        if (pid < 0) {
            send_message_string(cli_socket, "Fork failed\n");
            free_cmd_buff(&cmd_buff);
            continue;
        }
        else if (pid == 0) {
            // Child process
            dup2(cli_socket, STDOUT_FILENO);
            dup2(cli_socket, STDERR_FILENO);
            close(cli_socket);

            if (execvp(cmd_buff.argv[0], cmd_buff.argv) < 0) {
                perror("Exec failed");
                exit(ERR_EXEC_CMD);
            }
        }
        else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            
            // Send EOF to indicate command completion
            send_message_eof(cli_socket);
        }

        free_cmd_buff(&cmd_buff);
    }

    free(io_buff);
    return OK;
}

/*
 * send_message_eof(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client

 *  Sends the EOF character to the client to indicate that the server is
 *  finished executing the command that it sent. 
 * 
 *  Returns:
 * 
 *      OK:  The EOF character was sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the EOF character. 
 */
int send_message_eof(int cli_socket) {
    // Send the EOF character
    ssize_t bytes_sent = send(cli_socket, &RDSH_EOF_CHAR, 1, 0);
    
    if (bytes_sent != 1) {
        perror("Error sending EOF character");
        return ERR_RDSH_COMMUNICATION;
    }
    
    return OK;
}

/*
 * send_message_string(cli_socket, char *buff)
 *      cli_socket:  The server-side socket that is connected to the client
 *      buff:        A C string (aka null terminated) of a message we want
 *                   to send to the client. 
 *   
 *  Sends a message to the client.  Note this command executes both a send()
 *  to send the message and a send_message_eof() to send the EOF character to
 *  the client to indicate command execution terminated. 
 * 
 *  Returns:
 * 
 *      OK:  The message in buff followed by the EOF character was 
 *           sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the message followed by the EOF character. 
 */
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


/*
 * rsh_execute_pipeline(int cli_sock, command_list_t *clist)
 *      cli_sock:    The server-side socket that is connected to the client
 *      clist:       The command_list_t structure that we implemented in
 *                   the last shell. 
 *   
 *  This function executes the command pipeline.  It should basically be a
 *  replica of the execute_pipeline() function from the last deliverable. 
 *  The only thing different is that you will be using the cli_sock as the
 *  main file descriptor on the first executable in the pipeline for STDIN,
 *  and the cli_sock for the file descriptor for STDOUT, and STDERR for the
 *  last executable in the pipeline.  See picture below:  
 * 
 *      
 *┌───────────┐                                                    ┌───────────┐
 *│ cli_sock  │                                                    │ cli_sock  │
 *└─────┬─────┘                                                    └────▲──▲───┘
 *      │   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐  │  │    
 *      │   │   Process 1  │     │   Process 2  │     │   Process N  │  │  │    
 *      │   │              │     │              │     │              │  │  │    
 *      └───▶stdin   stdout├─┬──▶│stdin   stdout├─┬──▶│stdin   stdout├──┘  │    
 *          │              │ │   │              │ │   │              │     │    
 *          │        stderr├─┘   │        stderr├─┘   │        stderr├─────┘    
 *          └──────────────┘     └──────────────┘     └──────────────┘   
 *                                                      WEXITSTATUS()
 *                                                      of this last
 *                                                      process to get
 *                                                      the return code
 *                                                      for this function       
 * 
 *  Returns:
 * 
 *      EXIT_CODE:  This function returns the exit code of the last command
 *                  executed in the pipeline.  If only one command is executed
 *                  that value is returned.  Remember, use the WEXITSTATUS()
 *                  macro that we discussed during our fork/exec lecture to
 *                  get this value. 
 */
int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    if (clist->num == 0)  // If no commands are given
    {
        send_message_string(cli_sock, CMD_WARN_NO_CMD);  // Send warning message
        return WARN_NO_CMDS;  // Return a warning code
    }

    int pipes[CMD_MAX - 1][2];  // Array to store pipe file descriptors
    pid_t pids[CMD_MAX];  // Array to store process IDs

    // Create pipes for each command in the pipeline
    for (int i = 0; i < clist->num - 1; i++)
    {
        if (pipe(pipes[i]) == -1)  // If creating a pipe fails
        {
            send_message_string(cli_sock, "Pipe creation failed\n");
            return ERR_MEMORY;  // Return an error code
        }
    }

    // Fork a new process for each command in the pipeline
    for (int i = 0; i < clist->num; i++)
    {
        pids[i] = fork();  // Create a new child process

        if (pids[i] == -1)  // If forking fails
        {
            send_message_string(cli_sock, "Fork failed\n");
            for (int j = 0; j < i; j++)  // Close all previously created pipes
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return ERR_MEMORY;  // Return an error code
        }

        if (pids[i] == 0)  // If we are in the child process
        {
            // Special handling for first and last commands in the pipeline
            if (i == 0)  // First command in pipeline
            {
                // Use client socket as input
                if (dup2(cli_sock, STDIN_FILENO) == -1)
                {
                    perror("dup2 stdin");
                    exit(ERR_EXEC_CMD);
                }
            }
            
            if (i == clist->num - 1)  // Last command in pipeline
            {
                // Use client socket as output and error
                if (dup2(cli_sock, STDOUT_FILENO) == -1 || 
                    dup2(cli_sock, STDERR_FILENO) == -1)
                {
                    perror("dup2 stdout/stderr");
                    exit(ERR_EXEC_CMD);
                }
            }

            // Handle input/output for intermediate commands
            if (i > 0)  // Not the first command
            {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
                {
                    perror("dup2 input pipe");
                    exit(ERR_EXEC_CMD);
                }
            }

            if (i < clist->num - 1)  // Not the last command
            {
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

            // Execute the built-in command if applicable
            Built_In_Cmds bi_status = exec_built_in_cmd(&clist->commands[i]);
            if (bi_status == BI_EXECUTED)  // If the built-in command was executed
            {
                exit(OK);  // Exit the child process with success
            }
            else if (bi_status == BI_CMD_EXIT)  // If the built-in command was exit
            {
                exit(OK_EXIT);  // Exit the child process with the exit code
            }

            // If it's not a built-in command, execute it using execvp
            if (execvp(clist->commands[i].argv[0], clist->commands[i].argv) < 0)
            {
                perror("execvp");  // Print an error message
                exit(ERR_EXEC_CMD);  // Exit the child process with an error code
            }
        }
    }

    // Close all pipes in the parent process
    for (int i = 0; i < clist->num - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    int exit_status = OK;  // Variable to track the exit status of the pipeline
    int last_status = OK;  // Store the status of the last command
    // Wait for all child processes to finish
    for (int i = 0; i < clist->num; i++)
    {
        int status;
        waitpid(pids[i], &status, 0);  // Wait for the child process

        if (i == clist->num - 1)  // For the last command
        {
            last_status = WEXITSTATUS(status);  // Store the exit status of the last command
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == OK_EXIT)  // If any child process exits with OK_EXIT
        {
            exit_status = OK_EXIT;  // Set the overall exit status to OK_EXIT
        }
    }

    // If exit_status is OK, return the last command's exit status
    return (exit_status == OK) ? last_status : exit_status;
}

/**************   OPTIONAL STUFF  ***************/
/****
 **** NOTE THAT THE FUNCTIONS BELOW ALIGN TO HOW WE CRAFTED THE SOLUTION
 **** TO SEE IF A COMMAND WAS BUILT IN OR NOT.  YOU CAN USE A DIFFERENT
 **** STRATEGY IF YOU WANT.  IF YOU CHOOSE TO DO SO PLEASE REMOVE THESE
 **** FUNCTIONS AND THE PROTOTYPES FROM rshlib.h
 **** 
 */

/*
 * rsh_match_command(const char *input)
 *      cli_socket:  The string command for a built-in command, e.g., dragon,
 *                   cd, exit-server
 *   
 *  This optional function accepts a command string as input and returns
 *  one of the enumerated values from the BuiltInCmds enum as output. For
 *  example:
 * 
 *      Input             Output
 *      exit              BI_CMD_EXIT
 *      dragon            BI_CMD_DRAGON
 * 
 *  This function is entirely optional to implement if you want to handle
 *  processing built-in commands differently in your implementation. 
 * 
 *  Returns:
 * 
 *      BI_CMD_*:   If the command is built-in returns one of the enumeration
 *                  options, for example "cd" returns BI_CMD_CD
 * 
 *      BI_NOT_BI:  If the command is not "built-in" the BI_NOT_BI value is
 *                  returned. 
 */






// Built_In_Cmds rsh_match_command(const char *input)
// {
//     return BI_NOT_IMPLEMENTED;
// }







/*
 * rsh_built_in_cmd(cmd_buff_t *cmd)
 *      cmd:  The cmd_buff_t of the command, remember, this is the 
 *            parsed version fo the command
 *   
 *  This optional function accepts a parsed cmd and then checks to see if
 *  the cmd is built in or not.  It calls rsh_match_command to see if the 
 *  cmd is built in or not.  Note that rsh_match_command returns BI_NOT_BI
 *  if the command is not built in. If the command is built in this function
 *  uses a switch statement to handle execution if appropriate.   
 * 
 *  Again, using this function is entirely optional if you are using a different
 *  strategy to handle built-in commands.  
 * 
 *  Returns:
 * 
 *      BI_NOT_BI:   Indicates that the cmd provided as input is not built
 *                   in so it should be sent to your fork/exec logic
 *      BI_EXECUTED: Indicates that this function handled the direct execution
 *                   of the command and there is nothing else to do, consider
 *                   it executed.  For example the cmd of "cd" gets the value of
 *                   BI_CMD_CD from rsh_match_command().  It then makes the libc
 *                   call to chdir(cmd->argv[1]); and finally returns BI_EXECUTED
 *      BI_CMD_*     Indicates that a built-in command was matched and the caller
 *                   is responsible for executing it.  For example if this function
 *                   returns BI_CMD_STOP_SVR the caller of this function is
 *                   responsible for stopping the server.  If BI_CMD_EXIT is returned
 *                   the caller is responsible for closing the client connection.
 * 
 *   AGAIN - THIS IS TOTALLY OPTIONAL IF YOU HAVE OR WANT TO HANDLE BUILT-IN
 *   COMMANDS DIFFERENTLY. 
 */





// Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd)
// {
//     return BI_NOT_IMPLEMENTED;
// }