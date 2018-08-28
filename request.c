/* request.c: HTTP Request Functions */

#include "spidey.h"

#include <errno.h>
#include <string.h>

#include <unistd.h>

int parse_request_method(Request *r);
int parse_request_headers(Request *r);

/**
 * Accept request from server socket.
 *
 * @param   sfd         Server socket file descriptor.
 * @return  Newly allocated Request structure.
 *
 * This function does the following:
 *
 *  1. Allocates a request struct initialized to 0.
 *  2. Initializes the headers list in the request struct.
 *  3. Accepts a client connection from the server socket.
 *  4. Looks up the client information and stores it in the request struct.
 *  5. Opens the client socket stream for the request struct.
 *  6. Returns the request struct.
 *
 * The returned request struct must be deallocated using free_request.
 **/
Request * accept_request(int sfd) {
    Request *r;
    struct sockaddr raddr;
    socklen_t rlen = sizeof(struct sockaddr);
    int status;


    /* Allocate request struct (zeroed) */
    r = calloc(1, sizeof(Request));
    if(r == NULL){
        goto fail;
    }

    /* Accept a client */
    debug("Accepting client");
    r->fd = accept(sfd, &raddr, &rlen);
    if (r->fd < 0) {
        fprintf(stderr, "Unable to accept: %s\n", strerror(errno));
        goto fail;
    }
    /* Lookup client information */
    debug("Looking up client info");
    if((status = getnameinfo(&raddr, rlen, r->host, sizeof(r->host), r->port, sizeof(r->port), NI_NUMERICHOST | NI_NUMERICSERV)) != 0){
        fprintf(stderr, "Unable to getnameinfo: %s\n", gai_strerror(status));
        goto fail;
    }
    /* Open socket stream */
    debug("Opening socket stream");
    r->file = fdopen(r->fd, "w+");
    if (r->file == NULL) {
        fprintf(stderr, "Unable to fdopen: %s\n", strerror(errno));
        goto fail;
    }
    log("Accepted request from %s:%s", r->host, r->port);
    return r;

fail:
    /* Deallocate request struct */
    free_request(r);
    return NULL;
}

/**
 * Deallocate request struct.
 *
 * @param   r           Request structure.
 *
 * This function does the following:
 *
 *  1. Closes the request socket stream or file descriptor.
 *  2. Frees all allocated strings in request struct.
 *  3. Frees all of the headers (including any allocated fields).
 *  4. Frees request struct.
 **/
void free_request(Request *r) {
    if (!r) {
    	return;
    }

    /* Close socket or fd */
    if(r->file)
        fclose(r->file);
    else if(r->fd >= 0)
        close(r->fd);

    /* Free allocated strings */
    if (r->method) {
        free(r->method);
    }
    if (r->uri) {
        free(r->uri);
    }
    if (r->path) {
        free(r->path);
    }
    if (r->query) {
        free(r->query);
    }

    /* Free headers */
    Header * next;
    for(Header * curr = r->headers; curr; curr = next){
        next = curr->next;
        if (curr->name) {
            free(curr->name);
        }
        if (curr->value) {
            free(curr->value);
        }
        free(curr);
    }

    /* Free request */
    free(r);
}

/**
 * Parse HTTP Request.
 *
 * @param   r           Request structure.
 * @return  -1 on error and 0 on success.
 *
 * This function first parses the request method, any query, and then the
 * headers, returning 0 on success, and -1 on error.
 **/
int parse_request(Request *r) {
    /* Parse HTTP Request Method */
    if(parse_request_method(r) < 0){
        return -1;
    }

    /* Parse HTTP Requet Headers*/
    return parse_request_headers(r);

}
/**
 * Parse HTTP Request Method and URI.
 *
 * @param   r           Request structure.
 * @return  -1 on error and 0 on success.
 *
 * HTTP Requests come in the form
 *
 *  <METHOD> <URI>[QUERY] HTTP/<VERSION>
 *
 * Examples:
 *
 *  GET / HTTP/1.1
 *  GET /cgi.script?q=foo HTTP/1.0
 *
 * This function extracts the method, uri, and query (if it exists).
 **/
int parse_request_method(Request *r) {
    char buffer[BUFSIZ];
    char *method;
    char *uri;
    char *query;

    /* Read line from socket */
    if(fgets(buffer, BUFSIZ, r->file) == NULL) {
        goto fail;
    }

    method = (strtok(skip_whitespace(buffer), WHITESPACE));
    uri = strtok(NULL, WHITESPACE);
    if (!method || !uri) {
        goto fail;
    }

    if ((query = strchr(uri, '?')) != NULL) {
        *query++ = '\0';
    }
    else {
        query = "";
    }

    r->method = strdup(method);
    r->uri = strdup(uri);
    r->query = strdup(query);

    debug("HTTP METHOD: %s", r->method);
    debug("HTTP URI:    %s", r->uri);
    debug("HTTP QUERY:  %s", r->query);


    return 0;

fail:
    return -1;
}

/**
 * Parse HTTP Request Headers.
 *
 * @param   r           Request structure.
 * @return  -1 on error and 0 on success.
 *
 * HTTP Headers come in the form:
 *
 *  <NAME>: <VALUE>
 *
 * Example:
 *
 *  Host: localhost:8888
 *  User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:29.0) Gecko/20100101 Firefox/29.0
 *  Accept: text/html,application/xhtml+xml
 *  Accept-Language: en-US,en;q=0.5
 *  Accept-Encoding: gzip, deflate
 *  Connection: keep-alive
 *
 * This function parses the stream from the request socket using the following
 * pseudo-code:
 *
 *  while (buffer = read_from_socket() and buffer is not empty):
 *      name, value = buffer.split(':')
 *      header      = new Header(name, value)
 *      headers.append(header)
 **/
int parse_request_headers(Request *r) {
    struct header *curr = NULL;
    char buffer[BUFSIZ];
    char *name;
    char *value;

    /* Parse headers from socket */
    if(r->file == NULL){
        goto fail;
    }

    while(fgets(buffer, BUFSIZ, r->file)){
        chomp(buffer);
        if (!strchr(buffer, ':')) break;
        char *skip = strchr(buffer, ':');
        if (skip == NULL){
            fprintf(stderr, "Unable to parse: %s\n", strerror(errno));
            goto fail;
        }
        *skip = '\0';
        name = buffer;
        skip++;
        skip = skip_whitespace(skip);
        value = skip;
        curr = calloc(1, sizeof(struct header));
        if(!curr){
            fprintf(stderr, "Unable to calloc: %s\n", strerror(errno));
            goto fail;
        }
        curr->name = strdup(name);
        curr->value = strdup(value);
        curr->next = r->headers;
        r->headers = curr;
    }
#ifndef NDEBUG
    for (struct header *header = r->headers; header != NULL; header = header->next) {
    	debug("HTTP HEADER %s = %s", header->name, header->value);
    }
#endif
    return 0;

fail:
    return -1;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
