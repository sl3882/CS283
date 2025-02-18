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

int exec_cmd(cmd_buff_t *cmd)
{
    pid_t pid;
    int status;
    // Check if the command is "pwd" and ensure no arguments are passed to it
    if (strcmp(cmd->argv[0], "pwd") == 0 && cmd->argc > 1)
    {
        // If there are extra arguments for pwd, print a warning and return an error
        fprintf(stderr, "pwd: ignoring non-option arguments\n");
        return ERR_EXEC_CMD;
    }
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

int parse_input(char *cmd_line, cmd_buff_t *cmd_buff)
{
    clear_cmd_buff(cmd_buff);
    char *token;
    bool in_quotes = false;
    char *buffer = cmd_buff->_cmd_buffer;
    int index = 0;

    while (isspace((unsigned char)*cmd_line))
        cmd_line++;

    for (int i = 0; cmd_line[i] != '\0'; i++)
    {
        if (cmd_line[i] == '"')
        {
            in_quotes = !in_quotes;
        }
        else if (isspace((unsigned char)cmd_line[i]) && !in_quotes)
        {
            if (index > 0 && !isspace((unsigned char)buffer[index - 1]))
            {
                buffer[index++] = '\0';
            }
        }
        else
        {
            buffer[index++] = cmd_line[i];
        }
    }

    buffer[index] = '\0';

    char *ptr = buffer;
    while (*ptr)
    {
        cmd_buff->argv[cmd_buff->argc++] = ptr;
        ptr += strlen(ptr) + 1;
        if (cmd_buff->argc >= CMD_ARGV_MAX - 1)
        {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
    }

    cmd_buff->argv[cmd_buff->argc] = NULL;

    return (cmd_buff->argc == 0) ? WARN_NO_CMDS : OK;
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