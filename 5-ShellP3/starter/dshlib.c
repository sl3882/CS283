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
    char cmd_line[SH_CMD_MAX];
    command_list_t cmd_list;
    int result;
    
    while (1) {
        // Print the shell prompt
        printf("%s", SH_PROMPT);
        
        // Get user input
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        
        // Remove the trailing newline character
        cmd_line[strcspn(cmd_line, "\n")] = '\0';
        
        // Check if user wants to exit
        if (strcmp(cmd_line, EXIT_CMD) == 0) {
            return OK_EXIT;
        }
        
        // Skip empty commands
        if (strlen(cmd_line) == 0) {
            continue;
        }
        
        // Parse the command line into a command list
        result = build_cmd_list(cmd_line, &cmd_list);
        
        // Handle parsing results
        if (result == WARN_NO_CMDS) {
            printf(CMD_WARN_NO_CMD);
            continue;
        } else if (result == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        } else if (result == ERR_MEMORY) {
            printf("Error: Memory allocation failed\n");
            return ERR_MEMORY;
        } else if (result != OK) {
            printf("Error: Command parsing failed\n");
            continue;
        }
        
        // Check for built-in commands before executing pipeline
        Built_In_Cmds bi_result = exec_built_in_cmd(&cmd_list.commands[0]);
        
        if (bi_result == BI_CMD_EXIT) {
            free_cmd_list(&cmd_list);
            return OK_EXIT;
        } else if (bi_result == BI_EXECUTED) {
            // Built-in command was executed successfully
            free_cmd_list(&cmd_list);
            continue;
        }
        
        // Execute the commands with pipes if we have multiple commands
        if (cmd_list.num > 1) {
            result = execute_pipeline(&cmd_list);
            if (result != OK) {
                printf("Error executing piped commands\n");
            }
        } else {
            // Execute a single command (no pipes)
            result = exec_cmd(&cmd_list.commands[0]);
            if (result != OK) {
                printf("Error executing command\n");
            }
        }
        
        // Free any allocated memory in the command list
        free_cmd_list(&cmd_list);
    }
    
    return OK;
}




int execute_pipeline(command_list_t *clist)
{
    if (clist == NULL || clist->num <= 0) {
        return WARN_NO_CMDS;
    }
    
    // Array to store process IDs for each command
    pid_t pids[CMD_MAX];
    
    // We need n-1 pipes for n commands
    int pipes[CMD_MAX - 1][2];
    
    // Create all necessary pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            // Close any pipes already created
            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return ERR_EXEC_CMD;
        }
    }
    
    // Create and connect all processes in the pipeline
    for (int i = 0; i < clist->num; i++) {
        // Fork a child process
        pids[i] = fork();
        
        if (pids[i] < 0) {
            // Fork failed
            perror("fork");
            
            // Clean up all pipes
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Kill any children already created
            for (int j = 0; j < i; j++) {
                kill(pids[j], SIGTERM);
                waitpid(pids[j], NULL, 0);
            }
            
            return ERR_EXEC_CMD;
        } else if (pids[i] == 0) {
            // Child process
            
            // Connect pipes to stdin/stdout as needed
            
            // If not the first command, connect input from previous pipe
            if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2 (stdin)");
                    exit(EXIT_FAILURE);
                }
            }
            
            // If not the last command, connect output to next pipe
            if (i < clist->num - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2 (stdout)");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Close all pipe file descriptors in the child
            // This is important to prevent descriptor leaks
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            if (execvp(clist->commands[i].argv[0], clist->commands[i].argv) == -1) {
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            
            // This should never be reached
            exit(EXIT_FAILURE);
        }
    }
    
    // Parent process
    
    // Close all pipe file descriptors in the parent
    // This is crucial - without this, reading processes won't see EOF
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all child processes to complete
    int status;
    for (int i = 0; i < clist->num; i++) {
        if (waitpid(pids[i], &status, 0) == -1) {
            perror("waitpid");
            return ERR_EXEC_CMD;
        }
        
        // Check if the command executed successfully
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            // Command failed or terminated abnormally
            return ERR_EXEC_CMD;
        }
    }
    
    return OK;
}