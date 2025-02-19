#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"
#include "dragon.txt"

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 *
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 *
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 *
 *   Also, use the constants in the dshlib.h in this code.
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 *
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 *
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 *
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */

// Defines the return type for built-in commands.
typedef enum Built_In_Cmds
{
    BI_NOT_BI = 0, // Not a built-in command.
    BI_EXECUTED,   // Built-in command executed successfully.
    BI_CMD_EXIT,   // Built-in exit command.
    BI_CMD_DRAGON  // Built-in dragon command.
} Built_In_Cmds;

// Structure to hold command information.
typedef struct cmd_buff_t
{
    char *_cmd_buffer;        // Buffer to store the command line.
    char *argv[CMD_ARGV_MAX]; // Array of arguments.
    int argc;                 // Number of arguments.
} cmd_buff_t;

// Executes built-in commands.
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    // Check if the command is "cd".
    if (strcmp(cmd->argv[0], "cd") == 0)
    {
        // Check if there is an argument for cd.
        if (cmd->argc > 1)
        {
            // Change the current directory.
            if (chdir(cmd->argv[1]) != 0)
            {
                perror("cd"); // Print error message if chdir fails.
            }
        }
        return BI_EXECUTED; // Return BI_EXECUTED if cd is successful.
    }
    // Check if the command is "exit".
    else if (strcmp(cmd->argv[0], EXIT_CMD) == 0)
    {
        return BI_CMD_EXIT; // Return BI_CMD_EXIT if exit is called.
    }
    // Check if the command is "dragon".
    else if (strcmp(cmd->argv[0], "dragon") == 0)
    {
        return BI_CMD_DRAGON; // Return BI_CMD_DRAGON if dragon is called.
    }
    return BI_NOT_BI; // Return BI_NOT_BI if it's not a built-in command.
}

// Allocates memory for the command buffer.
int alloc_cmd_buff(cmd_buff_t *cmd_buff)
{
    // Allocate memory for the command buffer.
    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX * sizeof(char));
    if (cmd_buff->_cmd_buffer == NULL)
    {
        return ERR_MEMORY; // Return ERR_MEMORY if allocation fails.
    }
    cmd_buff->argc = 0; // Initialize argument count to 0.
    // Initialize argument array to NULL.
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL;
    }
    return OK; // Return OK if allocation is successful.
}

// Executes a command using fork and execvp.
int exec_cmd(cmd_buff_t *cmd)
{
    pid_t pid;
    int status;
    pid = fork(); // Create a child process.
    if (pid < 0)
    {
        perror("fork");      // Print error message if fork fails.
        return ERR_EXEC_CMD; // Return ERR_EXEC_CMD if fork fails.
    }
    else if (pid == 0)
    {
        // Child process.
        if (execvp(cmd->argv[0], cmd->argv) < 0)
        {
            perror("execvp");   // Print error message if execvp fails.
            exit(ERR_EXEC_CMD); // Exit the child process with an error code.
        }
    }
    else
    {
        // Parent process.
        waitpid(pid, &status, 0); // Wait for the child process to finish.
        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status); // Return the exit status of the child process.
        }
        else
        {
            return ERR_EXEC_CMD; // Return ERR_EXEC_CMD if the child process exited abnormally.
        }
    }
    return OK; // Should not reach here.
}

// Frees the memory allocated for the command buffer.
int free_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (cmd_buff->_cmd_buffer != NULL)
    {
        free(cmd_buff->_cmd_buffer); // Free the command buffer.
        cmd_buff->_cmd_buffer = NULL;
    }
    cmd_buff->argc = 0; // Reset argument count.
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL; // Reset argument array.
    }
    return OK; // Return OK after freeing the memory.
}

// Clears the command buffer.
int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->argc = 0; // Reset argument count.
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL; // Reset argument array.
    }
    if (cmd_buff->_cmd_buffer != NULL)
    {
        cmd_buff->_cmd_buffer[0] = '\0'; // Clear the command buffer.
    }
    return OK; // Return OK after clearing.
}

// Parses the input command line.
int parse_input(char *cmd_line, cmd_buff_t *cmd_buff)
{
    clear_cmd_buff(cmd_buff);                 // Clear the command buffer.
    while (isspace((unsigned char)*cmd_line)) // Skip leading spaces.
        cmd_line++;
    if (*cmd_line == '\0')                                    // Check for empty command.
        return WARN_NO_CMDS;                                  // Return WARN_NO_CMDS if the command is empty.
    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1); // Copy the command line to the buffer.
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';             // Null-terminate the buffer.

    // Handle "echo" command specifically.
    if (strncmp(cmd_line, "echo", 4) == 0 && (isspace((unsigned char)cmd_line[4]) || cmd_line[4] == '\0'))
    {
        cmd_buff->argv[0] = "echo";                // Set the command to "echo".
        cmd_buff->argc = 1;                        // Initialize argument count.
        char *arg_start = cmd_line + 4;            // Point to the start of the argument.
        while (isspace((unsigned char)*arg_start)) // Skip spaces after "echo".
            arg_start++;
        if (*arg_start != '\0') // Check if there is an argument.
        {
            if (*arg_start == '"') // Handle quoted arguments.
            {
                char *end_quote = strchr(arg_start + 1, '"'); // Find the closing quote.
                if (end_quote != NULL)
                {
                    arg_start++;                                                        // Move past the opening quote.
                    *end_quote = '\0';                                                  // Null-terminate the argument.
                    cmd_buff->argv[1] = cmd_buff->_cmd_buffer + (arg_start - cmd_line); // Store the argument.
                    strcpy(cmd_buff->argv[1], arg_start);                               // Copy argument into argv[1]
                    cmd_buff->argc = 2;                                                 // Set argument count.
                }
            }
            else
            {
                cmd_buff->argv[1] = cmd_buff->_cmd_buffer + (arg_start - cmd_line); // Store the argument.
                strcpy(cmd_buff->argv[1], arg_start);                               // Copy argument into argv[1]
                cmd_buff->argc = 2;                                                 // Set argument count.
            }
        }
    }
    else
    {
        // Parse other commands using strtok.
        char *token = strtok(cmd_buff->_cmd_buffer, " \t"); // Tokenize the command line.
        while (token != NULL && cmd_buff->argc < CMD_ARGV_MAX - 1)
        {
            cmd_buff->argv[cmd_buff->argc++] = token; // Store the token as an argument.
            token = strtok(NULL, " \t");              // Get the next token.
        }
    }
    cmd_buff->argv[cmd_buff->argc] = NULL; // Null-terminate the argument array.
    return OK;                             // Return OK after parsing.
}

// Builds the command buffer (alternative parsing method).
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    clear_cmd_buff(cmd_buff);            // Clear the command buffer.
    char *token = strtok(cmd_line, " "); // Tokenize the command line by spaces.
    while (token != NULL)
    {
        if (cmd_buff->argc >= CMD_ARGV_MAX - 1)
        {
            return ERR_CMD_OR_ARGS_TOO_BIG; // Return error if too many arguments.
        }
        cmd_buff->argv[cmd_buff->argc] = token; // Store the token as an argument.
        cmd_buff->argc++;                       // Increment argument count.
        token = strtok(NULL, " ");              // Get the next token.
    }
    cmd_buff->argv[cmd_buff->argc] = NULL; // Null-terminate the argument array.
    if (cmd_buff->argc == 0)
    {
        return WARN_NO_CMDS; // Return warning if no commands are found.
    }
    return OK; // Return OK if command buffer is built successfully.
}

// Main loop for executing commands.
int exec_local_cmd_loop()
{
    char cmd_line[SH_CMD_MAX]; // Buffer to store the command line.
    cmd_buff_t cmd_buff;       // Command buffer structure.

    // Allocate memory for the command buffer.
    if (alloc_cmd_buff(&cmd_buff) != OK)
    {
        fprintf(stderr, "Failed to allocate command buffer\n");
        return ERR_MEMORY; // Return error if allocation fails.
    }

    // Main command loop.
    while (1)
    {
        printf("%s", SH_PROMPT);                        // Print the shell prompt.
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) // Read a line from input.
        {
            printf("\n"); // Print a newline if fgets returns NULL (e.g., Ctrl+D).
            break;        // Exit the loop if fgets returns NULL.
        }

        cmd_line[strcspn(cmd_line, "\n")] = '\0'; // Remove the trailing newline character.

        // Parse the input command line.
        if (parse_input(cmd_line, &cmd_buff) != OK)
        {
            fprintf(stderr, "Failed to parse command\n"); // Print error if parsing fails.
            continue;                                     // Continue to the next iteration of the loop.
        }

        // Execute built-in commands.
        Built_In_Cmds result = exec_built_in_cmd(&cmd_buff);
        if (result == BI_EXECUTED)
        {
            continue; // Continue to the next iteration if it's a built-in command.
        }
        else if (result == BI_CMD_EXIT)
        {
            break; // Exit the loop if the command is "exit".
        }
        else if (result == BI_CMD_DRAGON)
        {
            printf("%s", dragon_txt); // Print the dragon text if the command is "dragon".
            continue;
        }

        // Execute external commands.
        int exec_status = exec_cmd(&cmd_buff);
        if (exec_status != OK)
        {
            fprintf(stderr, "Failed to execute command\n"); // Print error if execution fails.
        }
    }

    free_cmd_buff(&cmd_buff); // Free the command buffer memory.
    return OK;                // Return OK when the loop finishes.
}