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
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    // Check if the command is "cd"
    if (strcmp(cmd->argv[0], "cd") == 0) {
        // If there's an argument provided for cd
        if (cmd->argc > 1) {
            // Change directory using chdir()
            if (chdir(cmd->argv[1]) != 0) {  // Added missing closing parenthesis and check for error
                // Print error message if chdir fails
                perror("cd");
            }
        } 

        // Return that we executed a built-in command
        return BI_EXECUTED;
    }
    // Handle other built-in commands like exit
    else if (strcmp(cmd->argv[0], EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    }
    else if (strcmp(cmd->argv[0], "dragon") == 0) {
        return BI_CMD_DRAGON;
    }
    
    // Not a built-in command
    return BI_NOT_BI;
}


int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];
    cmd_buff_t cmd_buff;

    // Initialize the command buffer
    if (alloc_cmd_buff(&cmd_buff) != OK) {
        fprintf(stderr, "Failed to allocate command buffer\n");
        return ERR_MEMORY;
    }

    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        // Remove the trailing newline
        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        // Parse the command line into the command buffer
        if (build_cmd_buff(cmd_line, &cmd_buff) != OK) {
            fprintf(stderr, "Failed to parse command\n");
            continue;
        }

        // Execute built-in commands
        Built_In_Cmds result = exec_built_in_cmd(&cmd_buff);
        if (result == BI_EXECUTED) {
            // Built-in command executed successfully
            continue;
        } else if (result == BI_CMD_EXIT) {
            // Exit the shell
            break;
        } else if (result == BI_CMD_DRAGON) {
            // Handle the dragon command
            printf("%s", dragon.txt);
            continue;
        }

        // Execute external commands
        if (exec_cmd(&cmd_buff) != OK) {
            fprintf(stderr, "Failed to execute command\n");
        }
    }

    // Free the command buffer
    free_cmd_buff(&cmd_buff);
    return OK;
}