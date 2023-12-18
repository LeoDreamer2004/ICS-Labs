/*
 *  Name: Yuan Zixuan
 *  Student ID: 2200010825
 *
 *  pack.c - Helper functions on packet for proxy
 */

#include "pack.h"

/* Constants */
static const char* user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
static const char* conn_hdr = "Connection: close\r\n";
static const char* proxy_hdr = "Proxy-Connection: close\r\n";

/*
 * parse_uri - parse URI into host, path and port
 */
void parse_uri(char* uri, uri_t* parsed_uri) {
    int temp = 80;
    char* uri_copy = strdup(uri); /* in case of modification */
    char* hostptr = strstr(uri_copy, "//") + 2;
    char* portptr = strstr(hostptr, ":");
    char* pathptr = strstr(hostptr, "/");

    if (portptr) {
        *portptr = '\0';
        strncpy(parsed_uri->host, hostptr, MAXLINE);
        sscanf(portptr + 1, "%d%s", &temp, parsed_uri->path);
    } else if (pathptr) {
        *pathptr = '\0';
        strncpy(parsed_uri->host, hostptr, MAXLINE);
        *pathptr = '/';
        strncpy(parsed_uri->path, pathptr, MAXLINE);
    } else {
        strncpy(parsed_uri->host, hostptr, MAXLINE);
        strcpy(parsed_uri->path, "");
    }
    sprintf(parsed_uri->port, "%d", temp);
    return;
}

/*
 * build_header - build the http header which will send to the end server
 */
void build_header(rio_t* rio, uri_t* uri, char* header) {
    /* I just want to avoid buffer overflow!!! */
    char temp[MAXLINE * 3];
    char buf[MAXLINE * 10];
    sprintf(buf, "GET %s HTTP/1.0\r\n", uri->path);

    while (Rio_readlineb(rio, temp, MAXLINE) > 0) {
        if (temp[1] == '\n')
            break;
        if (strstr(temp, "Host:"))
            continue;
        if (strstr(temp, "User-Agent:"))
            continue;
        if (strstr(temp, "Connection:"))
            continue;
        if (strstr(temp, "Proxy Connection:"))
            continue;
        strcat(buf, temp);
    }

    sprintf(temp, "Host: %s:%s\r\n", uri->host, uri->port);
    strcat(buf, temp);
    strcat(buf, user_agent_hdr);
    strcat(buf, conn_hdr);
    strcat(buf, proxy_hdr);
    strcat(buf, "\r\n");
    strncpy(header, buf, MAXLINE);
    return;
}
