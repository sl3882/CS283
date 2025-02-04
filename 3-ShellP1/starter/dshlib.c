#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    // Initialize the command list structure
    memset(clist, 0, sizeof(command_list_t));

    // Temporary buffer to work with (copy since strtok modifies the string)
    char cmd_copy[SH_CMD_MAX];
    strncpy(cmd_copy, cmd_line, SH_CMD_MAX);
    cmd_copy[SH_CMD_MAX - 1] = '\0'; // Ensure null termination

    // Tokenize the command line by pipes '|'
    char *cmd = strtok(cmd_copy, PIPE_STRING);
    while (cmd != NULL)
    {
        // Trim leading spaces
        while (*cmd == SPACE_CHAR) cmd++;

        // Ensure the number of commands does not exceed CMD_MAX
        if (clist->num >= CMD_MAX)
        {
            return ERR_TOO_MANY_COMMANDS;
        }

        // Extract executable and arguments
        command_t *curr_cmd = &clist->commands[clist->num];
        char *exe = strtok(cmd, " ");
        if (!exe)
        {
            return WARN_NO_CMDS;
        }

        // Ensure executable size does not exceed EXE_MAX
        if (strlen(exe) >= EXE_MAX)
        {
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        strcpy(curr_cmd->exe, exe);

        // Extract arguments (everything after the first token)
        char *args = strtok(NULL, "");
        if (args)
        {
            // Ensure argument size does not exceed ARG_MAX
            if (strlen(args) >= ARG_MAX)
            {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            strcpy(curr_cmd->args, args);
        }
        else
        {
            curr_cmd->args[0] = '\0'; // No arguments case
        }

        clist->num++;
        cmd = strtok(NULL, PIPE_STRING);
    }

    return OK;
}