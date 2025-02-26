#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#include "dshlib.h"

/*
 * Allocate memory for a command buffer
 */
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff == NULL) {
        return ERR_MEMORY;
    }
    
    cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX * sizeof(char));
    if (cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }
    
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    return OK;
}

/*
 * Free memory allocated for a command buffer
 */
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff == NULL) {
        return ERR_MEMORY;
    }
    
    if (cmd_buff->_cmd_buffer != NULL) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    return OK;
}

/*
 * Clear a command buffer without deallocating memory
 */
int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff == NULL || cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }
    
    memset(cmd_buff->_cmd_buffer, 0, SH_CMD_MAX);
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    return OK;
}

/*
 * Build a command buffer from a command line string
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    if (cmd_line == NULL || cmd_buff == NULL) {
        return ERR_MEMORY;
    }
    
    // Clear the command buffer
    clear_cmd_buff(cmd_buff);
    
    // Copy the command line to the buffer
    strncpy(cmd_buff->_cmd_buffer, cmd_line, SH_CMD_MAX - 1);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';
    
    // Tokenize the command line by spaces
    char *token = strtok(cmd_buff->_cmd_buffer, " ");
    while (token != NULL && cmd_buff->argc < CMD_ARGV_MAX - 1) {
        cmd_buff->argv[cmd_buff->argc++] = token;
        token = strtok(NULL, " ");
    }
    
    // Null-terminate the argv array
    cmd_buff->argv[cmd_buff->argc] = NULL;
    
    return OK;
}

/*
 * Close a command buffer
 */
int close_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff == NULL) {
        return ERR_MEMORY;
    }
    
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    return OK;
}

/*
 * Build a command list from a command line string with pipes
 */
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    if (cmd_line == NULL || clist == NULL) {
        return ERR_MEMORY;
    }
    
    // Initialize command list
    clist->num = 0;
    
    // Make a copy of the command line for tokenizing
    char *cmd_copy = strdup(cmd_line);
    if (cmd_copy == NULL) {
        return ERR_MEMORY;
    }
    
    // Tokenize the command line by pipes
    char *cmd_token = strtok(cmd_copy, PIPE_STRING);
    while (cmd_token != NULL && clist->num < CMD_MAX) {
        // Remove leading and trailing whitespace
        while (*cmd_token == SPACE_CHAR) {
            cmd_token++;
        }
        
        int len = strlen(cmd_token);
        while (len > 0 && cmd_token[len - 1] == SPACE_CHAR) {
            cmd_token[--len] = '\0';
        }
        
        // Skip empty commands
        if (strlen(cmd_token) == 0) {
            cmd_token = strtok(NULL, PIPE_STRING);
            continue;
        }
        
        // Allocate and build command buffer
        if (alloc_cmd_buff(&clist->commands[clist->num]) != OK) {
            free(cmd_copy);
            for (int i = 0; i < clist->num; i++) {
                free_cmd_buff(&clist->commands[i]);
            }
            return ERR_MEMORY;
        }
        
        if (build_cmd_buff(cmd_token, &clist->commands[clist->num]) != OK) {
            free(cmd_copy);
            for (int i = 0; i <= clist->num; i++) {
                free_cmd_buff(&clist->commands[i]);
            }
            return ERR_MEMORY;
        }
        
        clist->num++;
        cmd_token = strtok(NULL, PIPE_STRING);
    }
    
    free(cmd_copy);
    
    // Check if we have too many commands
    if (cmd_token != NULL && clist->num >= CMD_MAX) {
        for (int i = 0; i < clist->num; i++) {
            free_cmd_buff(&clist->commands[i]);
        }
        return ERR_TOO_MANY_COMMANDS;
    }
    
    // Check if we have any commands
    if (clist->num == 0) {
        return WARN_NO_CMDS;
    }
    
    return OK;
}

/*
 * Free memory allocated for a command list
 */
int free_cmd_list(command_list_t *cmd_lst) {
    if (cmd_lst == NULL) {
        return ERR_MEMORY;
    }
    
    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&cmd_lst->commands[i]);
    }
    
    cmd_lst->num = 0;
    
    return OK;
}

/*
 * Match built-in commands
 */
Built_In_Cmds match_command(const char *input) {
    if (input == NULL) {
        return BI_NOT_BI;
    }
    
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    } else if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    } else if (strcmp(input, "dragon") == 0) {
        return BI_CMD_DRAGON;
    }
    
    return BI_NOT_BI;
}

/*
 * Execute built-in commands
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (cmd == NULL || cmd->argc == 0 || cmd->argv[0] == NULL) {
        return BI_NOT_BI;
    }
    
    Built_In_Cmds cmd_type = match_command(cmd->argv[0]);
    
    switch (cmd_type) {
        case BI_CMD_EXIT:
            return BI_CMD_EXIT;
            
        case BI_CMD_CD:
            if (cmd->argc > 1) {
                if (chdir(cmd->argv[1]) != 0) {
                    perror("cd");
                }
            } else {
                // Change to home directory if no argument
                char *home = getenv("HOME");
                if (home != NULL && chdir(home) != 0) {
                    perror("cd");
                }
            }
            return BI_EXECUTED;
            
        case BI_CMD_DRAGON:
            printf("                 _,_\n");
            printf("              ,-'    `-._\n");
            printf("             /'    -'     `.\n");
            printf("            /        .     \\\n");
            printf("           /         :      \\\n");
            printf("          /          )-.__.' \\\n");
            printf("         /          / `.      \\\n");
            printf("        /          /   |       \\\n");
            printf("       /         /    /        \\\n");
            printf("      /         /    /         /\n");
            printf("     /        .'   .'         /\n");
            printf("    /       .' | .'          /\n");
            printf("   /       /| | /|          /\n");
            printf("  (       ( \\/ )/)         (\n");
            printf("   \\       \\`-'//)          \\\n");
            printf("    \\       )-'//            \\\n");
            printf("     `.     |_.'              `.\n");
            printf("       `-._                 _.-'\n");
            printf("           `--.___/\\.___,--'\n");
            return BI_EXECUTED;
            
        default:
            return BI_NOT_BI;
    }
}

/*
 * Execute a single command
 */
int exec_cmd(cmd_buff_t *cmd) {
    if (cmd == NULL || cmd->argc == 0 || cmd->argv[0] == NULL) {
        return WARN_NO_CMDS;
    }
    
    // Check for built-in commands first
    Built_In_Cmds result = exec_built_in_cmd(cmd);
    if (result == BI_CMD_EXIT) {
        return OK_EXIT;
    } else if (result == BI_EXECUTED) {
        return OK;
    }
    
    // Fork a child process to execute the command
    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        perror("fork");
        return ERR_EXEC_CMD;
    } else if (pid == 0) {
        // Child process
        if (execvp(cmd->argv[0], cmd->argv) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        
        // This should never be reached
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return ERR_EXEC_CMD;
        }
        
        // Check if the command executed successfully
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            return ERR_EXEC_CMD;
        }
    }
    
    return OK;
}


/*
 * Main command loop for the shell
 */
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
        
        // Handle execution based on number of commands
        if (cmd_list.num > 1) {
            // Execute a pipeline for multiple commands
            result = execute_pipeline(&cmd_list);
            if (result == OK_EXIT) {
                free_cmd_list(&cmd_list);
                return OK_EXIT;
            } else if (result != OK) {
                printf("Error executing piped commands\n");
            }
        } else {
            // Execute a single command
            result = exec_cmd(&cmd_list.commands[0]);
            if (result == OK_EXIT) {
                free_cmd_list(&cmd_list);
                return OK_EXIT;
            } else if (result != OK) {
                printf("Error executing command\n");
            }
        }
        
        // Free the command list
        free_cmd_list(&cmd_list);
    }
    
    return OK;
}



int execute_pipeline(command_list_t *clist) {
    if (clist == NULL || clist->num <= 0) {
        return WARN_NO_CMDS;
    }
    
    // Check for built-in commands in the first command
    Built_In_Cmds result = exec_built_in_cmd(&clist->commands[0]);
    if (result == BI_CMD_EXIT) {
        return OK_EXIT;
    } else if (result == BI_EXECUTED && clist->num == 1) {
        return OK;
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
            
            // Set up the pipe redirections
            if (i > 0) {
                // Not the first command, so read from the previous pipe
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            if (i < clist->num - 1) {
                // Not the last command, so write to the next pipe
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close ALL pipe file descriptors in the child
            // This is critical - the child should only have the redirected stdin/stdout
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            
            // If execvp returns, it failed
            perror(clist->commands[i].argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    
    // Parent process - close ALL pipe file descriptors
    // This is crucial for the pipeline to work properly
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all children to finish
    for (int i = 0; i < clist->num; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
    
    return OK;
}