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




 typedef struct {
    char **args;
} Command;




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
    cmd_buff->_cmd_buffer = (char *)malloc(SH_CMD_MAX * sizeof(char)); // Allocate memory for command buffer
    if (cmd_buff->_cmd_buffer == NULL)
    {
        return ERR_MEMORY; // Return error if memory allocation fails
    }
    cmd_buff->argc = 0; // Initialize argument count to 0
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL; // Initialize argument vector to NULL
    }
    return OK; // Return OK if successful
}


int exec_cmd(cmd_buff_t *cmd)
{
    pid_t pid;  // Process ID
    int status; // Status of child process

    pid = fork(); // Fork a new process
    if (pid < 0)
    {
        perror("fork"); // Fork failed
        return ERR_EXEC_CMD;
    }
    else if (pid == 0)
    {
        if (execvp(cmd->argv[0], cmd->argv) < 0)
        {
            perror("execvp"); // execvp failed
            exit(ERR_EXEC_CMD);
        }
    }
    else
    {
        waitpid(pid, &status, 0); // Wait for the child process to complete
        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status); // Return the exit status of the child
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
        free(cmd_buff->_cmd_buffer); // Free the command buffer
        cmd_buff->_cmd_buffer = NULL;
    }
    cmd_buff->argc = 0; // Reset argument count to 0
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL; // Reset argument vector to NULL
    }
    return OK;
}






int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->argc = 0; // Reset argument count to 0
    for (int i = 0; i < CMD_ARGV_MAX; i++)
    {
        cmd_buff->argv[i] = NULL; // Reset argument vector to NULL
    }
    if (cmd_buff->_cmd_buffer != NULL)
    {
        cmd_buff->_cmd_buffer[0] = '\0'; // Clear the command buffer
    }
    return OK;
}


int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    clear_cmd_buff(cmd_buff); // Clear the command buffer

    char *token = strtok(cmd_line, " "); // Tokenize the command line
    while (token != NULL)
    {
        if (cmd_buff->argc >= CMD_ARGV_MAX - 1)
        {
            return ERR_CMD_OR_ARGS_TOO_BIG; // Return error if too many arguments
        }
        cmd_buff->argv[cmd_buff->argc] = token; // Add token to argument vector
        cmd_buff->argc++;
        token = strtok(NULL, " ");
    }
    cmd_buff->argv[cmd_buff->argc] = NULL; // Null-terminate the argument vector
    if (cmd_buff->argc == 0)
    {
        return WARN_NO_CMDS; // Return warning if no commands parsed
    }
    return OK;
}




int close_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (cmd_buff == NULL)
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






int free_cmd_list(command_list_t *cmd_lst)
{
    if (cmd_lst == NULL)
    {
        return ERR_MEMORY;
    }

    for (int i = 0; i < cmd_lst->num; i++)
    {
        free_cmd_buff(&cmd_lst->commands[i]);
    }

    cmd_lst->num = 0;

    return OK;
}










int exec_local_cmd_loop() {
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




int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    if (cmd_line == NULL || clist == NULL)
    {
        return ERR_MEMORY;
    }

    // Initialize command list
    clist->num = 0;

    // Make a copy of the command line for tokenizing
    char *cmd_copy = strdup(cmd_line);
    if (cmd_copy == NULL)
    {
        return ERR_MEMORY;
    }

    // Tokenize the command line by pipes
    char *cmd_token = strtok(cmd_copy, PIPE_STRING);
    while (cmd_token != NULL && clist->num < CMD_MAX)
    {
        // Remove leading and trailing whitespace
        while (*cmd_token == SPACE_CHAR)
        {
            cmd_token++;
        }

        int len = strlen(cmd_token);
        while (len > 0 && cmd_token[len - 1] == SPACE_CHAR)
        {
            cmd_token[--len] = '\0';
        }

        // Skip empty commands
        if (strlen(cmd_token) == 0)
        {
            cmd_token = strtok(NULL, PIPE_STRING);
            continue;
        }


        // Allocate and build command buffer
        if (alloc_cmd_buff(&clist->commands[clist->num]) != OK)
        {
            free(cmd_copy);
            for (int i = 0; i < clist->num; i++)
            {
                free_cmd_buff(&clist->commands[i]);
            }
            return ERR_MEMORY;
        }

        if (build_cmd_buff(cmd_token, &clist->commands[clist->num]) != OK)
        {
            free(cmd_copy);
            for (int i = 0; i <= clist->num; i++)
            {
                free_cmd_buff(&clist->commands[i]);
            }
            return ERR_MEMORY;
        }

        clist->num++;
        cmd_token = strtok(NULL, PIPE_STRING);
    }

    free(cmd_copy);

    // Check if we have too many commands
    if (cmd_token != NULL && clist->num >= CMD_MAX)
    {
        for (int i = 0; i < clist->num; i++)
        {
            free_cmd_buff(&clist->commands[i]);
        }
        return ERR_TOO_MANY_COMMANDS;
    }

    // Check if we have any commands
    if (clist->num == 0)
    {
        return WARN_NO_CMDS;
    }

    return OK;
}

/*
 * Match built-in commands
 */
Built_In_Cmds match_command(const char *input)
{
    if (input == NULL)
    {
        return BI_NOT_BI;
    }

    if (strcmp(input, EXIT_CMD) == 0)
    {
        return BI_CMD_EXIT;
    }
    else if (strcmp(input, "cd") == 0)
    {
        return BI_CMD_CD;
    }
    else if (strcmp(input, "dragon") == 0)
    {
        return BI_CMD_DRAGON;
    }

    return BI_NOT_BI;
}




int execute_pipeline(command_list_t *clist) {
    if (clist == NULL || clist->num <= 0) {
        return WARN_NO_CMDS;
    }

    int pipes[clist->num - 1][2];  // Array of pipes
    pid_t pids[clist->num];        // Array to store process IDs
    int pipeline_status = OK;      // Track overall pipeline status

    // Create all necessary pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    // Create processes for each command
    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");

            // Clean up pipes and kill any already forked children
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            for (int j = 0; j < i; j++) {
                kill(pids[j], SIGTERM);
                waitpid(pids[j], NULL, 0);
            }
            return ERR_EXEC_CMD;
        }

        if (pids[i] == 0) {  // Child process
            // Set up input pipe for all except first process
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2 (stdin)");
                    exit(EXIT_FAILURE);
                }
            }

            // Set up output pipe for all except last process
            if (i < clist->num - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2 (stdout)");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe ends in child
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            printf(CMD_ERR_EXECUTE, clist->commands[i].argv[0]);  // Print error message if execvp fails
            exit(EXIT_FAILURE);
        }
    }

    // Parent process: close all pipe ends
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children and check their status
    int status;
    for (int i = 0; i < clist->num; i++) {
        if (waitpid(pids[i], &status, 0) == -1) {
            perror("waitpid");
            pipeline_status = ERR_EXEC_CMD;
        } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            // Command failed or terminated abnormally
            pipeline_status = ERR_EXEC_CMD; // Mark the whole pipeline as failed
        }
    }


    return pipeline_status;
}