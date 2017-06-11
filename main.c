
#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>
#include <libguacd/user.h>

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <winsock2.h>

#include "ball.h"
#include "socket-wsa.h"

int main(int argc, char** argv) {

    WSADATA wsa;
    SOCKET sock;

    printf("Initializing ball client...\n");
    guac_client* client = guac_client_alloc();
    guac_client_init(client);

    struct sockaddr_in addr;

    printf("Initializing network stack...\n");

    if (WSAStartup(MAKEWORD(2, 2), &wsa)) {
        printf("Failed to init network stack. Error: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Networking initialized\n");

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        printf("Could not create socket. Error: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Socket created\n");

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(4822);

    if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Could not bind socket. Error: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Socket bound\n");

    listen(sock, 3);

    printf("Waiting for connection...\n");

    SOCKET connection = accept(sock, NULL, NULL);
    if (connection == INVALID_SOCKET) {
        printf("Could not accept connection. Error: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Accepted connection. Waiting for select...\n");

    guac_socket* socket = guac_socket_open_wsa(connection);
    guac_parser* parser = guac_parser_alloc();

    /* Wait for select */
    if (guac_parser_expect(parser, socket, 15000000, "select")) {
        printf("\"select\" not read: %s %d\n", guac_error_message, WSAGetLastError());
        return 1;
    }

    /* Validate select length */
    if (parser->argc != 1) {
        printf("\"select\" had wrong number of arguments.\n");
        return 1;
    }

    /* Validate contents of "select */
    const char* identifier = parser->argv[0];
    if (strcmp(identifier, "ball") == 0)
        printf("\"ball\" selected!\n");
    else if (strcmp(identifier, client->connection_id) == 0)
        printf("Joining existing.\n");
    else {
        printf("Invalid protocol or connection ID.\n");
        return 1;
    }

    /* Init user */
    guac_user* user = guac_user_alloc();
    user->client = client;
    user->socket = socket;

    /* Start and wait for user */
    guacd_handle_user(user);

    /* User done */
    guac_user_free(user);
    guac_parser_free(parser);
    guac_socket_free(socket);

    /* Client done */
    guac_client_free(client);
    printf("Done!\n");

    /* Daemon done */
    closesocket(sock);
    WSACleanup();

    return 0;

}

