#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"

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

 

 int exec_local_cmd_loop()
{
    char input[SH_CMD_MAX];  // Buffer for input command
    cmd_buff_t cmd_buff;     // Command buffer
    int rc;

    while (1)
    {
        // Print shell prompt
        printf("%s", SH_PROMPT);
        fflush(stdout);

        // Read user input
        if (fgets(input, SH_CMD_MAX, stdin) == NULL)
        {
            printf("\n");
            break;
        }

        // Trim newline character
        input[strcspn(input, "\n")] = '\0';

        rc = build_cmd_buff(input, &cmd_buff);
        
        if (rc == WARN_NO_CMDS)
        {
            printf("%s\n", CMD_WARN_NO_CMD);
            continue;
        }
        else if (rc == ERR_TOO_MANY_COMMANDS)
        {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        }

        // Execute built-in commands (exit, cd)
        Built_In_Cmds bi_cmd = exec_built_in_cmd(&cmd_buff);
        if (bi_cmd == BI_CMD_EXIT)
        {
            return OK_EXIT;
        }
        else if (bi_cmd == BI_EXECUTED)
        {
            continue;
        }

        // Execute external commands
        rc = exec_cmd(&cmd_buff);
        if (rc == ERR_EXEC_CMD)
        {
            perror("Execution failed");
        }

        free(cmd_buff._cmd_buffer);  // Free allocated memory
    }

    return OK;
}

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd_buff)
{
    if (strcmp(cmd_buff->argv[0], "exit") == 0)
    {
        return BI_CMD_EXIT;
    }
    else if (strcmp(cmd_buff->argv[0], "cd") == 0)
    {
        // No argument case: do nothing
        if (cmd_buff->argc == 1)
        {
            return BI_EXECUTED;
        }

        // Change directory
        if (chdir(cmd_buff->argv[1]) != 0)
        {
            perror("cd"); // Prints error message with reason
            return BI_RC;
        }

        return BI_EXECUTED;
    }

    return BI_NOT_BI; // Not a built-in command
}

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    char *ptr = cmd_line;
    bool in_quotes = false;
    int argc = 0;

    // Allocate space for cmd_buffer
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (!cmd_buff->_cmd_buffer)
    {
        return ERR_MEMORY;
    }

    // Tokenization
    char *token = strtok(cmd_buff->_cmd_buffer, " ");
    while (token != NULL)
    {
        if (argc >= CMD_MAX)
        {
            free(cmd_buff->_cmd_buffer);
            return ERR_TOO_MANY_COMMANDS;
        }

        // Handle quoted arguments
        if (token[0] == '"')
        {
            in_quotes = true;
            memmove(token, token + 1, strlen(token)); // Remove leading quote
        }

        if (in_quotes && token[strlen(token) - 1] == '"')
        {
            token[strlen(token) - 1] = '\0'; // Remove trailing quote
            in_quotes = false;
        }

        cmd_buff->argv[argc] = token;
        argc++;
        token = strtok(NULL, " ");
    }

    cmd_buff->argc = argc;
    cmd_buff->argv[argc] = NULL; // Null-terminate the argument list

    return (argc == 0) ? WARN_NO_CMDS : OK;
}

int exec_cmd(cmd_buff_t *cmd_buff)
{
    pid_t pid;
    int status;

    // Fork a child process
    pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        return ERR_EXEC_CMD;
    }
    else if (pid == 0) // Child process
    {
        // Execute command using execvp
        execvp(cmd_buff->argv[0], cmd_buff->argv);

        // If execvp fails, print error and exit child process
        perror("execvp failed");
        exit(ERR_EXEC_CMD);
    }
    else // Parent process
    {
        // Wait for child process to finish and get return code
        if (waitpid(pid, &status, 0) == -1)
        {
            perror("waitpid failed");
            return ERR_EXEC_CMD;
        }

        // Extract and return child exit status
        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status);
        }
        else
        {
            return ERR_EXEC_CMD;
        }
    }
}