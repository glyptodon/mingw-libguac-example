
#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/parser.h>
#include <guacamole/socket.h>
#include <guacamole/socket-wsa.h>
#include <guacamole/user.h>

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <winsock2.h>

#include "ball.h"

int main() {

    struct sockaddr_in addr;

    WSADATA wsa;
    SOCKET sock;

    /*
     * In Guacamole's C API, the server-side structure that deals with
     * rendering and display state is guac_client. The "client" name is due to
     * the structure most frequently being used to represent the display state
     * of a server-side remote desktop client.
     *
     * In this example, which is based on the protocol tutorial in the
     * manual [1], the client is not a remote desktop client but rather a
     * simplified application which leverages the Guacamole protocol to render
     * a bouncing ball. It maintains display state, replicates that state to
     * joining users, and continuously updates that state via Guacamole
     * protocol instructions.
     *
     * Unlike protocol plugins written for guacd, the guac_client_init()
     * function here is not required. The guac_client object does need to be
     * initialized, but outside guacd there is no need for a specific function
     * for this, nor that is be named "guac_client_init". The
     * "guac_client_init" name is used here only for convenience and
     * consistency.
     *
     * [1] http://guacamole.incubator.apache.org/doc/gug/custom-protocols.html
     */

    /* Start internal handling of display state */
    printf("Initializing ball client...\n");
    guac_client* client = guac_client_alloc();
    guac_client_init(client);

    /*
     * The main differences here from standard use of Guacamole's C API
     * (libguac) center around Windows APIs like winsock. Part of the libguac
     * porting effort involved adding a winsock (WSA) guac_socket
     * implementation (guac_socket being the communication object used
     * throughout the C API).
     *
     * Unlike Linux/UNIX, the WSA apparently must be initialized prior to use
     * on a per-application basis, thus you won't find this in the mainline
     * tutorials.
     */

    /* Init Windows Socket API */
    printf("Initializing network stack...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa)) {
        printf("Failed to init network stack. Error: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Networking initialized\n");

    /*
     * The creation of the actual listening socket here parallels that of
     * mainline guacd. In many respects, this application is effectively
     * integrating its own mini-guacd.
     */

    /* Create TCP socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        printf("Could not create socket. Error: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Socket created\n");

    /*
     * For simplicity's sake, we'll be listening on the wildcard address. There
     * are security implications for this in production applications, but the
     * bouncing ball will be safe.
     *
     * The default port used by Guacamole for its own Guacamole protocol
     * communications is 4822.
     */

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(4822);

    /* Bind socket to 0.0.0.0:4822 */
    if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Could not bind socket. Error: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Socket bound\n");

    /*
     * Note here that we are accepting only one connection. In practice, this
     * would be done within a loop, allowing any number of users to join and
     * leave the connection. For the sake of simplicity, and for the sake of
     * demonstrating proper cleanup when the application is shutting down, only
     * one user is maintained here, and the entire application shuts down after
     * that user disconnects.
     */

    /* Begin listening for incoming connections */
    listen(sock, 3);

    printf("Waiting for connection...\n");

    /* Wait for and accept the next connection */
    SOCKET connection = accept(sock, NULL, NULL);
    if (connection == INVALID_SOCKET) {
        printf("Could not accept connection. Error: %d\n", WSAGetLastError());
        return 1;
    }

    /* Obtain a Windows-compatible guac_socket for the new connection */
    guac_socket* socket = guac_socket_open_wsa(connection);
    guac_parser* parser = guac_parser_alloc();

    /*
     * Once the connection has been accepted, the first step in the Guacamole
     * protocol handshake process [1] is to receive a "select" instruction [2].
     * This instruction dictates the underlying protocol of the connection, or
     * the connection being joined. In the case of an application, the value
     * expected can simply be a unique name for the application. Here, we use
     * "ball".
     *
     * [1] http://guacamole.incubator.apache.org/doc/gug/guacamole-protocol.html#guacamole-protocol-handshake
     * [2] http://guacamole.incubator.apache.org/doc/gug/protocol-reference.html#select-instruction
     */

    printf("Accepted connection. Waiting for select...\n");

    /* Wait for select */
    if (guac_parser_expect(parser, socket, 15000000, "select")) {
        printf("\"select\" not read: %s %d\n", guac_error_message,
                WSAGetLastError());
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

    /*
     * The parsing of "select" is the only low-level Guacamole protocol parsing
     * that needs to be done by an application leveraging the Guacamole
     * protocol. The rest is all handled by guac_user_handle_connection(),
     * assuming the guac_user and associated guac_client have been fully and
     * properly initialized.
     */

    /* Init user */
    guac_user* user = guac_user_alloc();
    user->client = client;
    user->socket = socket;

    /* Start and wait for user */
    guac_user_handle_connection(user, 15000000);

    /*
     * The guac_user_handle_connection() function blocks until the user
     * disconnects or stops responding. That 15000000 value is the number of
     * microseconds the wait for user messages before assuming the user has
     * left or become unresponsive (15 seconds). The majority of the Guacamole
     * stack uses 15-second timeouts.
     *
     * Since this example is only handling a single connection, we now clean
     * up all resources and shut down.
     */

    /* User done */
    printf("User has left connection.\n");
    guac_user_free(user);
    guac_parser_free(parser);
    guac_socket_free(socket);

    /* Signal client to shut down */
    guac_client_stop(client);

    /* Client done */
    guac_client_free(client);
    printf("Done!\n");

    /* Daemon done */
    closesocket(sock);
    WSACleanup();

    return 0;

}

