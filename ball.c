
#include <guacamole/client.h>
#include <guacamole/layer.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/timestamp.h>
#include <guacamole/user.h>

#include <stdlib.h>
#include <windows.h>

/**
 * The arguments accepted by the ball "client" during the Guacamole protocol
 * handshake. In this case, no arguments are accepted.
 */
const char* BALL_ARGS[] = { NULL };

/**
 * The internal state of the ball "client".
 */
typedef struct ball_client_data {

    /**
     * The Guacamole layer containing the bouncing ball.
     */
    guac_layer* ball;

    /**
     * The current X position of the upper-left corner of the bouncing ball, in
     * pixels.
     */
    int ball_x;

    /**
     * The current Y position of the upper-left corner of the bouncing ball, in
     * pixels.
     */
    int ball_y;

    /**
     * The current X velocity of the bouncing ball, in pixels per second.
     */
    int ball_velocity_x;

    /**
     * The current Y velocity of the bouncing ball, in pixels per second.
     */
    int ball_velocity_y;

    /**
     * A reference to the running thread which is periodically updating the
     * ball position, velocity, and sending the necessary Guacamole protocol
     * instructions to update the display of each connected user.
     */
    HANDLE render_thread;

} ball_client_data;

/**
 * The ball "client" rendering thread. This function runs in the background
 * once guac_client_init() has been invoked, periodically updating the location
 * of the ball, its velocity, etc., sending Guacamole protocol instructions to
 * any connected users to update their display state to match.
 *
 * @param arg
 *     A pointer to the guac_client instance associated with the running ball
 *     "client".
 *
 * @returns
 *     Always zero.
 */
DWORD WINAPI ball_render_thread(LPVOID arg) {

    /* Get data */
    guac_client* client = (guac_client*) arg;
    ball_client_data* data = (ball_client_data*) client->data;

    /* Init time of last frame to current time */
    guac_timestamp last_frame = guac_timestamp_current();

    /* Update ball position as long as client is running */
    while (client->state == GUAC_CLIENT_RUNNING) {

        /* Default to 30ms frames */
        int frame_duration = 30;

        /* Lengthen frame duration if client is lagging */
        int processing_lag = guac_client_get_processing_lag(client);
        if (processing_lag > frame_duration)
            frame_duration = processing_lag;

        /* Sleep for duration of frame, then get timestamp */
        usleep(frame_duration * 1000);
        guac_timestamp current = guac_timestamp_current();

        /* Calculate change in time */
        int delta_t = current - last_frame;

        /* Update position */
        data->ball_x += data->ball_velocity_x * delta_t / 1000;
        data->ball_y += data->ball_velocity_y * delta_t / 1000;

        /* Bounce if necessary */
        if (data->ball_x < 0) {
            data->ball_x = -data->ball_x;
            data->ball_velocity_x = -data->ball_velocity_x;
        }
        else if (data->ball_x >= 1024 - 128) {
            data->ball_x = (2 * (1024 - 128)) - data->ball_x;
            data->ball_velocity_x = -data->ball_velocity_x;
        }

        if (data->ball_y < 0) {
            data->ball_y = -data->ball_y;
            data->ball_velocity_y = -data->ball_velocity_y;
        }
        else if (data->ball_y >= 768 - 128) {
            data->ball_y = (2 * (768 - 128)) - data->ball_y;
            data->ball_velocity_y = -data->ball_velocity_y;
        }

        guac_protocol_send_move(client->socket, data->ball,
                GUAC_DEFAULT_LAYER, data->ball_x, data->ball_y, 0);

        /* End frame and flush socket */
        guac_client_end_frame(client);
        guac_socket_flush(client->socket);

        /* Update timestamp */
        last_frame = current;

    }

    return 0;

}

/**
 * Invoked when a new user joins the connection. In the case of this example,
 * which accepts only one user, this function will only ever be invoked once.
 * It is the responsibility of this function to synchronize the display state
 * of the new user (such that any incremental updates sent through the
 * guac_client will have the expected effect), and to assign any user-level
 * handlers for input events, etc.
 *
 * @param user
 *     The guac_user that has joined the connection.
 *
 * @param argc
 *     The number of arguments within the argv array, provided by the user as
 *     part of the Guacamole protocol handshake. As the ball "client" does not
 *     declare any arguments, this should always be zero.
 *
 * @param argv
 *     All arguments provided by the user as part of the Guacamole protocol
 *     handshake.
 *
 * @returns
 *     Zero if the user was successfully joined to the connection, non-zero
 *     otherwise.
 */
static int ball_join_handler(guac_user* user, int argc, char** argv) {

    /* Get client associated with user */
    guac_client* client = user->client;

    /* Get ball layer from client data */
    ball_client_data* data = (ball_client_data*) client->data;
    guac_layer* ball = data->ball;

    /* Get user-specific socket */
    guac_socket* socket = user->socket;

    /* Send the display size */
    guac_protocol_send_size(socket, GUAC_DEFAULT_LAYER, 1024, 768);

    /* Create background tile */
    guac_layer* texture = guac_client_alloc_buffer(client);

    guac_protocol_send_rect(socket, texture, 0, 0, 64, 64);
    guac_protocol_send_cfill(socket, GUAC_COMP_OVER, texture,
            0x88, 0x88, 0x88, 0xFF);

    guac_protocol_send_rect(socket, texture, 0, 0, 32, 32);
    guac_protocol_send_cfill(socket, GUAC_COMP_OVER, texture,
            0xDD, 0xDD, 0xDD, 0xFF);

    guac_protocol_send_rect(socket, texture, 32, 32, 32, 32);
    guac_protocol_send_cfill(socket, GUAC_COMP_OVER, texture,
            0xDD, 0xDD, 0xDD, 0xFF);

    /* Prepare a curve which covers the entire layer */
    guac_protocol_send_rect(socket, GUAC_DEFAULT_LAYER,
            0, 0, 1024, 768);

    /* Fill curve with texture */
    guac_protocol_send_lfill(socket,
            GUAC_COMP_OVER, GUAC_DEFAULT_LAYER,
            texture);

    /* Set up ball layer */
    guac_protocol_send_size(socket, ball, 128, 128);

    /* Prepare a circular curve */
    guac_protocol_send_arc(socket, data->ball,
            64, 64, 62, 0, 6.28, 0);

    guac_protocol_send_close(socket, data->ball);

    /* Draw a 4-pixel black border */
    guac_protocol_send_cstroke(socket,
            GUAC_COMP_OVER, data->ball,
            GUAC_LINE_CAP_ROUND, GUAC_LINE_JOIN_ROUND, 4,
            0x00, 0x00, 0x00, 0xFF);

    /* Fill the circle with color */
    guac_protocol_send_cfill(socket,
            GUAC_COMP_OVER, data->ball,
            0x00, 0x80, 0x80, 0x80);

    /* Free texture (no longer needed) */
    guac_client_free_buffer(client, texture);

    /* Mark end-of-frame */
    guac_protocol_send_sync(socket, client->last_sent_timestamp);

    /* Flush buffer */
    guac_socket_flush(socket);

    /* User successfully initialized */
    return 0;

}

/**
 * Invoked when the associated guac_client is being freed, and any underlying
 * resources must also be freed. This function will automatically be invoked
 * when guac_client_free() is called.
 *
 * @param client
 *     The guac_client being freed.
 *
 * @returns
 *     Zero if the resources underlying the given guac_client were successfully
 *     released, non-zero otherwise.
 */
static int ball_free_handler(guac_client* client) {

    ball_client_data* data = (ball_client_data*) client->data;

    /* Wait for render thread to terminate */
    WaitForSingleObject(data->render_thread, INFINITE);
    CloseHandle(data->render_thread);

    /* Free client-level ball layer */
    guac_client_free_layer(client, data->ball);

    /* Free client-specific data */
    free(data);

    /* Data successfully freed */
    return 0;

}

void guac_client_init(guac_client* client) {

    /* Allocate storage for client-specific data */
    ball_client_data* data = malloc(sizeof(ball_client_data));

    /* Set up client data and handlers */
    client->data = data;

    /* Allocate layer at the client level */
    data->ball = guac_client_alloc_layer(client);

    /* Start ball at upper left */
    data->ball_x = 0;
    data->ball_y = 0;

    /* Move at a reasonable pace to the lower right */
    data->ball_velocity_x = 200; /* pixels per second */
    data->ball_velocity_y = 200; /* pixels per second */

    /* Start render thread */
    data->render_thread = CreateThread(NULL, 0, ball_render_thread,
            client, 0, NULL);

    /* This example does not implement any arguments */
    client->args = BALL_ARGS;

    /*
     * Handler functions are used by Guacamole's C API for just about
     * everything. In this case, we are only handling when a user joins the
     * connection (in which case their display must be synchronized with the
     * current display state) and when the client is being shut down (in which
     * case resources must be freed).
     *
     * There are also user-level handler functions (on the guac_user object
     * received by the client->join_handler) which deal with keyboard and
     * mouse events, clipboard and file streams, changes in display size, etc.
     */
    client->join_handler = ball_join_handler;
    client->free_handler = ball_free_handler;

}

