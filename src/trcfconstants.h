/**
 * Define Time-Rotated-Cache-Filter Constants
 * */
 
/* Number of iterations before replace on insert*/
#define MAX_TRIES 8
/* Max filter memory (total of all caches) in bytes */
#define MAX_MEMORY 10*1024*1024
/* Minimum assignable cache size */
#define MIN_SIZE_K 512
/* Maximum number of caches in filter */
#define MAX_CACHES 5
/* Add new cache to filter if occupancy exceeds this ratio */
#define MAX_OCCUPANCY 0.5
/* Size of contains_item timestamp array per cache */
#define CHECK_TIMES_SIZ 2<<9
/* Max cache rescale on trcf_best_guess_size */
#define MAX_RESCALE 4
/* Murmur Hash Seed */
#define SALT_CONSTANT 0x97c29b3a
