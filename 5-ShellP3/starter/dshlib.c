#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"


int exec_local_cmd_loop()
{
    char cmd_line[SH_CMD_MAX];
    cmd_buff_t cmd_buff;
    int status;

    while (1)
    {
        printf("%s", SH_PROMPT);
        
        if (fgets(cmd_line, sizeof(cmd_line), stdin) == NULL)
        {
            printf("\n");
            break;
        }

        // Remove trailing newline
        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        // Check if input is empty
        if (strlen(cmd_line) == 0)
        {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }

        // Exit condition
        if (strcmp(cmd_line, EXIT_CMD) == 0)
        {
            break;
        }

        // Allocate command buffer
        if ((status = alloc_cmd_buff(&cmd_buff)) != OK)
        {
            printf("Memory allocation error\n");
            continue;
        }

        // Build command buffer
        if ((status = build_cmd_buff(cmd_line, &cmd_buff)) != OK)
        {
            if (status == ERR_TOO_MANY_COMMANDS)
            {
                printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            }
            free_cmd_buff(&cmd_buff);
            continue;
        }

        // Execute built-in commands
        Built_In_Cmds bi_status = exec_built_in_cmd(&cmd_buff);
        if (bi_status == BI_EXECUTED)
        {
            free_cmd_buff(&cmd_buff);
            continue;
        }

        // Execute external command
        if ((status = exec_cmd(&cmd_buff)) != OK)
        {
            printf(CMD_ERR_EXECUTE, cmd_line);
        }

        // Free allocated command buffer
        free_cmd_buff(&cmd_buff);
    }

    return OK;
}


int execute_pipeline(command_list_t *clist) {
    if (clist->num == 0) {
        fprintf(stderr, CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }
    
    int pipes[CMD_MAX - 1][2]; // Pipes between commands
    pid_t pids[CMD_MAX];       // Store child PIDs

    // Create pipes
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return ERR_MEMORY;
        }
    }

    // Fork and execute each command
    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork");
            return ERR_MEMORY;
        }

        if (pids[i] == 0) {  // Child process
            // Redirect input if not the first command
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            // Redirect output if not the last command
            if (i < clist->num - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Close all pipe FDs (only need stdin/stdout now)
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            fprintf(stderr, CMD_ERR_EXECUTE, clist->commands[i].argv[0]);
            exit(ERR_EXEC_CMD);
        }
    }

    // Close all pipes in parent
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children
    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], NULL, 0);
    }

    return OK;
}
