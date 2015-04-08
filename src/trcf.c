#include <math.h>
#include "trcf.h"
#include "murmur.h"

/**
 * Circular Array Access
 * */
 
static inline void ct_append(check_times_t * ct, uint64_t tv) {
    ct->array[MOD((ct->idx++), ct->siz)] = tv;
}
inline uint64_t ct_get(check_times_t * ct, int i) {
    return (ct->array[(ct->idx > ct->siz ? MOD((i + ct->idx), ct->siz) : MOD(i, ct->idx ))]); 
}
static inline int ct_pos(check_times_t * ct, int i) {
    return (ct->idx > ct->siz ? MOD((i + ct->idx), ct->siz) : MOD(i, ct->idx));
}

static inline void trcf_append(time_rotated_cache_filter_t * trcf, time_rotated_cache_t * trc) {
    trcf->caches[MOD((trcf->idx++), trcf->siz)] = trc;
}
inline time_rotated_cache_t * trcf_get(time_rotated_cache_filter_t * trcf, int i) {
    return (trcf->caches[ (trcf->idx > trcf->siz ? MOD((i + trcf->idx), trcf->siz) : MOD(i,trcf->idx )) ]); 
}
static inline int trcf_pos(time_rotated_cache_filter_t * trcf, int i) {
    return (trcf->idx > trcf->siz ? MOD((i + trcf->idx), trcf->siz) : MOD(i, trcf->idx));
}

/* Time as uint64 */
uint64_t ct_gettime(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec*1000000000llu+ts.tv_nsec);
}

/* Hash 128 bits */
static void HASH_128(const char *s, size_t len, uint64_t hh[2]) { 
    MurmurHash3_x64_128(s, len, SALT_CONSTANT, hh);
}

/** 
 * Cuckoo-Cache Methods
 * */

static cache_t * init_cache(cache_t * cc, uint64_t size_k) {
    if ((cc->state = (uint64_t *)calloc(size_k, sizeof(uint64_t))) == NULL) {
        return NULL;
    }
    cc->size_k = size_k;
    cc->size_n = SIZE_N;
    cc->count = 0;
    return cc;
}

cache_t * new_cache(uint64_t size_k) {
    cache_t * cc;
    if ((cc = (cache_t *)malloc(sizeof(cache_t))) == NULL) {
        return NULL;
    }
    return init_cache(cc, size_k);
}

void remove_cache(cache_t * cc) {
    free(cc->state); 
    free(cc); 
}

uint64_t * cc_add_item(cache_t * cc, uint64_t hh[]) {
    uint64_t ind, fp, temp;
    int i;
    ind = hh[0] % cc->size_k;
    fp = hh[1];
    for (i=0; i < MAX_TRIES; i++) { 
        if (cc->state[ind] == fp) {
            return NULL;
        }
        temp=cc->state[ind];
        cc->state[ind] = fp;
        fp=temp;
        if (temp == 0) {
            cc->count++;
            return NULL;
        }
        ind = (ind^fp) % cc->size_k;
    }
    hh[0] = ind;
    hh[1] = fp;
    return hh;
}
int cc_remove_item(cache_t * cc, uint64_t hh[]) {
    uint64_t ind, fp;
    int i;
    ind = hh[0] % cc->size_k;
    fp = hh[1];
    for (i=0; i < MAX_TRIES; i++) { 
        if (cc->state[ind] == 0) {
            return 0; 
        }
        else if (cc->state[ind] == fp) {
            cc->state[ind] = 0;
            return 1;
        }
        ind = (ind^fp) % cc->size_k;
    }
    return 0;
}

int cc_contains_item(cache_t * cc, uint64_t hh[]) {
    uint64_t ind, fp;
    int i;
    ind = hh[0] % cc->size_k;
    fp = hh[1];
    for (i=0; i < 2; i++) { 
        if (cc->state[ind] == fp) {
            return 1;
        }
        ind = (ind^fp) % cc->size_k;
    }
    return 0;
}

/**
 * Time-Rotated-Cache Methods
 * */
 
static time_rotated_cache_t * init_time_rotated_cache(time_rotated_cache_t * trc, uint64_t size_k) {
    check_times_t * checks;
    if ((checks =(check_times_t *)malloc(sizeof(check_times_t))) == NULL) {
        return NULL;
    }
    trc->creation_time = ct_gettime();
    checks->siz = CHECK_TIMES_SIZ;
    checks->idx = 0;
    trc->checks = checks; 
    return (time_rotated_cache_t*)init_cache((cache_t*)trc, size_k);
}

time_rotated_cache_t * new_time_rotated_cache(uint64_t size_k) {
    time_rotated_cache_t * trc;
    if ((trc = (time_rotated_cache_t*)malloc(sizeof(time_rotated_cache_t))) == NULL) {
        return NULL;
    }
    return init_time_rotated_cache(trc, size_k);
}

void remove_time_rotated_cache(time_rotated_cache_t * trc) {
    free(trc->state);
    free(trc->checks);
    free(trc);
}

int trc_contains_item(time_rotated_cache_t * trc, uint64_t hh[2]) {
    ct_append(trc->checks, ct_gettime());
    return cc_contains_item((cache_t *)trc, hh);
}
int trc_remove_item(time_rotated_cache_t * trc, uint64_t hh[2]) {
    return cc_remove_item((cache_t *)trc, hh);
}
uint64_t * trc_add_item(time_rotated_cache_t * trc, uint64_t hh[2]) {
    return cc_add_item((cache_t *)trc, hh);
}

double trc_checks_per_count_second(time_rotated_cache_t * trc) {
    check_times_t * ct = trc->checks;
    return (double)min(ct->idx, ct->siz) / ((trc->count)*(ct_get(ct, -1) - ct_get(ct, 0) )/1000000000);
}

/** 
 * Time-Rotated-Cache-Filter Methods
 * */
 
static time_rotated_cache_filter_t * init_time_rotated_cache_filter(time_rotated_cache_filter_t * trcf, uint64_t size_k) {
    time_rotated_cache_t * trc;
    if ((trc = new_time_rotated_cache(size_k)) == NULL) {
        return NULL;
    }
    trcf->caches[0] = trc;
    trcf->idx = 1;
    trcf->siz = MAX_CACHES;
    return trcf;
}

time_rotated_cache_filter_t * new_time_rotated_cache_filter(uint64_t size_k) {
    time_rotated_cache_filter_t * trcf; 
    if ((trcf = (time_rotated_cache_filter_t *)malloc(sizeof(time_rotated_cache_filter_t))) == NULL) {
        return NULL;
    }
    return init_time_rotated_cache_filter(trcf, size_k);
}

void remove_time_rotated_cache_filter(time_rotated_cache_filter_t * trcf) {
    int i;
    for (i=1; i< min(trcf->idx, trcf->siz) +1; i++) {
        remove_time_rotated_cache(trcf_get(trcf, -i));
    }
    free(trcf);
}

/*
void trcf_remove_cache(time_rotated_cache_filter_t * trcf) {
    //check that not trying to remove a NULL CACHE
    time_rotated_cache_t * trc = trcf_get(trcf, 0);
    printf("Removing Cache trc->size_k %i", trc->size_k);
    remove_time_rotated_cache(trc);
    trcf->idx = min(trcf->idx -1, trcf->siz - 1);
}
*/

/* Estimate optimal size for a new cache 
 * Attempts to regularize to about about ~1% activity for the last cache. 
 * Returns INIT_SIZE_K if only one cache is present
 * */
uint64_t trcf_best_guess_size(time_rotated_cache_filter_t * trcf) {
    double load_increase, ttf1, ttf2, check_count_scalar;
    time_rotated_cache_t * trc1, * trc2;
    if (trcf->idx < 2) {
        return trcf_get(trcf, -1)->size_k;
    }
    trc1 = trcf_get(trcf, -1);
    trc2 = trcf_get(trcf, -2);
    /* ttf = elements added to cache / time */
    ttf1 = ((double)trc1->count / (ct_gettime() - trc1->creation_time));
    ttf2 = ((double)trc2->count / (trc1->creation_time - trc2->creation_time));
    load_increase = ttf1/ttf2;
    /* compare checks per second ratio relative to expected with ~1% decay for the last cache */ 
    //printf("Checks per count second trc1    %.8f   trc2    %.8f \n", trc_checks_per_count_second(trc1), trc_checks_per_count_second(trc2));
    check_count_scalar = (log(100)/log(MAX_CACHES))*(trc_checks_per_count_second(trc2)/trc_checks_per_count_second(trc1));
    
    if (check_count_scalar != check_count_scalar) {
        check_count_scalar=1;
    }
    //printf("load_increase %.8f check_count_scaler %.8f \n", load_increase, check_count_scalar); 
    load_increase *= check_count_scalar;
    /* Don't scale more than MAX_RESCALE in either direction*/
    return (uint64_t)(load_increase > 1.0 ? min(load_increase,MAX_RESCALE)*trc1->size_k : max(load_increase, MAX_RESCALE)*trc1->size_k); 
}
    
void trcf_add_cache(time_rotated_cache_filter_t * trcf, uint64_t size_k) {
    if (trcf->idx > trcf->siz) {
        remove_time_rotated_cache(trcf_get(trcf, 0));
    }
    trcf_append(trcf, new_time_rotated_cache(size_k)); 
}

uint64_t trcf_total_size_k(time_rotated_cache_filter_t * trcf) {
    uint64_t total_size = 0;
    int i;
    for (i=1; i < min(trcf->idx, trcf->siz) + 1; i++) {
        total_size += trcf_get(trcf, -i)->size_k;
    }
    return total_size;
}

/* Add a cache scaled to best-guess-size, don't rescale below MIN_SIZE_K */ 
void trcf_add_cache_best_guess(time_rotated_cache_filter_t * trcf) { 
    uint64_t new_size_k;
    uint64_t current_memory = trcf_total_size_k(trcf);
    if ((new_size_k = trcf_best_guess_size(trcf)) > (MAX_MEMORY - SIZE_N*current_memory)) {
        new_size_k = (MAX_MEMORY - SIZE_N*current_memory)/SIZE_N;
    }
    //printf("Adding Cache... new_size_k: %i \n", max(new_size_k, MIN_SIZE_K));
    trcf_add_cache(trcf, max(new_size_k, MIN_SIZE_K));
}

void trcf_add_item(time_rotated_cache_filter_t * trcf, const char *s, size_t len) {
    uint64_t hh[2];
    time_rotated_cache_t * trc1;
    HASH_128(s, len, hh);
    trc1= trcf_get(trcf, -1);
    if (trc_add_item(trc1, hh) != NULL) {
        /* If a value was popped and the cache is at 50% occupancy add a cache */
        if (((double)trc1->count / trc1->size_k) > MAX_OCCUPANCY) {
            trcf_add_cache_best_guess(trcf);
        }
    }
}

int trcf_contains_item(time_rotated_cache_filter_t * trcf, const char *s, size_t len)  {
    uint64_t hh[2];
    int i;
    HASH_128(s, len, hh);
    for (i=1; i < min(trcf->idx, trcf->siz) + 1; i++) { 
        if (trc_contains_item(trcf_get(trcf, -i), hh) == 1) {
            return 1;
        }
    }
    return 0;
}

int trcf_add_if_new(time_rotated_cache_filter_t * trcf, const char *s, size_t len) {
    uint64_t hh[2];
    int i;
    HASH_128(s, len, hh);
    time_rotated_cache_t * trc1;
    for (i=1; i < min(trcf->idx, trcf->siz) + 1; i++) { 
        if (trc_contains_item(trcf_get(trcf, -i), hh) == 1) {
            return 0;
        }
    }
    trc1 = trcf_get(trcf, -1);
    if ((trc_add_item(trc1, hh) != NULL ) && (((double)trc1->count / trc1->size_k) > MAX_OCCUPANCY)) {
        trcf_add_cache_best_guess(trcf);
    }
    return 1;
}


