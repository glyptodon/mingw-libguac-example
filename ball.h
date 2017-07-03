#ifndef BALL_H
#define BALL_H

#include <guacamole/client.h>

/**
 * Initializes the given guac_client and begins background maintenance of the
 * bouncing ball's display state. Once this function returns, the given
 * guac_client is used to represent the display state of the application for
 * the entire life of the application, just as it would represent the display
 * state of the remote desktop connection in the case of normal use via guacd.
 *
 * @param client
 *     The guac_client structure to initialize.
 */
void guac_client_init(guac_client* client);

#endif
