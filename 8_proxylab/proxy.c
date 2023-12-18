/*
 * Proxy Lab
 * Name: Yuan Zixuan
 * Student ID: 2200010825
 *
 * proxy.c - A simple proxy
 * Usage: ./proxy <port>
 * - Using thread pool to handle requests
 * - Using cache to improve performance
 */

#include "cache.h"
#include "csapp.h"
#include "pack.h"

#define NTHREADS 4 /* number of threads */

void doit(int clientfd);
void* thread(void* vargp);
cache_t cache; /* global cache */

int main(int argc, char** argv) {
    int listenfd, *connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* Ignore SIGPIPE */
    Signal(SIGPIPE, SIG_IGN);

    /* Initialize cache */
    cache_init(&cache);

    /* Listen to port */
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port,
                    MAXLINE, 0);
        Pthread_create(&tid, NULL, thread, connfd);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
    }
}

void* thread(void* vargp) {
    int connfd = *((int*)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);
    Close(connfd);
    return NULL;
}

void doit(int clientfd) {
    int serverfd;
    char method[MAXLINE], uri[MAXLINE + 50], version[MAXLINE];
    char buf[MAXLINE], cacheline[MAX_OBJECT_SIZE];
    char request[MAXLINE];
    rio_t rio_client, rio_server;
    size_t n, size = 0;
    uri_t parsed_uri;

    /* Read request line and headers */
    Rio_readinitb(&rio_client, clientfd);
    Rio_readlineb(&rio_client, buf, MAXLINE + 50);
    sscanf(buf, "%s %s %s", method, uri, version);

    /* Check request */
    if (strlen(buf) > MAXLINE) {
        printf("Bad request\n");
        return;
    }
    if (strcasecmp(method, "GET")) {
        printf("Proxy does not implement this method\n");
        return;
    }

    /* Try to get response from cache */
    if (cache_get(&cache, uri, buf, &size)) {
        Rio_writen(clientfd, buf, size);
        return;
    }

    /* Parse URI from GET request */
    parse_uri(uri, &parsed_uri);

    /* Build the http header which will send to the end server */
    build_header(&rio_client, &parsed_uri, request);

    /* Connect to end server */
    serverfd = Open_clientfd(parsed_uri.host, parsed_uri.port);
    if (serverfd < 0) {
        printf("connection failed\n");
        return;
    }

    /* Send request to end server */
    Rio_readinitb(&rio_server, serverfd);
    Rio_writen(serverfd, request, strlen(request));

    /* Forward response to client */
    memset(buf, 0, sizeof(buf));
    while ((n = Rio_readlineb(&rio_server, buf, MAXLINE))) {
        Rio_writen(clientfd, buf, n);
        if ((size += n) <= MAX_OBJECT_SIZE) /* in case of buffer overflow */
            memcpy(cacheline + size - n, buf, n);
    }

    /* Write response to cache */
    cache_write(&cache, uri, cacheline, size);
    Close(serverfd);
}
