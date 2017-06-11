
#include <guacamole/client.h>
#include <guacamole/socket.h>

#include <stdio.h>
#include <winsock2.h>

int main(int argc, char** argv) {

    WSADATA wsa;
    SOCKET sock;

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

    guac_socket* socket = guac_socket_open(connection);

    guac_client* client = guac_client_alloc();
    guac_client_free(client);

    guac_socket_free(socket);

    printf("Done!\n");

    closesocket(sock);
    WSACleanup();

    return 0;

}

