/* * * * * * * * *
 * Dynamic hash table using extendible hashing with multiple keys per bucket,
 * resolving collisions by incrementally growing the hash table
 *
 * created for COMP20007 Design of Algorithms - Assignment 2, 2017
 * by Matt F
 *
 * Modified By:
 * Author:  Luke Hedt
 * StuID:   832153
 * Date:    13/05/2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "xtndbln.h"

// macro to calculate the rightmost n bits of a number x
#define rightmostnbits(n, x) (x) & ((1 << (n)) - 1)


// a bucket stores an array of keys
// it also knows how many bits are shared between possible keys, and the first 
// table address that references it
typedef struct xtndbln_bucket {
	int id;			// a unique id for this bucket, equal to the first address
					// in the table which points to it
	int depth;		// how many hash value bits are being used by this bucket
	int nkeys;		// number of keys currently contained in this bucket
	int64 *keys;	// the keys stored in this bucket
} Bucket;

// a hash table is an array of slots pointing to buckets holding up to 
// bucketsize keys, along with some information about the number of hash value 
// bits to use for addressing
struct xtndbln_table {
	Bucket **buckets;	// array of pointers to buckets
	int size;			// how many entries in the table of pointers (2^depth)
	int depth;			// how many bits of the hash value to use (log2(size))
	int bucketsize;		// maximum number of keys per bucket
};


/* 
 * Helper Functions
 */
/* Initialises a Bucket with size 'bucketsize' */
static void init_bucket(Bucket *bucket, int id, int depth, int bucksize) {
    /* Don't play with memory that isn't yours! */
    assert(bucket);

    /* Create a safe space for the keys in the bucket. */
    bucket->keys = malloc(sizeof(bucket->keys) * bucksize);
    assert(bucket->keys);

    /* Set all the relevant stuff up. */
    bucket->id = id;
    bucket->depth = depth;
    bucket->nkeys = 0;
}


/* 
 * Real Functions
 */

// initialise an extendible hash table with 'bucketsize' keys per bucket
XtndblNHashTable *new_xtndbln_hash_table(int bucketsize) {

	// calculate table address
	//int hash = h1(key);
	//int address = rightmostnbits(table->depth, hash);

    XtndblNHashTable *table = malloc(sizeof(*table));
    table->buckets = malloc(sizeof(table->buckets));
    table->size = 0;
    table->depth = 1;
    table->bucketsize = bucketsize;

    //init_bucket(*(table->buckets), address, 1, bucketsize);
}


// free all memory associated with 'table'
void free_xtndbln_hash_table(XtndblNHashTable *table) {
	fprintf(stderr, "not yet implemented\n");
}


// insert 'key' into 'table', if it's not in there already
// returns true if insertion succeeds, false if it was already in there
bool xtndbln_hash_table_insert(XtndblNHashTable *table, int64 key) {
	fprintf(stderr, "not yet implemented\n");
	return false;
}


// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool xtndbln_hash_table_lookup(XtndblNHashTable *table, int64 key) {
	fprintf(stderr, "not yet implemented\n");
	return false;
}


// print the contents of 'table' to stdout
void xtndbln_hash_table_print(XtndblNHashTable *table) {
	assert(table);
	printf("--- table size: %d\n", table->size);

	// print header
	printf("  table:               buckets:\n");
	printf("  address | bucketid   bucketid [key]\n");
	
	// print table and buckets
	int i;
	for (i = 0; i < table->size; i++) {
		// table entry
		printf("%9d | %-9d ", i, table->buckets[i]->id);

		// if this is the first address at which a bucket occurs, print it now
		if (table->buckets[i]->id == i) {
			printf("%9d ", table->buckets[i]->id);

			// print the bucket's contents
			printf("[");
			for(int j = 0; j < table->bucketsize; j++) {
				if (j < table->buckets[i]->nkeys) {
					printf(" %llu", table->buckets[i]->keys[j]);
				} else {
					printf(" -");
				}
			}
			printf(" ]");
		}
		// end the line
		printf("\n");
	}

	printf("--- end table ---\n");
}


// print some statistics about 'table' to stdout
void xtndbln_hash_table_stats(XtndblNHashTable *table) {
	fprintf(stderr, "not yet implemented\n");
}
