#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "murmur.h"
#include "trcf.h"

#define CAPACITY 8192*2
#define ERROR_RATE .01

enum {
    TEST_PASS,
    TEST_WARN,
    TEST_FAIL,
};

struct stats {
    int true_positives;
    int true_negatives;
    int false_positives;
    int false_negatives;
};

static void chomp_line(char *word)
{
    char *p;
    if ((p = strchr(word, '\r'))) {
        *p = '\0';
    }
    if ((p = strchr(word, '\n'))) {
        *p = '\0';
    }
}

static void print_cc_stat(cache_t *cc) {
    printf("Cache size_k:         %llu \n", cc->size_k);
    printf("Fingerprint (bytes):  %i \n", sizeof(cc->size_n));
    printf("Total mememory usage: %i \n", cc->size_k*sizeof(SIZE_N));
    printf("Elements added:       %i" "\n", cc->count );
}

static void print_trc_stat(time_rotated_cache_t *trc) {
    print_cc_stat((cache_t *)trc);
    //printf("Creation Time:        %llu \n", trc->creation_time);
    printf("Checks per Count Second %.6f \n", trc_checks_per_count_second(trc));
    //printf("Checks[0] %llu    Checks[-1]      %llu \n", ct_get(trc->checks, 0), ct_get(trc->checks, -1));
}
static void print_trcf_stat(time_rotated_cache_filter_t * trcf) {
    printf("Number of Caches in Filter:  %i \n" 
           "Total size_k:                %llu \n" 
           "Fingerprint (bytes):         %i \n"
           "Total mememory usage:        %llu \n",
           min(trcf->idx, trcf->siz),
           trcf_total_size_k(trcf),
           sizeof(SIZE_N),
           sizeof(SIZE_N)*trcf_total_size_k(trcf)
           );
}

static int print_results(struct stats *results) {
    float false_positive_rate = (float)results->false_positives /
                                (results->false_positives + results->true_negatives);
    float false_negative_rate = (float)results->false_negatives / 
                                (results->false_negatives + results->true_positives);
    printf("----------------------------\n");
    printf("True positives:     %7d"   "\n"
           "True negatives:     %7d"   "\n"
           "False positives:    %7d"   "\n"
           "False negatives:    %7d"   "\n"
           "False positive rate: %.4f" "\n"
           "False negative rate: %.4f" "\n",
           results->true_positives,
           results->true_negatives,
           results->false_positives,
           results->false_negatives,
           false_positive_rate,
           false_negative_rate             
           );
           
    if (false_negative_rate > ERROR_RATE) {
        printf("TEST WARN (false negative rate too high)\n");
        return TEST_FAIL;
    } else if (false_positive_rate > 0) {
        printf("TEST FAIL (false positives exist)\n");
        return TEST_WARN;
    } else {
        printf("TEST PASS\n");
        return TEST_PASS;
    }
}
static void score(int positive, int should_positive, struct stats *stats, const char *key)
{
    if (should_positive) {
        if (positive) {
            stats->true_positives++;
        } else {
            stats->false_negatives++;
           // fprintf(stderr, "False negative: '%s'\n", key);
        }
    } else {
        if (positive) {
            stats->false_positives++;
            fprintf(stderr, "ERROR: False positive: '%s'\n", key);
        } else {
            stats->true_negatives++;
        }
    }
}

int test_cc(const char *words_file) {
    cache_t * cc;
    int i, ret;
    int replacements = 0;
    char word[256];
    FILE *fp, *out;
    uint64_t hh[2];
    struct stats results = { 0 };
    printf("\n** Testing Cuckoo Cache \n");
    if (!(cc = new_cache(CAPACITY*2))) {
        fprintf(stderr, "ERROR: Could not create cache\n");
        return TEST_FAIL;
    }
    if (!(fp = fopen(words_file, "r"))) {
        fprintf(stderr, "ERROR: Could not open words file\n");
        return TEST_FAIL;
    }
    /*
    if (!(out = fopen("hh.txt", "w"))) {
        fprintf(stderr, "ERROR: Could not open writing file\n");
        return TEST_FAIL;
    }*/
    for (i = 0; i< CAPACITY; i++) {
        fgets(word, sizeof(word), fp);
        chomp_line(word);
        MurmurHash3_x64_128(word, strlen(word), SALT_CONSTANT, hh);
        //fprintf(out, "%llu, %llu \n", hh[0], hh[1]);
        if (cc_add_item(cc, hh) != NULL) {
            replacements++;
        }
    }
    fseek(fp, 0, SEEK_SET);
    for (i = 0; i< CAPACITY; i++) {
        fgets(word, sizeof(word), fp);
        chomp_line(word);
        MurmurHash3_x64_128(word, strlen(word), SALT_CONSTANT, hh);
        score(cc_contains_item(cc, hh), 1, &results, word);
        
    }
    for (i = 0; i< CAPACITY; i++) {
        fgets(word, sizeof(word), fp);
        chomp_line(word);
        MurmurHash3_x64_128(word, strlen(word), SALT_CONSTANT, hh);
        score(cc_contains_item(cc, hh), 0, &results, word);
        
    }
    fclose(fp);
    print_cc_stat(cc);
    printf("Replacements %i \n", replacements);
    remove_cache(cc); 
    return print_results(&results);
}

int test_trc(const char *words_file) {
    time_rotated_cache_t * trc;
    int i;
    int replacements = 0;
    char word[256];
    FILE *fp;
    uint64_t hh[2];
    struct stats results = { 0 };
    printf("\n** Testing Time-Rotated-Cache \n");
    if (!(trc = new_time_rotated_cache(CAPACITY*2))) {
        fprintf(stderr, "ERROR: Could not create cache\n");
        return TEST_FAIL;
    }
    if (!(fp = fopen(words_file, "r"))) {
        fprintf(stderr, "ERROR: Could not open words file\n");
        return TEST_FAIL;
    }
    for (i = 0; i< CAPACITY; i++) {
        fgets(word, sizeof(word), fp);
        chomp_line(word);
        MurmurHash3_x64_128(word, strlen(word), SALT_CONSTANT, hh);
        if ( trc_add_item(trc, hh) != NULL) {
            replacements++;
        }
    }
    fseek(fp, 0, SEEK_SET);
    for (i = 0; i< CAPACITY; i++) {
        fgets(word, sizeof(word), fp);
        chomp_line(word);
        MurmurHash3_x64_128(word, strlen(word), SALT_CONSTANT, hh);
        score(trc_contains_item(trc, hh), 1, &results, word);
        
    }
    for (i = 0; i< CAPACITY; i++) {
        usleep(1);
        fgets(word, sizeof(word), fp);
        chomp_line(word);
        MurmurHash3_x64_128(word, strlen(word), SALT_CONSTANT, hh);
        score(trc_contains_item(trc, hh), 0, &results, word);
    }
    fclose(fp);
    print_trc_stat(trc);
    printf("Replacements %i \n", replacements);
    remove_time_rotated_cache(trc); 
    return print_results(&results);
}

int test_trcf(const char *words_file) {
    time_rotated_cache_filter_t * trcf;
    int i;
    char word[256];
    FILE *fp;
    uint64_t hh[2];
    struct stats results = { 0 };
    printf("\n** Testing Time-Rotated-Cache-Filter \n");
    /* Create a filter at SIZE_K 1/2 of Items to be added */ 
    if (!(trcf = new_time_rotated_cache_filter(CAPACITY/2))) {
        fprintf(stderr, "ERROR: Could not create cache\n");
        return TEST_FAIL;
    }
    if (!(fp = fopen(words_file, "r"))) {
        fprintf(stderr, "ERROR: Could not open words file\n");
        return TEST_FAIL;
    }
    
    for (i = 0; i< CAPACITY; i++) {
        fgets(word, sizeof(word), fp);
        chomp_line(word);
        MurmurHash3_x64_128(word, strlen(word), SALT_CONSTANT, hh);
        trcf_add_item(trcf, word, strlen(word));
        trcf_contains_item(trcf, word, strlen(word));
    }
    fseek(fp, 0, SEEK_SET);
    for (i = 0; i< CAPACITY; i++) {
        fgets(word, sizeof(word), fp);
        chomp_line(word);
        score(trcf_contains_item(trcf, word, strlen(word)), 1, &results, word);
        
    }    
    for (i = 0; i< CAPACITY*2; i++) {
        usleep(1);
        fgets(word, sizeof(word), fp);
        chomp_line(word);
        MurmurHash3_x64_128(word, strlen(word), SALT_CONSTANT, hh);
        score(trcf_contains_item(trcf, word, strlen(word)), 0, &results, word);
    }
    fclose(fp);
    print_trcf_stat(trcf);
    remove_time_rotated_cache_filter(trcf); 
    return print_results(&results);
}

int main(int argc, char *argv[]) {
    int i, failures = 0, warnings = 0;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <words_file>\n", argv[0]);
        return 1;
    }
    int (*tests[])(const char *) = {
        test_cc,
        test_trc,
        test_trcf,
        NULL,
    };
    for (i = 0; tests[i] != NULL;  i++) {
        int result = (tests[i])(argv[1]);
        if (result == TEST_FAIL) {
            failures++;
        } else if (result == TEST_WARN) {
            warnings++;
        }
    }
    printf("\n** %d failures, %d warnings\n", failures, warnings);
    if (failures) {
        return EXIT_FAILURE;
    } else {
        return EXIT_SUCCESS;
    }
}

