// id:  name: 
/*
This is a C program that simulates a cache memory system 
using the command line arguments. 
The program takes as input the cache size, associativity, block size, 
and a trace file that describes a sequence of memory accesses. 
The cache is represented as an array of cache lines, 
each of which contains a valid bit, tag, and LRU counter. 
The LRU counter is used to determine which cache line to 
evict in the case of a cache miss and an eviction is needed.
*/

#include "cachelab.h"
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//cache_line
typedef struct{
    int valid_bit;
    int tag;
    int LRU_counter;
}cache_line;

cache_line **caches = NULL;
int hit, miss, eviction;
int s, E, b, S, B;
int time_stamp;

void print_help(){
    printf("Usage: ./csim-ref [-hv]"
            "-s <s> -E <E> -b <b> -t <tracefile>\n"
            "-h: Optional help flag that prints usage info\n"
            "-v: Optional verbose flag that displays trace info\n"
            "-s <s>: Number of set index bits (S = 2^s is the number of sets)\n"
            "-E <E>: Associativity (number of lines per set)\n"
            "-b <b>: Number of block bits (B = 2^b is the block size)\n"
            "-t <tracefile>: Name of the valgrind trace to replay\n");
}

int is_hit(int set, int tag){
    for(int i = 0; i < E; ++i){
        if(caches[set][i].valid_bit == 1 && caches[set][i].tag == tag){
            return i;
        }
    }
    return -1;
}

bool need_eviction(int set){
    for(int i = 0; i < E; ++i){
        if(caches[set][i].valid_bit == 0){
            return false;
        }
    }
    return true;
}

void handle_miss(int set, int tag){
    //find an empty line
    for(int i = 0; i < E; ++i){
        if(caches[set][i].valid_bit == 0){
            caches[set][i].valid_bit = 1;
            caches[set][i].tag = tag;
            caches[set][i].LRU_counter = time_stamp++;
            return;
        }
    }
}

void handle_eviction(int set, int tag){
    //find the smallest LRU_counter
    int min_LRU_counter = caches[set][0].LRU_counter;
    int min_index = 0;
    for(int i = 1; i < E; ++i){
        if(caches[set][i].valid_bit && 
        caches[set][i].LRU_counter < min_LRU_counter){
            min_LRU_counter = caches[set][i].LRU_counter;
            min_index = i;
        }
    }
    caches[set][min_index].tag = tag;
    caches[set][min_index].LRU_counter = time_stamp++;
}

void load_data(int set, int b_offset, int tag, int verbose){
    //check if hit
    int hit_index = is_hit(set, tag);
    if(hit_index != -1){
        hit++;
        if(verbose){
            printf(" hit");
        }
        caches[set][hit_index].LRU_counter = time_stamp++;
        return;
    }
    miss++;
    if(verbose){
        printf(" miss");
    }
    bool eviction_flag = need_eviction(set);
    if(eviction_flag){ //eviction
        eviction++;
        if(verbose){
            printf(" eviction");
        }
        handle_eviction(set, tag);
    }else{ 
        handle_miss(set, tag);
    }
}

void store_data(int set, int b_offset, int tag, int verbose){
    int hit_index = is_hit(set, tag);
    if(hit_index != -1){
        hit++;
        if(verbose){
            printf(" hit");
        }
        caches[set][hit_index].LRU_counter = time_stamp++;
        return;
    }
    miss++;
    if(verbose){
        printf(" miss");
    }
    bool eviction_flag = need_eviction(set);
    if(eviction_flag){ //eviction
        eviction++;
        if(verbose){
            printf(" eviction");
        }
        handle_eviction(set, tag);
    }else{
        handle_miss(set, tag);
    }
}

void modify_data(int set, int b_offset, int tag, int verbose){
    //1-load
    load_data(set, b_offset, tag, verbose);
    //2-store
    store_data(set, b_offset, tag, verbose);
}


int main(int argc, char **argv)
{
    hit = 0, miss = 0, eviction = 0;
    time_stamp = 1;
    int verbose = 0;
    
    //parse line
    int opt;
    char trace_file[100];
    while(-1 != (opt = getopt(argc, argv, "hvs:E:b:t:"))){
        switch(opt){
            case 'h':
                print_help();
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
                memcpy(trace_file, optarg, strlen(optarg));
                break;
            default:
                break;
        }
    }
    B = 1 << b;
    S = 1 << s;

    //initialize cache lines
    caches = (cache_line**)malloc(sizeof(cache_line*) * S);
    for(int i = 0; i < S; ++i){
        caches[i] = (cache_line*)malloc(sizeof(cache_line) * E);
        for(int j = 0; j < E; ++j){
            caches[i][j].valid_bit = 0;
            caches[i][j].tag = -1;
            caches[i][j].LRU_counter = 0;
        }
    }

    //parse line in trace file
    FILE *fp = fopen(trace_file, "r");
    if (fp == NULL) {
        printf("Error: could not open trace file %s\n", trace_file);
        exit(1);
    }
    char operation;
    unsigned address;
    int	size;
    int cur_set, cur_tag, cur_B;
    while(fscanf(fp, "%c %x, %d", &operation, &address, &size) > 0){
        cur_tag = address >> (s + b);
        cur_set = (address >> b) & ((1 << s) - 1);
        cur_B = address & ((1 << b) - 1);
        switch (operation)
        {
        case 'I':
            break;
        case 'L':{
            if(verbose){
                printf("%c %x, %d", operation, address, size);
            }
            load_data(cur_set, cur_B, cur_tag, verbose);
            if(verbose) printf("\n");
            break;
        }
        case 'S':{
            if(verbose){
                printf("%c %x, %d", operation, address, size);
            }
            store_data(cur_set, cur_B, cur_tag, verbose);
            if(verbose) printf("\n");
            break;
        }
        case 'M':{
            if(verbose){
                printf("%c %x, %d", operation, address, size);
            }
            //modify = load + store
            modify_data(cur_set, cur_B, cur_tag, verbose);
            if(verbose) printf("\n");
            break;
        }
        default:
            break;
        }
    }
    fclose(fp);

    //free memory
    for(int i = 0; i < S; ++i){
        free(caches[i]);
    }
    free(caches);

    printSummary(hit, miss, eviction);  //hits, misses, evictions
    return 0;
}
