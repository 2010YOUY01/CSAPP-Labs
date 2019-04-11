#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
#include "cachelab.h"
#include <math.h>

struct cache
{
    long tag;
    bool valid;
    int time_stamp;
};

struct cachingResult{
    bool miss;
    bool hit;
    bool eviction;
};

void count(struct cachingResult r1, int* miss, int* hit, int* eviction){
    if(r1.hit){
        (*hit)++;
    }
    if(r1.miss){
        (*miss)++;
    }
    if(r1.eviction){
        (*eviction)++;
    }
}


/* addr - 64bit address
    b - bits in cache offset
    s - bits in cache set# */
long getSetIndex(long addr, int b, int s){
    long mask = 0x8000000000000000;
    mask =mask>>(64-s-b-1);
    mask = ~mask;
    long setIndex = mask & (addr);
    setIndex = setIndex >>b;
    return setIndex;
}
long getTag(long addr, int b, int s){
    long mask = 0x8000000000000000;
    mask =mask>>(s+b-1);
    mask = ~mask;
    long tag = addr >>(s+b);
    tag = tag & mask;
    return tag;
}

void initCache(struct cache** c, int setNum, int E){
    struct cache cInit = {.tag = 0, .valid = false, .time_stamp = 0};
    for(int i=0; i<setNum; i++){
        c[i] = malloc(sizeof(struct cache) * E);
        for(int j=0; j<E; j++){
            c[i][j] = cInit;
        }
    }
}

struct cachingResult updateCache(struct cache** c, long setIndex, long tag, int E){
    struct cachingResult r = {.miss = false, .hit = false, .eviction = false};
    struct cache* cacheLine = c[setIndex];
    int maxTimeStamp = cacheLine[0].time_stamp;
    int LRUIndex = 0;
    bool isFull = true;
    int emptyIndex = -1;
    //first iteration to find empty slot, see if hit
    for(int i=0; i<E; i++){
        if(cacheLine[i].time_stamp > maxTimeStamp){
            LRUIndex = i;
            maxTimeStamp = cacheLine[i].time_stamp;
        }
        if(cacheLine[i].valid == false){
            isFull = false;
            emptyIndex = i;
        }
        cacheLine[i].time_stamp++;
        bool isValid = cacheLine[i].valid;
        bool isTagMatch = (cacheLine[i].tag == tag);
        if(isValid && isTagMatch){
            r.hit = true;
        }
    }
    if(r.hit){
        return r;
    }
    else
    {
        r.miss = true;
        //update cache
        cacheLine[isFull?LRUIndex:emptyIndex].valid = true;
        cacheLine[isFull?LRUIndex:emptyIndex].tag = tag;
        cacheLine[isFull?LRUIndex:emptyIndex].time_stamp = 0;
        //update caching result
        r.eviction = isFull;
        //return
        return r;
    }
}

int main(int argc, char **argv)
{
    //read command line arguments
    int opt;
    char *optstring = "s:E:b:t:";
    char *tracefile;
    int s_val, E_val, b_val;
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_val = atoi(optarg);
            break;
        case 'E':
            E_val = atoi(optarg);
            break;
        case 'b':
            b_val = atoi(optarg);
            break;
        case 't':
            tracefile = optarg;
            break;
        default:
            printf("Wrong Arugumeng!\n");
            break;
        }
    }
    
    //make the cache
    int setNum = (int) (pow (2, s_val));
    struct cache *myCache[setNum];
    initCache(myCache, setNum, E_val);
    //setup the return valuse;
    int hits=0;
    int misss = 0;
    int evictions = 0;
    //read in tracefile
    FILE *pFile;
    pFile = fopen(tracefile, "r");
    char identifier;
    unsigned address;
    int size;
    while (fscanf(pFile, " %c %x,%d", &identifier, &address, &size) > 0)
    {
        printf("%c, %x, %d\n", identifier, address, size);
        long setIndex = getSetIndex(address, b_val, s_val);
        long tag = getTag(address, b_val, s_val);
        struct cachingResult r;
        switch (identifier)
        {
        case 'L':
            r = updateCache(myCache, setIndex, tag, E_val);
            count(r, &misss, &hits, &evictions);            
            break;
        case 'S':
            r = updateCache(myCache, setIndex, tag, E_val);
            count(r, &misss, &hits, &evictions);
            break;
        case 'M':
            r = updateCache(myCache, setIndex, tag, E_val);
            count(r, &misss, &hits, &evictions);
            r = updateCache(myCache, setIndex, tag, E_val);
            count(r, &misss, &hits, &evictions);
            break;
        default:
            break;
        }
    }
    fclose(pFile);
    //output
    printSummary(hits, misss, evictions);
    return 0;
}