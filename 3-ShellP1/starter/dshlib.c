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

int build_cmd_list(char *cmd_line, command_list_t *clist) {
    memset(clist, 0, sizeof(command_list_t));
    
    // Check for empty command line
    if (!cmd_line || strlen(cmd_line) == 0) {
        printf(CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }

    // // Skip leading spaces in the initial command line
    // while (*cmd_line && isspace((unsigned char)*cmd_line)) {
    //     cmd_line++;
    // }

    if (*cmd_line == '\0') {
        printf(CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }

    char cmd_copy[SH_CMD_MAX];
    strncpy(cmd_copy, cmd_line, SH_CMD_MAX - 1);
    cmd_copy[SH_CMD_MAX - 1] = '\0';

    char *cmd_ptr = cmd_copy;
    char *next_cmd = cmd_ptr;
    
    while (next_cmd && *next_cmd) {
        if (clist->num >= CMD_MAX) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            return ERR_TOO_MANY_COMMANDS;
        }

        // Find next pipe
        char *pipe = strchr(next_cmd, PIPE_CHAR);
        if (pipe) {
            *pipe = '\0';
        }

        // // Skip leading spaces
        // while (*next_cmd && isspace((unsigned char)*next_cmd)) {
        //     next_cmd++;
        // }

        if (*next_cmd) {
            char *space = strchr(next_cmd, SPACE_CHAR);
            
            if (space) {
                *space = '\0';
                // Skip leading spaces in arguments
                char *args = space + 1;
                while (*args && isspace((unsigned char)*args)) {
                    args++;
                }
                
                // Remove trailing spaces from arguments
                char *end = args + strlen(args) - 1;
                while (end > args && isspace((unsigned char)*end)) {
                    *end = '\0';
                    end--;
                }

                if (strlen(next_cmd) >= EXE_MAX || strlen(args) >= ARG_MAX) {
                    return ERR_CMD_OR_ARGS_TOO_BIG;
                }
                
                strcpy(clist->commands[clist->num].exe, next_cmd);
                strcpy(clist->commands[clist->num].args, args);
            } else {
                // Remove trailing spaces from command
                char *end = next_cmd + strlen(next_cmd) - 1;
                while (end > next_cmd && isspace((unsigned char)*end)) {
                    *end = '\0';
                    end--;
                }

                if (strlen(next_cmd) >= EXE_MAX) {
                    return ERR_CMD_OR_ARGS_TOO_BIG;
                }
                
                strcpy(clist->commands[clist->num].exe, next_cmd);
                clist->commands[clist->num].args[0] = '\0';
            }
            
            clist->num++;
        }

        if (pipe) {
            next_cmd = pipe + 1;
        } else {
            break;
        }
    }

    if (clist->num == 0) {
        printf(CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }

    return OK;
}