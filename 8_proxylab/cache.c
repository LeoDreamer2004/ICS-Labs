/*
 *  Name: Yuan Zixuan
 *  Student ID: 2200010825
 * 
 *  cache.c - Cache implementation
 *  a simple cache implementation using LRU policy
 */

#include "cache.h"

/*
 * cache_init - initialize the cache
 */
void cache_init(cache_t* cache) {
    cache->readcnt = 0;
    cache->time = 0;
    Sem_init(&cache->mutex, 0, 1);
    Sem_init(&cache->w, 0, 1);
    for (int i = 0; i < MAX_CACHE_LINE; i++) {
        cache->lines[i].valid = 0;
        cache->lines[i].timestamp = 0;
    }
}

/*
 * cache_get - get the response from cache, return 1 if hit, 0 otherwise.
 *              If hit, the response will be copied to the response buffer.
 */
int cache_get(cache_t* cache, char* uri, char* response, size_t* size) {
    P(&cache->mutex);
    cache->readcnt++;
    if (cache->readcnt == 1)
        P(&cache->w);
    V(&cache->mutex);

    int i;
    for (i = 0; i < MAX_CACHE_LINE; i++) {
        if (cache->lines[i].valid && !strcmp(cache->lines[i].uri, uri)) {
            memcpy(response, cache->lines[i].response, MAXLINE);
            *size = cache->lines[i].size;
            break;
        }
    }

    P(&cache->mutex);
    cache->readcnt--;
    if (cache->readcnt == 0)
        V(&cache->w);
    V(&cache->mutex);

    if (i == MAX_CACHE_LINE)
        return 0;
    return 1;
}

/*
 * cache_write - write the response to cache, using LRU policy.
 */
void cache_write(cache_t* cache, char* uri, char* reponse, size_t size) {
    if (size > MAX_OBJECT_SIZE)
        return; /* too large to cache */

    P(&cache->w);

    int i, idx, minstamp = 0x7fffffff;
    for (i = 0; i < MAX_CACHE_LINE; i++) {
        if (!cache->lines[i].valid) {
            /* empty line */
            idx = i;
            break;
        } else if (cache->lines[i].timestamp < minstamp) {
            /* find the least recently used line */
            idx = i;
            minstamp = cache->lines[i].timestamp;
        }
    }
    cache->lines[idx].valid = 1;
    strcpy(cache->lines[idx].uri, uri);
    memcpy(cache->lines[idx].response, reponse, MAXLINE);
    cache->lines[idx].size = size;
    cache->lines[idx].timestamp = ++cache->time;

    V(&cache->w);
}
