/**
 * Name: Yuan Zixuan
 * Student ID: 2200010825
 * Cache Lab
 * csim.c - A Simulator for Cache
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "cachelab.h"

// cache line
typedef struct {
    int valid;               // 0: invalid, 1: valid
    unsigned int tag;        // tag
    unsigned int timeStamp;  // last visit time
} CacheLine;
typedef CacheLine* CacheSet;

int s, E, b, S, B;
CacheSet* cache;
FILE* fp;
int hits, misses, evictions;  // final results
int verbose = 0;

void printUsage() {
    printf(
        "Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n"
        "Options:\n"
        "  -h         Print this help message.\n"
        "  -v         Optional verbose flag.\n"
        "  -s <num>   Number of set index bits.\n"
        "  -E <num>   Number of lines per set.\n"
        "  -b <num>   Number of block offset bits.\n"
        "  -t <file>  Trace file.\n\n"
        "Examples:\n"
        "  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n"
        "  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
    return;
}

/**
 * 1. Get arguments from command line
 */
int getArgs(int argc, char* argv[]) {
    int opt;
    char* traceFile;

    // get arguments, -h -v -s -E -b -t
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                printUsage();
                break;
            case 'v':
                verbose = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                traceFile = optarg;
                break;
            default:
                break;
        }
    }

    // check if the arguments are valid
    if (s <= 0 || E <= 0 || b <= 0 || traceFile == NULL) {
        exit(-1);
    }

    // prepare global variables
    fp = fopen(traceFile, "r");
    S = 1 << s;
    B = 1 << b;
    return 0;
}

/**
 * 2. Initialize cache
 */
void initCache() {
    // allocate memory for cache
    cache = (CacheSet*)malloc(S * sizeof(CacheSet));
    for (int i = 0; i < S; i++) {
        cache[i] = (CacheLine*)malloc(E * sizeof(CacheLine));
        cache[i]->valid = 0;
        cache[i]->tag = 0;
        cache[i]->timeStamp = 0;
    }
}

/**
 * 3. Simulate cache
 */
void updateStamp() {
    for (int i = 0; i < S; i++) {
        for (int j = 0; j < E; j++) {
            if (cache[i][j].valid == 1) {
                cache[i][j].timeStamp++;
            }
        }
    }
}

/**
 * 4. Update cache
 * @param op operation
 * @param address data address
 * @param size data size
 */
void updateCache(char op, size_t address, unsigned int size) {
    unsigned int tag = address >> (s + b);
    unsigned int setIndex = (address >> b) & (S - 1);
    CacheSet set = cache[setIndex];  // the set to be operated

    for (int i = 0; i < E; i++) {
        // find the cache line
        if (set[i].valid && set[i].tag == tag) {  // hit
            hits++;                               // update hits
            set[i].timeStamp = 0;                 // restore the time
            if (verbose) {
                printf(" hit");
            }
            return;
        }
    }

    // miss
    misses++;  // update misses
    for (int i = 0; i < E; i++) {
        if (!set[i].valid) {       // empty line
            set[i].valid = 1;      // set valid
            set[i].tag = tag;      //  set tag
            set[i].timeStamp = 0;  // restore the time
            if (verbose) {
                printf(" miss");
            }
            return;
        }
    }

    // eviction
    evictions++;  // update evictions
    unsigned int maxStamp = 0;
    int maxStampIndex;
    for (int i = 0; i < E; i++) {
        // find the max time stamp
        if (set[i].timeStamp > maxStamp) {
            maxStamp = set[i].timeStamp;
            maxStampIndex = i;
        }
    }
    if (verbose) {
        printf(" miss eviction");
    }
    set[maxStampIndex].tag = tag;      // set tag
    set[maxStampIndex].timeStamp = 0;  // restore the time
    return;
}

/**
 * 5. Simulate
 */
void simulate() {
    char op;
    size_t address;
    unsigned int size;
    while (fscanf(fp, "%c", &op) != EOF) {
        // read the instruction, and ignore I
        if (op == ' ') {
            fscanf(fp, "%c", &op);
        }
        fscanf(fp, " %lx,%u\n", &address, &size);

        if (verbose) {
            printf("%c %lx,%u", op, address, size);
        }

        // update cache
        switch (op) {
            case 'L':  // load
                updateCache(op, address, size);
                break;
            case 'M':  // modify: load & store
                updateCache(op, address, size);
                /* breakthrough */
            case 'S':  // store
                updateCache(op, address, size);
                break;
            default:
                break;
        }

        if (verbose) {
            printf("\n");
        }

        updateStamp();
    }
}

/**
 * 6. Free cache
 */
void freeCache() {
    fclose(fp);
    for (int i = 0; i < S; i++) {
        free(cache[i]);
    }
    free(cache);
}

int main(int argc, char* argv[]) {
    getArgs(argc, argv);
    initCache();
    simulate();
    freeCache();
    printSummary(hits, misses, evictions);
    return 0;
}