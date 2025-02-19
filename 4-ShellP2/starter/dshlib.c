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

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    // Check if the command is "cd"
    if (strcmp(cmd->argv[0], "cd") == 0)
    {
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
        // Return that we executed a built-in command
        return BI_EXECUTED;
    }
    // Handle other built-in commands like exit
    else if (strcmp(cmd->argv[0], EXIT_CMD) == 0)
    {
        return BI_CMD_EXIT;
    }
    else if (strcmp(cmd->argv[0], "dragon") == 0)
    {
        return BI_CMD_DRAGON;
    }
    // Not a built-in command
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

int parse_input(char *cmd_line, cmd_buff_t *cmd_buff)
{
    clear_cmd_buff(cmd_buff); // Clear the command buffer

    while (isspace((unsigned char)*cmd_line))
        cmd_line++; // Skip leading whitespace

    if (*cmd_line == '\0')
        return WARN_NO_CMDS; // Return warning if the line is empty

    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1); // Copy the command line for tokenization
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';             // Null-terminate the command buffer

    if (strncmp(cmd_line, "echo", 4) == 0 && (isspace((unsigned char)cmd_line[4]) || cmd_line[4] == '\0'))
    {
        cmd_buff->argv[0] = "echo"; // Handle echo command specially
        cmd_buff->argc = 1;

        char *arg_start = cmd_line + 4; // Skip "echo" and exactly one whitespace
        while (isspace((unsigned char)*arg_start))
            arg_start++;

        if (*arg_start != '\0')
        {
            if (*arg_start == '"')
            {
                char *end_quote = strchr(arg_start + 1, '"'); // Find the closing quote
                if (end_quote != NULL)
                {
                    arg_start++;                                                        // Skip opening quote
                    *end_quote = '\0';                                                  // Remove closing quote
                    cmd_buff->argv[1] = cmd_buff->_cmd_buffer + (arg_start - cmd_line); // Copy everything between the quotes
                    strcpy(cmd_buff->argv[1], arg_start);
                    cmd_buff->argc = 2;
                }
            }
            else
            {
                cmd_buff->argv[1] = cmd_buff->_cmd_buffer + (arg_start - cmd_line); // No quotes, treat rest of line as single argument
                strcpy(cmd_buff->argv[1], arg_start);
                cmd_buff->argc = 2;
            }
        }
    }
    else
    {
        char *token = strtok(cmd_buff->_cmd_buffer, " \t"); // For non-echo commands, use standard tokenization
        while (token != NULL && cmd_buff->argc < CMD_ARGV_MAX - 1)
        {
            cmd_buff->argv[cmd_buff->argc++] = token;
            token = strtok(NULL, " \t");
        }
    }
    cmd_buff->argv[cmd_buff->argc] = NULL; // Null-terminate the argument vector
    return OK;
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    clear_cmd_buff(cmd_buff); // Clear the command buffer

    char *token = strtok(cmd_line, " "); // Tokenize the command line
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

int exec_local_cmd_loop()
{
    char cmd_line[SH_CMD_MAX]; // Buffer to store the command line input
    cmd_buff_t cmd_buff;       // Command buffer structure

    if (alloc_cmd_buff(&cmd_buff) != OK)
    {
        fprintf(stderr, "Failed to allocate command buffer\n"); // Print error message if allocation fails
        return ERR_MEMORY;                                      // Return memory error
    }
    while (1)
    {
        printf("%s", SH_PROMPT); // Print the shell prompt
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL)
        {
            printf("\n"); // Print newline if fgets fails
            break;        // Exit the loop
        }

        cmd_line[strcspn(cmd_line, "\n")] = '\0'; // Remove the trailing newline

        if (parse_input(cmd_line, &cmd_buff) != OK)
        {
            fprintf(stderr, "Failed to parse command\n"); // Print error message if parsing fails
            continue;                                     // Continue to the next iteration
        }

        Built_In_Cmds result = exec_built_in_cmd(&cmd_buff); // Execute built-in commands
        if (result == BI_EXECUTED)
        {
            continue; // Continue to the next iteration if built-in command executed
        }
        else if (result == BI_CMD_EXIT)
        {
            break; // Exit the loop if exit command
        }
        else if (result == BI_CMD_DRAGON)
        {
            printf("%s", dragon_txt); // Print dragon text if dragon command
            continue;                 // Continue to the next iteration
        }

        int exec_status = exec_cmd(&cmd_buff); // Execute external commands
        if (exec_status != OK)
        {
            fprintf(stderr, "Failed to execute command\n"); // Print error message if execution fails
        }
    }

    free_cmd_buff(&cmd_buff); // Free the command buffer
    return OK;                // Return OK
}