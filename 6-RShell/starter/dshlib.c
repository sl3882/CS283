#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dragon.txt"
#include "dshlib.h"

int exec_local_cmd_loop()
{
    char cmd_line[SH_CMD_MAX];    // Buffer for storing the command line input
    cmd_buff_t cmd_buff;          // Structure to store parsed command buffer
    command_list_t cmd_list;      // Structure to store command list for pipeline
    int status;                   // Variable to store status of execution

    // Infinite loop to repeatedly get and process commands
    while (1)
    {
        printf("%s", SH_PROMPT);  // Print shell prompt

        // Read command line from standard input
        if (fgets(cmd_line, sizeof(cmd_line), stdin) == NULL)
        {
            printf("\n");    // If input is NULL, print a newline and break loop
            break;
        }

        cmd_line[strcspn(cmd_line, "\n")] = '\0';   // Remove newline character from the input string

        if (strlen(cmd_line) == 0)   // If command line is empty
        {
            printf("%s", CMD_WARN_NO_CMD);  // Print warning about no command
            continue;
        }

        // Check if the input command is the exit command
        if (strcmp(cmd_line, EXIT_CMD) == 0)
        {
            break;  // Exit the loop if the exit command is entered
        }

        // Check if the command contains a pipe character
        if (strchr(cmd_line, PIPE_CHAR) != NULL)
        {
            // Build the command list for pipeline
            if ((status = build_cmd_list(cmd_line, &cmd_list)) != OK)
            {
                // Handle errors in building command list
                if (status == ERR_TOO_MANY_COMMANDS)
                {
                    printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);  // Error: Too many commands in pipeline
                }
                else if (status == WARN_NO_CMDS)
                {
                    printf("%s", CMD_WARN_NO_CMD);  // Warning: No commands found
                }
                else
                {
                    printf("Error building command list\n");  // General error message
                }
                free_cmd_list(&cmd_list);  // Free the command list and continue loop
                continue;
            }

            // Execute the pipeline of commands
            if ((status = execute_pipeline(&cmd_list)) != OK)
            {
                // If pipeline execution fails, print error
                if (status != OK_EXIT)
                {
                    printf(CMD_ERR_EXECUTE, cmd_line);
                }
                else
                {
                    free_cmd_list(&cmd_list);  // Free the command list and exit
                    return OK;
                }
            }

            free_cmd_list(&cmd_list);  // Free the command list after execution
        }
        else
        {
            // Handle single command (no pipeline)
            if ((status = alloc_cmd_buff(&cmd_buff)) != OK)
            {
                printf("Memory allocation error\n");  // Error in allocating memory for command buffer
                continue;
            }

            // Build the command buffer from the input string
            if ((status = build_cmd_buff(cmd_line, &cmd_buff)) != OK)
            {
                if (status == WARN_NO_CMDS)
                {
                    printf("%s", CMD_WARN_NO_CMD);  // Warning: No commands found
                }
                free_cmd_buff(&cmd_buff);  // Free the command buffer and continue
                continue;
            }

            // Execute built-in commands if matched
            Built_In_Cmds bi_status = exec_built_in_cmd(&cmd_buff);
            if (bi_status == BI_EXECUTED)
            {
                free_cmd_buff(&cmd_buff);  // Free buffer after execution of built-in command
                continue;
            }
            else if (bi_status == BI_CMD_EXIT)
            {
                free_cmd_buff(&cmd_buff);  // Free buffer and exit if exit command is found
                break;
            }

            // Execute external command
            if ((status = exec_cmd(&cmd_buff)) != OK)
            {
                printf(CMD_ERR_EXECUTE, cmd_line);  // Error executing external command
            }

            free_cmd_buff(&cmd_buff);  // Free the command buffer after execution
        }
    }

    return OK;  // Return OK after the loop ends
}

// Match input command to built-in command types
Built_In_Cmds match_command(const char *input)
{
    if (strcmp(input, EXIT_CMD) == 0)
    {
        return BI_CMD_EXIT;  // Exit command
    }
    else if (strcmp(input, "cd") == 0)
    {
        return BI_CMD_CD;    // Change directory command
    }
    else if (strcmp(input, "dragon") == 0)
    {
        return BI_CMD_DRAGON; // Dragon command
    }
    return BI_NOT_BI;  // Return non-built-in if no match
}

// Allocate memory for command buffer
int alloc_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX * sizeof(char));  // Allocate memory for command buffer
    if (cmd_buff->_cmd_buffer == NULL)
    {
        return ERR_MEMORY;  // Error if memory allocation fails
    }
    cmd_buff->argc = 0;  // Initialize argument count to 0
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL;  // Initialize argument list to NULL
    }
    return OK;  // Return OK if successful
}

// Build command buffer from input string
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    clear_cmd_buff(cmd_buff);  // Clear the command buffer before building

    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1);  // Copy input line to command buffer
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';  // Ensure null-termination of string

    char *token = strtok(cmd_buff->_cmd_buffer, " ");  // Tokenize the command line based on space
    while (token != NULL)
    {
        if (cmd_buff->argc >= CMD_ARGV_MAX - 1)  // Check for too many arguments
        {
            return ERR_CMD_OR_ARGS_TOO_BIG;  // Error if too many arguments
        }
        cmd_buff->argv[cmd_buff->argc] = token;  // Store token in argument list
        cmd_buff->argc++;  // Increment argument count
        token = strtok(NULL, " ");  // Get next token
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;  // Null-terminate argument list
    if (cmd_buff->argc == 0)
    {
        return WARN_NO_CMDS;  // Return warning if no commands were found
    }
    return OK;  // Return OK if successful
}

// Free memory allocated for command buffer
int free_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (cmd_buff->_cmd_buffer != NULL)
    {
        free(cmd_buff->_cmd_buffer);  // Free the command buffer
        cmd_buff->_cmd_buffer = NULL;  // Set pointer to NULL
    }
    cmd_buff->argc = 0;  // Reset argument count
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL;  // Clear the argument list
    }
    return OK;  // Return OK if successful
}

// Clear command buffer (reset its values)
int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->argc = 0;  // Reset argument count
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL;  // Clear the argument list
    }
    if (cmd_buff->_cmd_buffer != NULL)
    {
        cmd_buff->_cmd_buffer[0] = '\0';  // Clear the command buffer string
    }
    return OK;  // Return OK if successful
}

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    // Match the command to its corresponding type
    Built_In_Cmds cmd_type = match_command(cmd->argv[0]);

    // Switch based on the matched command type
    switch (cmd_type)
    {
    case BI_CMD_CD:  // Change directory command
        if (cmd->argc > 1)  // If there's a directory argument
        {
            if (chdir(cmd->argv[1]) != 0)  // Try changing to the specified directory
            {
                perror("cd");  // Print an error if the directory change fails
            }
        }
        else  // If no argument is given, change to the home directory
        {
            char *home = getenv("HOME");  // Get the home directory from environment
            if (home && chdir(home) != 0)  // Try changing to the home directory
            {
                perror("cd");  // Print an error if the directory change fails
            }
        }
        return BI_EXECUTED;  // Return that the built-in command was executed

    case BI_CMD_EXIT:  // Exit command
        return BI_CMD_EXIT;  // Return that the exit command was matched

    case BI_CMD_DRAGON:  // Custom dragon text command
        printf("%s", dragon_txt);  // Print the dragon text
        return BI_EXECUTED;  // Return that the built-in command was executed

    default:  // If no built-in command is matched
        return BI_NOT_BI;  // Return that the command is not built-in
    }
}

// Function to free a list of commands
int free_cmd_list(command_list_t *clist)
{
    for (int i = 0; i < clist->num; i++)  // Loop through all commands
    {
        free_cmd_buff(&clist->commands[i]);  // Free each command buffer
    }
    clist->num = 0;  // Reset the number of commands
    return OK;  // Return OK after freeing the list
}

// Function to execute a command
int exec_cmd(cmd_buff_t *cmd)
{
    pid_t pid;  // Process ID
    int status;  // Status of the child process

    pid = fork();  // Create a new child process
    if (pid < 0)  // If fork fails
    {
        perror("fork");  // Print an error message
        return ERR_EXEC_CMD;  // Return error
    }
    else if (pid == 0)  // If we are in the child process
    {
        if (execvp(cmd->argv[0], cmd->argv) < 0)  // Try executing the command
        {
            perror("execvp");  // Print an error if execvp fails
            exit(ERR_EXEC_CMD);  // Exit the child process with an error code
        }
    }
    else  // If we are in the parent process
    {
        waitpid(pid, &status, 0);  // Wait for the child process to finish
        if (WIFEXITED(status))  // If the child process exited normally
        {
            return WEXITSTATUS(status);  // Return the exit status of the child process
        }
        else  // If the child process did not exit normally
        {
            return ERR_EXEC_CMD;  // Return error
        }
    }
    return OK;  // Return OK (although this line should not be reached)
}

// Function to execute a pipeline of commands
int execute_pipeline(command_list_t *clist)
{
    if (clist->num == 0)  // If no commands are given
    {
        fprintf(stderr, CMD_WARN_NO_CMD);  // Print a warning message
        return WARN_NO_CMDS;  // Return a warning code
    }

    int pipes[CMD_MAX - 1][2];  // Array to store pipe file descriptors
    pid_t pids[CMD_MAX];  // Array to store process IDs

    // Create pipes for each command in the pipeline
    for (int i = 0; i < clist->num - 1; i++)
    {
        if (pipe(pipes[i]) == -1)  // If creating a pipe fails
        {
            perror("pipe");  // Print an error message
            return ERR_MEMORY;  // Return an error code
        }
    }

    // Fork a new process for each command in the pipeline
    for (int i = 0; i < clist->num; i++)
    {
        pids[i] = fork();  // Create a new child process

        if (pids[i] == -1)  // If forking fails
        {
            perror("fork");  // Print an error message
            for (int j = 0; j < i; j++)  // Close all previously created pipes
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return ERR_MEMORY;  // Return an error code
        }

        if (pids[i] == 0)  // If we are in the child process
        {
            if (i > 0)  // If this is not the first command in the pipeline
            {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1)  // Redirect input from the previous pipe
                {
                    perror("dup2");  // Print an error message
                    exit(ERR_EXEC_CMD);  // Exit with an error code
                }
            }

            if (i < clist->num - 1)  // If this is not the last command in the pipeline
            {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1)  // Redirect output to the next pipe
                {
                    perror("dup2");  // Print an error message
                    exit(ERR_EXEC_CMD);  // Exit with an error code
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
    // Wait for all child processes to finish
    for (int i = 0; i < clist->num; i++)
    {
        int status;
        waitpid(pids[i], &status, 0);  // Wait for the child process

        if (WIFEXITED(status) && WEXITSTATUS(status) == OK_EXIT)  // If any child process exits with OK_EXIT
        {
            exit_status = OK_EXIT;  // Set the overall exit status to OK_EXIT
        }
    }

    return exit_status;  // Return the final exit status of the pipeline
}


int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    // Initialize the number of commands to 0
    clist->num = 0;

    // Duplicate the input command line string to avoid modifying the original one
    char *cmd_copy = strdup(cmd_line);
    if (cmd_copy == NULL)
    {
        // Return error code if memory allocation fails
        return ERR_MEMORY;
    }

    // Pointer used by strtok_r for tokenizing the command line
    char *saveptr;
    // Tokenize the command line by the "|" character, separating different commands in a pipeline
    char *token = strtok_r(cmd_copy, "|", &saveptr);
    
    // Loop through each token (command) while there are tokens and the number of commands is within the limit
    while (token != NULL && clist->num < CMD_MAX)
    {
        // Duplicate the token (command) string to process it further
        char *tmp = strdup(token);
        if (tmp == NULL)
        {
            // Free memory if duplicate fails and return memory error
            free(cmd_copy);
            return ERR_MEMORY;
        }

        // Skip leading whitespace in the command string
        char *start = tmp;
        while (*start && isspace(*start))
            start++;

        // Trim trailing whitespace from the command string
        char *end = start + strlen(start) - 1;
        while (end > start && isspace(*end))
            *end-- = '\0';

        // If the command string is empty after trimming, free the memory and continue to the next token
        if (*start == '\0')
        {
            free(tmp);
            token = strtok_r(NULL, "|", &saveptr);
            continue;
        }

        // Allocate memory for a new command buffer in the command list
        if (alloc_cmd_buff(&clist->commands[clist->num]) != OK)
        {
            // Free memory in case allocation fails and return memory error
            free(tmp);
            free(cmd_copy);
            return ERR_MEMORY;
        }

        // Build the command buffer with the processed command string
        if (build_cmd_buff(start, &clist->commands[clist->num]) != OK)
        {
            // Free memory if building the command buffer fails and return an error
            free(tmp);
            free(cmd_copy);
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }

        // Increment the number of commands processed
        clist->num++;
        // Free temporary memory for the command string
        free(tmp);
        
        // Move to the next token (command in the pipeline)
        token = strtok_r(NULL, "|", &saveptr);
    }

    // Free the memory allocated for the duplicated command line string
    free(cmd_copy);

    // If there are too many commands in the list, return an error
    if (token != NULL && clist->num >= CMD_MAX)
    {
        return ERR_TOO_MANY_COMMANDS;
    }

    // Return OK if command list has been successfully built
    return OK;
}