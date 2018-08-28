/* single.c: Single User HTTP Server */

#include "spidey.h"

#include <errno.h>
#include <string.h>

#include <unistd.h>

/**
 * Handle one HTTP request at a time.
 *
 * @param   sfd         Server socket file descriptor.
 * @return  Exit status of server (EXIT_SUCCESS).
 **/
int single_server(int sfd) {
    Request *r;
   // HTTPStatus result;
    /* Accept and handle HTTP request */
    debug("Starting single server");
    while (true) {
    	/* Accept request */
        debug("Accepting request");
        r = accept_request(sfd);
        if(!r){
            close(sfd);
            return EXIT_FAILURE;
        }
	/* Handle request */
        debug("Handling request");
        handle_request(r);
        
	/* Free request */
        debug("Freeing request");
        free_request(r);
    }

    /* Close server socket */
    close(sfd);
    debug("Finishing single server");

    return EXIT_SUCCESS;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
