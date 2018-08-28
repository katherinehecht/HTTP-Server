/* forking.c: Forking HTTP Server */

#include "spidey.h"

#include <errno.h>
#include <signal.h>
#include <string.h>

#include <unistd.h>

/**
 * Fork incoming HTTP requests to handle the concurrently.
 *
 * @param   sfd         Server socket file descriptor.
 * @return  Exit status of server (EXIT_SUCCESS).
 *
 * The parent should accept a request and then fork off and let the child
 * handle the request.
 **/
int forking_server(int sfd) {
    Request* r;
  //  HTTPStatus result;
    /* Accept and handle HTTP request */
    signal(SIGCHLD, SIG_IGN);

    while (true) {
    	/* Accept request */
        r = accept_request(sfd);
        if(!r){
            /*close(sfd);
            return EXIT_FAILURE;*/
            continue;
        }

        pid_t pid = fork();

	/* Ignore children */
        if(pid < 0 ) { // ERROR
            fprintf(stderr, "Unable to fork: %s\n", strerror(errno));
            free_request(r);
<<<<<<< HEAD
        } else if (pid == 0){ // CHILD
=======
        } 
        else if (pid == 0){ //CHILD
>>>>>>> d90698cc1db12c6b2de5480a28c40b219e453fb2
        /* Fork off child process to handle request */
            close(sfd);
            handle_request(r);
            free_request(r);
            return EXIT_SUCCESS;
<<<<<<< HEAD
        } else { // PARENT
=======
        } 
        else { //PARENT
>>>>>>> d90698cc1db12c6b2de5480a28c40b219e453fb2
            free_request(r);
        }

    }

    /* Close server socket */
    close(sfd);
    return EXIT_SUCCESS;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
