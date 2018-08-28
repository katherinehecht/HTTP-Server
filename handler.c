/* return handler.c: HTTP Request Handlers */

#include "spidey.h"

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/* Internal Declarations */
HTTPStatus handle_browse_request(Request *request);
HTTPStatus handle_file_request(Request *request);
HTTPStatus handle_cgi_request(Request *request);
HTTPStatus handle_error(Request *request, HTTPStatus status);

/**
 * Handle HTTP Request.
 *
 * @param   r           HTTP Request structure
 * @return  Status of the HTTP request.
 *
 * This parses a request, determines the request path, determines the request
 * type, and then dispatches to the appropriate return handler type.
 *
 * On error, handle_error should be used with an appropriate HTTP status code.
 **/
HTTPStatus  handle_request(Request *r) {
    HTTPStatus result;
    /* Parse request */
    debug("Parsing request");
    if(parse_request(r) < 0){
        return  handle_error(r, HTTP_STATUS_BAD_REQUEST);
    }

    /* Determine request path */
    debug("Determining request path");

    r->path = determine_request_path(r->uri);
    debug("R->path: %s", r->path);
    if(r->path == NULL){
        return  handle_error(r, HTTP_STATUS_NOT_FOUND);
    }
    debug("HTTP REQUEST PATH: %s", r->path);

    /* Dispatch to appropriate request return handler type based on file type */
    struct stat s;
    if(stat(r->path, &s) == -1){
        return handle_error(r, HTTP_STATUS_NOT_FOUND);
    }

    // call stat
    if(S_ISDIR(s.st_mode)){
        result =  handle_browse_request(r);
    }else if (S_ISREG(s.st_mode)){
        if(access(r->path, X_OK) == 0){
            result = handle_cgi_request(r);
        }else {
            result = handle_file_request(r);
        }
    }
    log("HTTP REQUEST STATUS: %s", http_status_string(result));
    return result;
}

/**
 * Handle browse request.
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP browse request.
 *
 * This lists the contents of a directory in HTML.
 *
 * If the path cannot be opened or scanned as a directory, then handle error
 * with HTTP_STATUS_NOT_FOUND.
 **/
HTTPStatus  handle_browse_request(Request *r) {
    struct dirent **entries;
    int n;

    /* Open a directory for reading or scanning */
    n = scandir(r->path, &entries, NULL, alphasort);
    if(n < 0){
        handle_error(r, HTTP_STATUS_NOT_FOUND);
    }

    /* Write HTTP Header with OK Status and text/html Content-Type */
    fprintf(r->file, "HTTP/1.0 200 OK\r\nContent.Type: text/html\r\n\r\n");

    /* For each entry in directory, emit HTML list item */
    fprintf(r->file, "<html>\n");
    fprintf(r->file, "<ul>\n");
    for (int i = 0; i < n; i++) {
        if (streq(entries[i]->d_name, ".")) {
            free(entries[i]);
            continue;
        }
        if (streq(r->uri, "/")) {
            fprintf(r->file, "<li><a href=\"%s%s\">%s</a></li>\n", r->uri, entries[i]->d_name, entries[i]->d_name);
        }
        else {
            fprintf(r->file, "<li><a href=\"%s/%s\">%s</a></li>\n", r->uri, entries[i]->d_name, entries[i]->d_name);
        }
        free(entries[i]);
    }
    fprintf(r->file, "</ul>\n");
    fprintf(r->file, "</html>\n");
    free(entries);

    /* Flush socket, return OK */
    fflush(r->file);
    return HTTP_STATUS_OK;
}

/**
 * Handle file request.
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP file request.
 *
 * This opens and streams the contents of the specified file to the socket.
 *
 * If the path cannot be opened for reading, then handle error with
 * HTTP_STATUS_NOT_FOUND.
 **/
HTTPStatus  handle_file_request(Request *r) {
    FILE *fs;
    char buffer[BUFSIZ];
    char *mimetype = NULL;
    size_t nread;

    /* Open file for reading */
    fs = fopen(r->path, "r");
    if(!fs) {
        fprintf(stderr, "Unable to fdopen: %s", r->path);
        goto fail;
    }

    /* Determine mimetype */
    mimetype = determine_mimetype(r->path);

    /* Write HTTP Headers with OK status and determined Content-Type */
     fprintf(r->file, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", mimetype);


    /* Read from file and write to socket in chunks */
    while((nread = fread(buffer, 1, BUFSIZ, fs)) > 0){
        size_t nwrit = fwrite(buffer, 1, nread, r->file);
        if(nwrit != nread){
            goto fail;
        }
    }

    /* Close file, flush socket, deallocate mimetype, return OK */
    fclose(fs);
    fflush(r->file);
    free(mimetype);
    return HTTP_STATUS_OK;

fail:
    /* Close file, free mimetype, return INTERNAL_SERVER_ERROR */
    fclose(fs);
    free(mimetype);
    return HTTP_STATUS_INTERNAL_SERVER_ERROR;
}

/**
 * Handle CGI request
 *
 * @param   r           HTTP Request structure.
 * @return  Status of the HTTP file request.
 *
 * This popens and streams the results of the specified executables to the
 * socket.
 *
 * If the path cannot be popened, then handle error with
 * HTTP_STATUS_INTERNAL_SERVER_ERROR.
 **/
HTTPStatus handle_cgi_request(Request *r) {
    FILE *pfs;
    char buffer[BUFSIZ];

    /* Export CGI environment variables from request structure:
     * http://en.wikipedia.org/wiki/Common_Gateway_Interface */
    if(setenv("DOCUMENT_ROOT", RootPath, 1) < 0 ){
        return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }
    if(setenv("QUERY_STRING", r->query, 1) < 0){
        return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }
    if(setenv("REQUEST_METHOD", r->method, 1) < 0){
        return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }
    if(setenv("REQUEST_URI", r->uri, 1) < 0){
        return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }
    if(setenv("SCRIPT_FILENAME", r->path, 1) < 0){
        return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }
    if(setenv("REMOTE_ADDR", r->host, 1) < 0){
        return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }
    if(setenv("REMOTE_PORT", r->port, 1) < 0){
        return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }
    if(setenv("SERVER_PORT", Port , 1) < 0){
      return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }

    /* Export CGI environment variables from request headers */
    for(Header* it = r->headers; it; it = it->next) {
        if(streq(it->name, "Host")){
            if(setenv("HTTP_HOST", it->value, 1) < 0){
                return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
            }
        }
        if(streq(it->name, "User-Agent")){
            if(setenv("HTTP_USER_AGENT", it->value, 1) < 0){
                return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
            }
        }
        if(streq(it->name, "Accept")){
            if(setenv("HTTP_ACCEPT", it->value, 1) < 0){
                return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
            }
        }
        if(streq(it->name, "Accept-Language")){
            if(setenv("HTTP_ACCEPT_LANGUAGE", it->value, 1) < 0){
                return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
            }
        }
        if(streq(it->name, "Accept-Encoding")){
            if(setenv("HTTP_ACCEPT_ENCODING", it->value, 1) < 0){
                return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
            }
        }
        if(streq(it->name, "Connection")){
            if(setenv("HTTP_CONNECTION", it->value, 1) < 0){
                return handle_error(r, HTTP_STATUS_INTERNAL_SERVER_ERROR);
            }
        }
    }

    /* POpen CGI Script */
    pfs = popen(r->path, "r");

    /* Copy data from popen to socket */
    while(fgets(buffer, BUFSIZ, pfs)){
        fputs(buffer, r->file);
    }

    /* Close popen, flush socket, return OK */
    fclose(pfs);
    fflush(r->file);
    return HTTP_STATUS_OK;
}

/**
 * Handle displaying error page
 *
 * @param   r           HTTP Request structure.
 * @return  Sta:tus of the HTTP error request.
 *
 * This writes an HTTP status error code and then generates an HTML message to
 * notify the user of the error.
 **/
HTTPStatus  handle_error(Request *r, HTTPStatus status) {
    const char *status_string = http_status_string(status);
    /* Write HTTP Header */
    fprintf(r->file, "HTTP/1.0 %s\r\nContent.Type: text/html\r\n\r\n", status_string);

    /* Write HTML Description of Error*/
    fprintf(r->file, "<html>\r\n");
    fprintf(r->file, "<h1>%s</h1>\r\n", status_string);
    fprintf(r->file, "</html>\r\n");

    /* Return specified status */
    return status;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
