#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dshlib.h"
#include "rshlib.h"

int boot_server(char *ifaces, int port) {
    int svr_socket;
    struct sockaddr_in addr;

    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket == -1) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }

    int enable = 1;
    setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    if (bind(svr_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    if (listen(svr_socket, 20) == -1) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    return svr_socket;
}

int stop_server(int svr_socket) {
    return close(svr_socket);
}

int send_message_eof(int cli_socket) {
    int sent_len = send(cli_socket, &RDSH_EOF_CHAR, sizeof(RDSH_EOF_CHAR), 0);
    if (sent_len != sizeof(RDSH_EOF_CHAR)) {
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

int send_message_string(int cli_socket, char *buff) {
    if (buff == NULL) {
        return ERR_RDSH_COMMUNICATION;
    }

    // Append a newline character to the response
    char response[RDSH_COMM_BUFF_SZ];
    snprintf(response, RDSH_COMM_BUFF_SZ, "%s\n", buff);

    // Send the response to the client
    int send_len = strlen(response);
    int sent_len = send(cli_socket, response, send_len, 0);
    if (sent_len != send_len) {
        perror("send");
        return ERR_RDSH_COMMUNICATION;
    }

    // Send EOF character to indicate end of message
    return send_message_eof(cli_socket);
}

int exec_client_requests(int cli_socket) {
    char *io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (io_buff == NULL) {
        return ERR_RDSH_SERVER;
    }

    int rc = OK;
    while (1) {
        memset(io_buff, 0, RDSH_COMM_BUFF_SZ);
        int io_size = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ - 1, 0);
        
        if (io_size <= 0) {
            if (io_size == 0) {
                rc = OK;  // Normal client disconnect
            } else {
                perror("recv");
                rc = ERR_RDSH_COMMUNICATION;
            }
            break;
        }

        io_buff[io_size] = '\0';
        printf("Received: %s\n", io_buff);

        cmd_buff_t cmd_buff;
        if (alloc_cmd_buff(&cmd_buff) != OK) {
            send_message_string(cli_socket, "Memory allocation error");
            continue;
        }
        if (build_cmd_buff(io_buff, &cmd_buff) != OK) {
            send_message_string(cli_socket, "Invalid command");
            free_cmd_buff(&cmd_buff);
            continue;
        }

        if (strcmp(io_buff, "stop-server") == 0) {
            send_message_string(cli_socket, "Shutting down server. Goodbye!");
            free_cmd_buff(&cmd_buff);
            rc = OK_EXIT;
            break;
        }

        Built_In_Cmds bi_status = exec_built_in_cmd(&cmd_buff);
        if (bi_status == BI_EXECUTED) {
            send_message_string(cli_socket, "Command executed successfully");
            free_cmd_buff(&cmd_buff);
            continue;
        } else if (bi_status == BI_CMD_EXIT) {
            send_message_string(cli_socket, "Goodbye!");
            free_cmd_buff(&cmd_buff);
            rc = OK;
            break;
        }

        if (bi_status == BI_NOT_BI) {
            rc = exec_cmd(&cmd_buff);
            char response[RDSH_COMM_BUFF_SZ];
            snprintf(response, RDSH_COMM_BUFF_SZ, "Command returned: %d", rc);
            send_message_string(cli_socket, response);
        } else {
            send_message_string(cli_socket, "Command not supported");
        }

        free_cmd_buff(&cmd_buff);
    }

    free(io_buff);
    close(cli_socket);  // Close the client socket
    return rc;
}



int process_cli_requests(int svr_socket) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        int cli_socket = accept(svr_socket, (struct sockaddr*)&client_addr, &client_len);
        if (cli_socket < 0) {
            perror("accept");
            return ERR_RDSH_COMMUNICATION;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        int rc = exec_client_requests(cli_socket);
        printf(RCMD_MSG_CLIENT_EXITED);

        if (rc == OK_EXIT) {
            printf(RCMD_MSG_SVR_STOP_REQ);
            close(cli_socket);  // Close the client socket
            return rc;  // Return to shut down the server
        }
    }
    return OK;
}
int start_server(char *ifaces, int port, int is_threaded){
    int svr_socket;
    int rc;

    //
    //TODO:  If you are implementing the extra credit, please add logic
    //       to keep track of is_threaded to handle this feature
    //

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0){
        int err_code = svr_socket;  //server socket will carry error code
        return err_code;
    }

    rc = process_cli_requests(svr_socket);

    stop_server(svr_socket);


    return rc;
}
