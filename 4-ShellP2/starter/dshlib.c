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
    char *cmd_line = (char *)malloc(SH_CMD_MAX);
    if (!cmd_line)
    {
        printf("error: memory allocation failed\n");
        return ERR_MEMORY;
    }

    cmd_buff_t cmd_buff;

    while (1)
    {
        // Display shell prompt
        printf("%s", SH_PROMPT);

        // Get user input
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL)
        {
            printf("\n");
            break; // Exit loop on EOF (Ctrl+D)
        }

        // Remove trailing newline
        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        // Check for empty command
        if (strlen(cmd_line) == 0)
        {
            printf(CMD_WARN_NO_CMD);
            continue;
        }

        // Exit condition
        if (strcmp(cmd_line, EXIT_CMD) == 0)
            break;

        // Build command buffer
        int rc = build_cmd_buff(cmd_line, &cmd_buff);
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

        // Check if it's a built-in command
        Built_In_Cmds cmd_type = match_command(cmd_buff.argv[0]);
        if (cmd_type != BI_NOT_BI)
        {
            exec_built_in_cmd(&cmd_buff);
            continue;
        }

        // Execute external command
        rc = exec_cmd(&cmd_buff);
        if (rc == ERR_EXEC_CMD)
        {
            printf("error: failed to execute command\n");
        }
    }

    // Free allocated memory
    free(cmd_line);
    return OK;
}




Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    Built_In_Cmds bi_cmd = match_command(cmd->argv[0]);
    
    switch (bi_cmd) {
        case BI_CMD_CD:
            if (cmd->argc == 1) {
                // Do nothing when called with no arguments
                return BI_EXECUTED;
            } else if (cmd->argc == 2) {
                if (chdir(cmd->argv[1]) == 0) {
                    return BI_EXECUTED;
                } else {
                    perror("cd");
                    return BI_RC;
                }
            } else {
                fprintf(stderr, "cd: too many arguments\n");
                return BI_RC;
            }
        // ... other built-in commands ...
        default:
            return BI_NOT_BI;
    }
}



