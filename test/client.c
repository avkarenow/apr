/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include <stdlib.h>
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include <errno.h>

#define STRLEN 15

void closeapr(void)
{
    apr_terminate();
}

int main(int argc, char *argv[])
{
    apr_pool_t *context;
    apr_socket_t *sock;
    apr_size_t length;
    apr_status_t stat;
    char datasend[STRLEN] = "Send data test";
    char datarecv[STRLEN];
    char msgbuf[80];
    char *local_ipaddr, *remote_ipaddr;
    char *dest = "127.0.0.1";
    apr_port_t local_port, remote_port;
    apr_interval_time_t read_timeout = -1;
    apr_sockaddr_t *local_sa, *remote_sa;

    setbuf(stdout, NULL);
    if (argc > 1) {
        dest = argv[1];
    }

    if (argc > 2) {
        read_timeout = APR_USEC_PER_SEC * atoi(argv[2]);
    }

    fprintf(stdout, "Initializing.........");
    if (apr_initialize() != APR_SUCCESS) {
        fprintf(stderr, "Something went wrong\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    atexit(closeapr);

    fprintf(stdout, "Creating context.......");
    if (apr_create_pool(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Something went wrong\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout,"\tClient:  Making socket address...............");
    if ((stat = apr_getaddrinfo(&remote_sa, dest, APR_UNSPEC, 8021, 0, context)) 
        != APR_SUCCESS) {
        fprintf(stdout, "Failed!\n");
        fprintf(stdout, "Address resolution failed for %s: %s\n", 
                dest, apr_strerror(stat, msgbuf, sizeof(msgbuf)));
        exit(-1);
    }
    fprintf(stdout,"OK\n");

    fprintf(stdout, "\tClient:  Creating new socket.......");
    if (apr_create_socket(&sock, remote_sa->sa.sin.sin_family, SOCK_STREAM,
                          context) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't create socket\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Connecting to socket.......");

    stat = apr_connect(sock, remote_sa);

    if (stat != APR_SUCCESS) {
        apr_close_socket(sock);
        fprintf(stderr, "Could not connect: %s (%d)\n", 
		apr_strerror(stat, msgbuf, sizeof(msgbuf)), stat);
        fflush(stderr);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    if (read_timeout == -1) {
        fprintf(stdout, "\tClient:  Setting socket option NONBLOCK.......");
        if (apr_setsocketopt(sock, APR_SO_NONBLOCK, 1) != APR_SUCCESS) {
            apr_close_socket(sock);
            fprintf(stderr, "Couldn't set socket option\n");
            exit(-1);
        }
        fprintf(stdout, "OK\n");
    }

    apr_get_sockaddr(&remote_sa, APR_REMOTE, sock);
    apr_get_ipaddr(&remote_ipaddr, remote_sa);
    apr_get_port(&remote_port, remote_sa);
    apr_get_sockaddr(&local_sa, APR_LOCAL, sock);
    apr_get_ipaddr(&local_ipaddr, local_sa);
    apr_get_port(&local_port, local_sa);
    fprintf(stdout, "\tClient socket: %s:%u -> %s:%u\n", local_ipaddr, local_port, remote_ipaddr, remote_port);

    fprintf(stdout, "\tClient:  Trying to send data over socket.......");
    length = STRLEN;
    if (apr_send(sock, datasend, &length) != APR_SUCCESS) {
        apr_close_socket(sock);
        fprintf(stderr, "Problem sending data\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
   
    if (read_timeout != -1) {
        fprintf(stdout, "\tClient:  Setting read timeout.......");
        stat = apr_setsocketopt(sock, APR_SO_TIMEOUT, read_timeout);
        if (stat) {
            fprintf(stderr, "Problem setting timeout: %d\n", stat);
            exit(-1);
        }
        fprintf(stdout, "OK\n");
    }

    length = STRLEN; 
    fprintf(stdout, "\tClient:  Trying to receive data over socket.......");

    if ((stat = apr_recv(sock, datarecv, &length)) != APR_SUCCESS) {
        apr_close_socket(sock);
        fprintf(stderr, "Problem receiving data: %s (%d)\n", 
		apr_strerror(stat, msgbuf, sizeof(msgbuf)), stat);
        exit(-1);
    }
    if (strcmp(datarecv, "Recv data test")) {
        apr_close_socket(sock);
        fprintf(stderr, "I did not receive the correct data %s\n", datarecv);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Shutting down socket.......");
    if (apr_shutdown(sock, APR_SHUTDOWN_WRITE) != APR_SUCCESS) {
        apr_close_socket(sock);
        fprintf(stderr, "Could not shutdown socket\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Closing down socket.......");
    if (apr_close_socket(sock) != APR_SUCCESS) {
        fprintf(stderr, "Could not shutdown socket\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    return 1;
}
