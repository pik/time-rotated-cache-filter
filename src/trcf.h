#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifndef TRCFCONSTANTS
#include "trcfconstants.h"
#define TRCFCONSTANTS
#endif

#define SIZE_N 0xffffffffffffffffLL

#ifndef min 
# define min(a,b) (((a)<(b)) ? (a) : (b))
#endif 
#ifndef max
# define max(a,b) (((a)<(b)) ? (b) : (a))
#endif

/* Note this will not work correctly for negative numbers */
#define  MOD(a, b)    (a%b)

typedef struct { 
    uint64_t size_k;
    uint64_t size_n;
    uint64_t count;
    uint64_t * state;
} cache_t;

typedef struct {
    uint32_t idx;
    uint32_t siz;
    uint64_t array[CHECK_TIMES_SIZ];
} check_times_t;

typedef struct {
    cache_t; 
    check_times_t * checks;
    uint64_t creation_time;
} time_rotated_cache_t; 

typedef struct {
    uint32_t idx;
    uint32_t siz;
    time_rotated_cache_t * caches[MAX_CACHES]; //correct?
} time_rotated_cache_filter_t; 

/** 
 * Cuckoo-Cache Methods
 * */
int cc_contains_item(cache_t * cc, uint64_t hh[]);
int cc_remove_item(cache_t * cc, uint64_t hh[]);
uint64_t * cc_add_item(cache_t * cc, uint64_t hh[]);
cache_t * new_cache(uint64_t size_k); 
void remove_cache(cache_t * cc);

/**
 * Time-Rotated-Cache Methods
 * */
int trc_contains_item(time_rotated_cache_t * trc, uint64_t hh[2]);
int trc_remove_item(time_rotated_cache_t * trc, uint64_t hh[2]);
uint64_t * trc_add_item(time_rotated_cache_t * trc, uint64_t hh[2]);
time_rotated_cache_t * new_time_rotated_cache(uint64_t size_k);
void remove_time_rotated_cache(time_rotated_cache_t * trc);
double trc_checks_per_count_second(time_rotated_cache_t * trc);

/** 
 * Time-Rotated-Cache-Filter Methods
 * */
void trcf_add_item(time_rotated_cache_filter_t * trcf, const char *s, size_t len);
int trcf_contains_item(time_rotated_cache_filter_t * trcf, const char *s, size_t len);
int trcf_remove_item(time_rotated_cache_t * trcf,  const char *s, size_t len);
int trcf_add_if_new(time_rotated_cache_filter_t * trcf, const char *s, size_t len);
time_rotated_cache_filter_t * new_time_rotated_cache_filter(uint64_t size_k);
void remove_time_rotated_cache_filter(time_rotated_cache_filter_t * trcf);
void trcf_add_cache_best_guess(time_rotated_cache_filter_t * trcf);
uint64_t trcf_total_size_k(time_rotated_cache_filter_t * trcf);
void trcf_add_stash(time_rotated_cache_filter_t * trcf, const char *s, size_t len);

uint64_t ct_get(check_times_t * ct, int i);
time_rotated_cache_t * trcf_get(time_rotated_cache_filter_t * trcf, int i);

