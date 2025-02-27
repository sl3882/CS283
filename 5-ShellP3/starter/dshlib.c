#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"


int exec_local_cmd_loop()
{
    char cmd_line[SH_CMD_MAX];
    cmd_buff_t cmd_buff;
    command_list_t cmd_list;
    int status;

    while (1)
    {
        printf("%s", SH_PROMPT);

        if (fgets(cmd_line, sizeof(cmd_line), stdin) == NULL)
        {
            printf("\n");
            break;
        }

        // Remove trailing newline
        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        // Check if input is empty
        if (strlen(cmd_line) == 0)
        {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }

        // Exit condition
        if (strcmp(cmd_line, EXIT_CMD) == 0)
        {
            break;
        }

        // Check for pipe characters
        if (strchr(cmd_line, PIPE_CHAR) != NULL)
        {
            // Handle pipeline command
            if ((status = build_cmd_list(cmd_line, &cmd_list)) != OK)
            {
                if (status == ERR_TOO_MANY_COMMANDS)
                {
                    printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
                }
                else if (status == WARN_NO_CMDS)
                {
                    printf("%s", CMD_WARN_NO_CMD);
                }
                else
                {
                    printf("Error building command list\n");
                }
                free_cmd_list(&cmd_list);
                continue;
            }

            // Execute pipeline
            if ((status = execute_pipeline(&cmd_list)) != OK)
            {
                if (status != OK_EXIT)  // Don't show error for exit command
                {
                    printf(CMD_ERR_EXECUTE, cmd_line);
                }
                else
                {
                    // Exit was called from inside a pipeline
                    free_cmd_list(&cmd_list);
                    return OK;
                }
            }

            // Free command list
            free_cmd_list(&cmd_list);
        }
        else
        {
            // Handle single command
            // Allocate command buffer
            if ((status = alloc_cmd_buff(&cmd_buff)) != OK)
            {
                printf("Memory allocation error\n");
                continue;
            }

            // Build command buffer
            if ((status = build_cmd_buff(cmd_line, &cmd_buff)) != OK)
            {
                if (status == WARN_NO_CMDS)
                {
                    printf("%s", CMD_WARN_NO_CMD);
                }
                free_cmd_buff(&cmd_buff);
                continue;
            }

            // Execute built-in commands
            Built_In_Cmds bi_status = exec_built_in_cmd(&cmd_buff);
            if (bi_status == BI_EXECUTED)
            {
                free_cmd_buff(&cmd_buff);
                continue;
            }
            else if (bi_status == BI_CMD_EXIT)
            {
                free_cmd_buff(&cmd_buff);
                break; // Exit the loop
            }

            // Execute external command
            if ((status = exec_cmd(&cmd_buff)) != OK)
            {
                printf(CMD_ERR_EXECUTE, cmd_line);
            }

            // Free allocated command buffer
            free_cmd_buff(&cmd_buff);
        }
    }

    return OK;
}





Built_In_Cmds match_command(const char *input)
{
    if (strcmp(input, EXIT_CMD) == 0)
    {
        return BI_CMD_EXIT;
    }
    else if (strcmp(input, "cd") == 0)
    {
        return BI_CMD_CD;
    }
    else if (strcmp(input, "dragon") == 0)
    {
        return BI_CMD_DRAGON;
    }
    return BI_NOT_BI;
}

int alloc_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX * sizeof(char)); // Allocate memory for command buffer
    if (cmd_buff->_cmd_buffer == NULL)
    {
        return ERR_MEMORY; // Return error if memory allocation fails
    }
    cmd_buff->argc = 0; // Initialize argument count to 0
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL; // Initialize argument vector to NULL
    }
    return OK; // Return OK if successful
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    clear_cmd_buff(cmd_buff); // Clear the command buffer
    
    // Copy the command line to the internal buffer
    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';
    
    char *token = strtok(cmd_buff->_cmd_buffer, " "); // Tokenize the command line
    while (token != NULL)
    {
        if (cmd_buff->argc >= CMD_ARGV_MAX - 1)
        {
            return ERR_CMD_OR_ARGS_TOO_BIG; // Return error if too many arguments
        }
        cmd_buff->argv[cmd_buff->argc] = token; // Add token to argument vector
        cmd_buff->argc++;
        token = strtok(NULL, " ");
    }
    cmd_buff->argv[cmd_buff->argc] = NULL; // Null-terminate the argument vector
    if (cmd_buff->argc == 0)
    {
        return WARN_NO_CMDS; // Return warning if no commands parsed
    }
    return OK;
}

int free_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (cmd_buff->_cmd_buffer != NULL)
    {
        free(cmd_buff->_cmd_buffer); // Free the command buffer
        cmd_buff->_cmd_buffer = NULL;
    }
    cmd_buff->argc = 0; // Reset argument count to 0
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL; // Reset argument vector to NULL
    }
    return OK;
}
int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->argc = 0; // Reset argument count to 0
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL; // Reset argument vector to NULL
    }
    if (cmd_buff->_cmd_buffer != NULL)
    {
        cmd_buff->_cmd_buffer[0] = '\0'; // Clear the command buffer
    }
    return OK;
}



Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    // Use match_command to identify the built-in command
    Built_In_Cmds cmd_type = match_command(cmd->argv[0]);
    
    switch (cmd_type)
    {
        case BI_CMD_CD:
            // If there's an argument provided for cd
            if (cmd->argc > 1)
            {
                // Change directory using chdir()
                if (chdir(cmd->argv[1]) != 0)
                {
                    // Print error message if chdir fails
                    perror("cd");
                }
            }
            else
            {
                // If no argument provided, change to home directory
                char *home = getenv("HOME");
                if (home && chdir(home) != 0)
                {
                    perror("cd");
                }
            }
            return BI_EXECUTED;
        
        case BI_CMD_EXIT:
            return BI_CMD_EXIT;
            
        case BI_CMD_DRAGON:
            printf("Rawr! 🐉\n");
            return BI_EXECUTED;
            
        default:
            return BI_NOT_BI;
    }
}



int free_cmd_list(command_list_t *clist)
{
    for (int i = 0; i < clist->num; i++)
    {
        free_cmd_buff(&clist->commands[i]);
    }
    clist->num = 0;
    return OK;
}


int exec_cmd(cmd_buff_t *cmd)
{
    pid_t pid;  // Process ID
    int status; // Status of child process

    pid = fork(); // Fork a new process
    if (pid < 0)
    {
        perror("fork"); // Fork failed
        return ERR_EXEC_CMD;
    }
    else if (pid == 0)
    {
        if (execvp(cmd->argv[0], cmd->argv) < 0)
        {
            perror("execvp"); // execvp failed
            exit(ERR_EXEC_CMD);
        }
    }
    else
    {
        waitpid(pid, &status, 0); // Wait for the child process to complete
        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status); // Return the exit status of the child
        }
        else
        {
            return ERR_EXEC_CMD;
        }
    }
    return OK;
}


int execute_pipeline(command_list_t *clist)
{
    if (clist->num == 0)
    {
        fprintf(stderr, CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }

    int pipes[CMD_MAX - 1][2]; // Pipes for each command in the pipeline
    pid_t pids[CMD_MAX];       // Child processes

    // Create pipes for each command in the pipeline
    for (int i = 0; i < clist->num - 1; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("pipe");
            return ERR_MEMORY;
        }
    }

    // Execute each command in the pipeline
    for (int i = 0; i < clist->num; i++)
    {
        pids[i] = fork();

        if (pids[i] == -1)
        {
            perror("fork");
            // Close all previously created pipes
            for (int j = 0; j < i; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return ERR_MEMORY;
        }

        if (pids[i] == 0)
        { // Child process
            // Redirect input if it's not the first command
            if (i > 0)
            {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
                {
                    perror("dup2");
                    exit(ERR_EXEC_CMD);
                }
            }

            // Redirect output if it's not the last command
            if (i < clist->num - 1)
            {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1)
                {
                    perror("dup2");
                    exit(ERR_EXEC_CMD);
                }
            }

            // Close all pipes in the child process
            for (int j = 0; j < clist->num - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Check for built-in commands first
            Built_In_Cmds bi_status = exec_built_in_cmd(&clist->commands[i]);
            if (bi_status == BI_EXECUTED)
            {
                exit(OK); // Exit with success if built-in command was executed
            }
            else if (bi_status == BI_CMD_EXIT)
            {
                exit(OK_EXIT); // Exit with special code for exit command
            }

            // Execute the command
            if (execvp(clist->commands[i].argv[0], clist->commands[i].argv) < 0)
            {
                perror("execvp");
                exit(ERR_EXEC_CMD);
            }
        }
    }

    // Parent process: close all pipe ends
    for (int i = 0; i < clist->num - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    int exit_status = OK;
    for (int i = 0; i < clist->num; i++)
    {
        int status;
        waitpid(pids[i], &status, 0);
        
        // Check if any child exited with OK_EXIT status
        if (WIFEXITED(status) && WEXITSTATUS(status) == OK_EXIT)
        {
            exit_status = OK_EXIT;
        }
    }

    return exit_status;
}


int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    // Initialize command list
    clist->num = 0;
    
    // Make a copy of cmd_line to avoid modifying the original
    char *cmd_copy = strdup(cmd_line);
    if (cmd_copy == NULL)
    {
        return ERR_MEMORY;
    }
    
    char *saveptr;
    // Split the command line by pipe character
    char *token = strtok_r(cmd_copy, "|", &saveptr);
    while (token != NULL && clist->num < CMD_MAX)
    {
        // Create a temporary working copy of the token
        char *tmp = strdup(token);
        if (tmp == NULL)
        {
            free(cmd_copy);
            return ERR_MEMORY;
        }
        
        // Trim leading and trailing spaces
        char *start = tmp;
        while (*start && isspace(*start)) start++;
        
        char *end = start + strlen(start) - 1;
        while (end > start && isspace(*end)) *end-- = '\0';
        
        // Skip if empty after trimming
        if (*start == '\0')
        {
            free(tmp);
            token = strtok_r(NULL, "|", &saveptr);
            continue;
        }
        
        // Allocate memory for the command buffer
        if (alloc_cmd_buff(&clist->commands[clist->num]) != OK)
        {
            free(tmp);
            free(cmd_copy);
            return ERR_MEMORY;
        }
        
        // Build the command buffer
        if (build_cmd_buff(start, &clist->commands[clist->num]) != OK)
        {
            free(tmp);
            free(cmd_copy);
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        
        clist->num++;
        free(tmp);
        token = strtok_r(NULL, "|", &saveptr);
    }
    
    free(cmd_copy);
    
    // Check if we have too many commands
    if (token != NULL && clist->num >= CMD_MAX)
    {
        return ERR_TOO_MANY_COMMANDS;
    }
    
    return OK;
}










