#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dshlib.h"
#include "rshlib.h"

int start_server(char *ifaces, int port, int is_threaded) {
    int svr_socket;
    int rc;

    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        return svr_socket; // Error code from boot_server
    }

    rc = process_cli_requests(svr_socket);

    stop_server(svr_socket);
    return rc;
}

int stop_server(int svr_socket) {
    return close(svr_socket);
}

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

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ifaces);
    addr.sin_port = htons(port);

    if (bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    if (listen(svr_socket, 20) == -1) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    printf("Server started on %s:%d\n", ifaces, port);
    return svr_socket;
}

int process_cli_requests(int svr_socket) {
    int cli_socket;
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    while (1) {
        cli_socket = accept(svr_socket, (struct sockaddr *)&cli_addr, &cli_len);
        if (cli_socket == -1) {
            perror("accept");
            return ERR_RDSH_COMMUNICATION;
        }

        printf("Client connected\n");
        int rc = exec_client_requests(cli_socket);
        close(cli_socket);

        if (rc < 0) { // Negative return means stop server
            break;
        }
    }
    return OK;
}

int exec_client_requests(int cli_socket) {
    char *io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (!io_buff) {
        return ERR_RDSH_SERVER;
    }

    while (1) {
        int recv_len = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ - 1, 0);
        if (recv_len <= 0) { // Client disconnected or error
            if (recv_len == 0) {
                printf("Client disconnected\n");
            } else {
                perror("recv");
            }
            free(io_buff);
            return OK; // Exit to accept next client
        }

        io_buff[recv_len] = '\0'; // Null terminate for safety
        printf("Received: %s\n", io_buff);

        // Echo back the input
        int sent_len = send(cli_socket, io_buff, recv_len, 0);
        if (sent_len != recv_len) {
            perror("send");
            free(io_buff);
            return ERR_RDSH_COMMUNICATION;
        }

        // Send EOF character
        if (send_message_eof(cli_socket) != OK) {
            free(io_buff);
            return ERR_RDSH_COMMUNICATION;
        }
    }

    free(io_buff);
    return OK;
}

int send_message_eof(int cli_socket) {
    int sent_len = send(cli_socket, &RDSH_EOF_CHAR, sizeof(RDSH_EOF_CHAR), 0);
    if (sent_len != sizeof(RDSH_EOF_CHAR)) {
        perror("send EOF");
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

int send_message_string(int cli_socket, char *buff) {
    int len = strlen(buff);
    int sent_len = send(cli_socket, buff, len, 0);
    if (sent_len != len) {
        printf(CMD_ERR_RDSH_SEND, sent_len, len);
        return ERR_RDSH_COMMUNICATION;
    }
    return send_message_eof(cli_socket);
}