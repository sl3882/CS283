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

// Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
// {
//     // Check if the command is "cd"
//     if (strcmp(cmd->argv[0], "cd") == 0)
//     {
//         // If there's an argument provided for cd
//         if (cmd->argc > 1)
//         {
//             // Change directory using chdir()
//             if (chdir(cmd->argv[1]) != 0)
//             {
//                 // Print error message if chdir fails
//                 perror("cd");
//             }
//         }

//         // Return that we executed a built-in command
//         return BI_EXECUTED;
//     }
//     // Handle other built-in commands like exit
//     else if (strcmp(cmd->argv[0], EXIT_CMD) == 0)
//     {
//         return BI_CMD_EXIT;
//     }
//     else if (strcmp(cmd->argv[0], "dragon") == 0)
//     {
//         return BI_CMD_DRAGON;
//     }

//     // Not a built-in command
//     return BI_NOT_BI;
// }


Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    if (strcmp(cmd->argv[0], "cd") == 0)
    {
        if (cmd->argc > 1)
        {
            if (chdir(cmd->argv[1]) != 0)
            {
                perror("cd");
            }
        }
        return BI_EXECUTED;
    }
    else if (strcmp(cmd->argv[0], EXIT_CMD) == 0)
    {
        return BI_CMD_EXIT;
    }
    else if (strcmp(cmd->argv[0], "dragon") == 0)
    {
        return BI_CMD_DRAGON;
    }

    return BI_NOT_BI;
}

int alloc_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX * sizeof(char));
    if (cmd_buff->_cmd_buffer == NULL)
    {
        return ERR_MEMORY;
    }

    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL;
    }

    return OK;
}

int exec_cmd(cmd_buff_t *cmd)
{
    pid_t pid;
    int status;

    // Fork a new process
    pid = fork();
    if (pid < 0)
    {
        // Fork failed
        perror("fork");
        return ERR_EXEC_CMD;
    }
    else if (pid == 0)
    {
        // Child process
        if (execvp(cmd->argv[0], cmd->argv) < 0)
        {
            // execvp failed
            perror("execvp");
            exit(ERR_EXEC_CMD);
        }
    }
    else
    {
        // Parent process
        // Wait for the child process to complete
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            // Return the exit status of the child
            return WEXITSTATUS(status);
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
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }

    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL;
    }

    return OK;
}

int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->argc = 0;

    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL;
    }

    if (cmd_buff->_cmd_buffer != NULL)
    {
        cmd_buff->_cmd_buffer[0] = '\0';
    }

    return OK;
}






// int parse_input(char *cmd_line, cmd_buff_t *cmd_buff) {
//     clear_cmd_buff(cmd_buff);

//     // Skip leading whitespace
//     while (isspace((unsigned char)*cmd_line))
//         cmd_line++;

//     // If after skipping whitespace the line is empty, return WARN_NO_CMDS
//     if (*cmd_line == '\0')
//         return WARN_NO_CMDS;

//     // Copy the command line for tokenization
//     strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1);
//     cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';

//     // First, check if this is an echo command
//     if (strncmp(cmd_line, "echo", 4) == 0 && (isspace((unsigned char)cmd_line[4]) || cmd_line[4] == '\0')) {
//         // Handle echo command specially
//         cmd_buff->argv[0] = "echo";
//         cmd_buff->argc = 1;

//         // Skip "echo" and exactly one whitespace
//         char *arg_start = cmd_line + 4;
//         while (isspace((unsigned char)*arg_start))
//             arg_start++;

//         // If there's anything after echo, add it as a single argument
//         if (*arg_start != '\0') {
//             // Check if the argument starts with a quote
//             if (*arg_start == '"') {
//                 // Find the closing quote
//                 char *end_quote = strchr(arg_start + 1, '"');
//                 if (end_quote != NULL) {
//                     // Copy everything between the quotes
//                     arg_start++; // Skip opening quote
//                     *end_quote = '\0'; // Remove closing quote
//                     cmd_buff->argv[1] = cmd_buff->_cmd_buffer + (arg_start - cmd_line);
//                     strcpy(cmd_buff->argv[1], arg_start);
//                     cmd_buff->argc = 2;
//                 }
//             } else {
//                 // No quotes, treat rest of line as single argument
//                 cmd_buff->argv[1] = cmd_buff->_cmd_buffer + (arg_start - cmd_line);
//                 strcpy(cmd_buff->argv[1], arg_start);
//                 cmd_buff->argc = 2;
//             }
//         }
//     } else {
//         // For non-echo commands, use standard tokenization
//         char *token = strtok(cmd_buff->_cmd_buffer, " \t");
//         while (token != NULL && cmd_buff->argc < CMD_ARGV_MAX - 1) {
//             cmd_buff->argv[cmd_buff->argc++] = token;
//             token = strtok(NULL, " \t");
//         }
//     }

//     cmd_buff->argv[cmd_buff->argc] = NULL;
//     return OK;
// }





int parse_input(char *cmd_line, cmd_buff_t *cmd_buff) {
    clear_cmd_buff(cmd_buff);

    // Skip leading whitespace
    while (isspace((unsigned char)*cmd_line))
        cmd_line++;

    // If after skipping whitespace the line is empty, return WARN_NO_CMDS
    if (*cmd_line == '\0')
        return WARN_NO_CMDS;

    // Copy the command line into our buffer
    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';

    // Use our buffer for tokenization
    char *token = strtok(cmd_buff->_cmd_buffer, " \t");
    while (token != NULL && cmd_buff->argc < CMD_ARGV_MAX - 1) {
        cmd_buff->argv[cmd_buff->argc++] = token;
        token = strtok(NULL, " \t");
    }

    cmd_buff->argv[cmd_buff->argc] = NULL;
    return OK;
}


int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    clear_cmd_buff(cmd_buff);

    char *token = strtok(cmd_line, " ");
    while (token != NULL)
    {
        if (cmd_buff->argc >= CMD_ARGV_MAX - 1)
        {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        cmd_buff->argv[cmd_buff->argc] = token;
        cmd_buff->argc++;
        token = strtok(NULL, " ");
    }

    cmd_buff->argv[cmd_buff->argc] = NULL;
    if (cmd_buff->argc == 0)
    {
        return WARN_NO_CMDS;
    }

    return OK;
}

int exec_local_cmd_loop()
{
    char cmd_line[SH_CMD_MAX];
    cmd_buff_t cmd_buff;

    if (alloc_cmd_buff(&cmd_buff) != OK)
    {
        fprintf(stderr, "Failed to allocate command buffer\n");
        return ERR_MEMORY;
    }

    while (1)
    {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL)
        {
            printf("\n");
            break;
        }

        // Remove the trailing newline
        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        // Trim leading and trailing spaces and handle quoted strings
        if (parse_input(cmd_line, &cmd_buff) != OK)
        {
            fprintf(stderr, "Failed to parse command\n");
            continue;
        }

        // Execute built-in commands
        Built_In_Cmds result = exec_built_in_cmd(&cmd_buff);
        if (result == BI_EXECUTED)
        {
            continue;
        }
        else if (result == BI_CMD_EXIT)
        {
            break;
        }
        else if (result == BI_CMD_DRAGON)
        {
            printf("%s", dragon_txt);
            continue;
        }

        // Execute external commands
        if (exec_cmd(&cmd_buff) != OK)
        {
            fprintf(stderr, "Failed to execute command\n");
        }
    }

    free_cmd_buff(&cmd_buff);
    return OK;
}


