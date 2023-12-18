#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct {
    char uri[MAXLINE];
    char response[MAX_OBJECT_SIZE];
    size_t size;
    int valid;
    int timestamp;
} cacheLine_t;

#define MAX_CACHE_LINE (MAX_CACHE_SIZE / sizeof(cacheLine_t))

typedef struct {
    cacheLine_t lines[MAX_CACHE_LINE];
    size_t readcnt;
    sem_t mutex; /* readcnt access */
    sem_t w;     /* write access */
    int time;    /* current time */
} cache_t;

void cache_init(cache_t* cache);
int cache_get(cache_t* cache, char* uri, char* response, size_t* size);
void cache_write(cache_t* cache, char* uri, char* reponse, size_t size);
