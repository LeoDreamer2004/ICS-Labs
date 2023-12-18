#include "csapp.h"

typedef struct {
    char host[MAXLINE];
    char port[MAXLINE];
    char path[MAXLINE];
} uri_t;

void parse_uri(char* uri, uri_t* parsed_uri);
void build_header(rio_t* rio, uri_t* uri, char* header);
