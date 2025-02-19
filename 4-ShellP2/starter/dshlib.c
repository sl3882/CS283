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
    char cmd_buff[SH_CMD_MAX];
    cmd_buff_t cmd;
    int rc;

    while (1)
    {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL)
        {
            printf("\n");
            break;
        }

        // Remove the trailing newline character
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // Trim leading and trailing spaces
        char *trimmed_cmd = trim_spaces(cmd_buff);

        // Clear the command buffer
        clear_cmd_buff(&cmd);

        // Build the command buffer
        rc = build_cmd_buff(trimmed_cmd, &cmd);
        if (rc == WARN_NO_CMDS)
        {
            printf(CMD_WARN_NO_CMD);
            continue;
        }
        else if (rc == ERR_TOO_MANY_COMMANDS)
        {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        }
        else if (rc != OK)
        {
            printf("error: failed to build command buffer\n");
            continue;
        }

        // Check for built-in commands
        Built_In_Cmds bi_cmd = match_command(cmd.argv[0]);
        if (bi_cmd == BI_CMD_EXIT)
        {
            break;
        }
        else if (bi_cmd != BI_NOT_BI)
        {
            exec_built_in_cmd(&cmd);
            continue;
        }

        // Execute the command
        rc = exec_cmd(&cmd);
        if (rc != OK)
        {
            printf("error: failed to execute command\n");
        }
    }

    return OK;
}

Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    if (strcmp(cmd->argv[0], "cd") == 0)
    {
        if (cmd->argc == 1)
        {
            // No arguments, do nothing
            return BI_EXECUTED;
        }
        else if (cmd->argc == 2)
        {
            // Change directory to the provided argument
            if (chdir(cmd->argv[1]) != 0)
            {
                perror("cd");
            }
            return BI_EXECUTED;
        }
        else
        {
            printf("cd: too many arguments\n");
            return BI_EXECUTED;
        }
    }
    else if (strcmp(cmd->argv[0], EXIT_CMD) == 0)
    {
        return BI_CMD_EXIT;
    }
    // Add other built-in commands here

    return BI_NOT_BI;
}

// Function to trim leading and trailing spaces
char *trim_spaces(char *str)
{
    char *end;

    // Trim leading spaces
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0)  // All spaces?
        return str;

    // Trim trailing spaces
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = 0;

    return str;
}

// Function to parse the command line into cmd_buff
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    bool in_quotes = false;
    char *cmd_ptr = cmd_line;
    cmd_buff->argc = 0;

    // Allocate memory for the command buffer
    cmd_buff->_cmd_buffer = malloc(strlen(cmd_line) + 1);
    if (cmd_buff->_cmd_buffer == NULL)
    {
        return ERR_MEMORY;
    }
    strcpy(cmd_buff->_cmd_buffer, cmd_line);

    while (*cmd_ptr)
    {
        // Handle quoted strings
        if (*cmd_ptr == '"')
        {
            in_quotes = !in_quotes;
            cmd_ptr++;
            continue;
        }

        // Split tokens by spaces
        if (!in_quotes && isspace((unsigned char)*cmd_ptr))
        {
            *cmd_ptr = '\0';
            cmd_ptr++;
            continue;
        }

        // Add token to argv
        if (cmd_buff->argc < CMD_ARGV_MAX - 1)
        {
            cmd_buff->argv[cmd_buff->argc++] = cmd_ptr;
        }
        else
        {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }

        // Move to the next token
        while (*cmd_ptr && (!isspace((unsigned char)*cmd_ptr) || in_quotes))
        {
            cmd_ptr++;
        }
    }

    cmd_buff->argv[cmd_buff->argc] = NULL;
    return OK;
}

// Function to clear the command buffer
int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (cmd_buff->_cmd_buffer)
    {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    cmd_buff->argc = 0;
    memset(cmd_buff->argv, 0, sizeof(cmd_buff->argv));
    return OK;
}