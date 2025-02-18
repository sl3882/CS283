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
    char cmd_buff[SH_CMD_MAX]; // Buffer for user input
    cmd_buff_t cmd;            // Struct to hold parsed command
    int rc;                    // Return code

    // TODO IMPLEMENT MAIN LOOP

    // TODO IMPLEMENT parsing input to cmd_buff_t *cmd_buff

    // TODO IMPLEMENT if built-in command, execute builtin logic for exit, cd (extra credit: dragon)
    // the cd command should chdir to the provided directory; if no directory is provided, do nothing

    // TODO IMPLEMENT if not built-in command, fork/exec as an external command
    // for example, if the user input is "ls -l", you would fork/exec the command "ls" with the arg "-l"

    while (1)
    {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_buff, SH_CMD_MAX, stdin) == NULL)
        {
            printf("\n");
            break;
        }

        // Remove trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // Ignore empty commands
        if (strlen(cmd_buff) == 0)
        {
            printf("%s\n", CMD_WARN_NO_CMD);
            continue;
        }

        // Parse input into cmd struct
        rc = parse_cmd(cmd_buff, &cmd);
        if (rc == WARN_NO_CMDS)
        {
            printf("%s\n", CMD_WARN_NO_CMD);
            continue;
        }
        else if (rc == ERR_TOO_MANY_COMMANDS)
        {
            printf("%s\n", CMD_ERR_PIPE_LIMIT);
            continue;
        }
        else if (rc == ERR_MEMORY)
        {
            fprintf(stderr, "Memory allocation failure\n");
            return ERR_MEMORY;
        }

        // Check for built-in commands
        Built_In_Cmds builtin = exec_built_in_cmd(&cmd);
        if (builtin == BI_CMD_EXIT)
        {
            break; // Exit loop if "exit" is entered
        }
        else if (builtin == BI_CMD_CD)
        {
            continue; // Skip external execution if "cd" is executed
        }

        // Fork and execute external commands
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            continue;
        }
        if (pid == 0)
        { // Child process
            if (execvp(cmd.argv[0], cmd.argv) == -1)
            {
                printf("%s\n", CMD_ERR_EXECUTE);
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        }
        else
        { // Parent process
            int status;
            waitpid(pid, &status, 0);
        }
    }

    return OK;
}
