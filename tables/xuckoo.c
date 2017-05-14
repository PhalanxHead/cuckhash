/* * * * * * * * *
* Dynamic hash table using a combination of extendible hashing and cuckoo
* hashing with a single keys per bucket, resolving collisions by switching keys 
* between two tables with two separate hash functions and growing the tables 
* incrementally in response to cycles
*
* created for COMP20007 Design of Algorithms - Assignment 2, 2017
* by Matt F
*
* Modified By:
* Author:   Luke Hedt
* StuID:    832153
* Date:     14/05/2017
*
* Code based on cuckoo.c and xtndbl1.c unless stated otherwise.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "xuckoo.h"

// macro to calculate the rightmost n bits of a number x
#define rightmostnbits(n, x) (x) & ((1 << (n)) - 1)

// a bucket stores a single key (full=true) or is empty (full=false)
// it also knows how many bits are shared between possible keys, and the first 
// table address that references it
typedef struct bucket {
	int id;		// a unique id for this bucket, equal to the first address
				// in the table which points to it
	int depth;	// how many hash value bits are being used by this bucket
	bool full;	// does this bucket contain a key
	int64 key;	// the key stored in this bucket
} Bucket;

// helper structure to store statistics gathered
typedef struct stats {
	int nbuckets;	// how many distinct buckets does the table point to
	int nkeys;		// how many keys are being stored in the table
	int time;		// how much CPU time has been used to insert/lookup keys
					// in this table
} Stats;

// an inner table is an extendible hash table with an array of slots pointing 
// to buckets holding up to 1 key, along with some information about the number 
// of hash value bits to use for addressing
typedef struct inner_table {
	Bucket **buckets;	// array of pointers to buckets
	int size;			// how many entries in the table of pointers (2^depth)
	int depth;			// how many bits of the hash value to use (log2(size))
	int nkeys;			// how many keys are being stored in the table
    Stats stats;		// collection of statistics about this hash table
} InnerTable;

// a xuckoo hash table is just two inner tables for storing inserted keys
struct xuckoo_table {
	InnerTable *table1;
	InnerTable *table2;
};


/* 
 * Helper Functions
 * */

/* Initialises values for the new bucket */
static void init_bucket(Bucket *bucket, int first_address, int depth) {
    /* Make sure this is your memory */
	assert(bucket);

	bucket->id = first_address;
	bucket->depth = depth;
	bucket->full = false;

}

/* Initialises an InnerTable */
static void init_xuck_table(InnerTable *table) {
    /* Don't touch memory you didn't ask for! */
    assert(table);

    /* Initialise values and create bucket space */
    table->size = 1;
    table->buckets = malloc(sizeof(*(table->buckets)));
    assert(table->buckets);
    table->buckets[0] = malloc(sizeof(table->buckets[0]));
    init_bucket(table->buckets[0], 0, 0);
    table->depth = 0;
    table->nkeys = 0;

    /* Initialise Stats Info */
	table->stats.nbuckets = 1;
	table->stats.nkeys = 0;
	table->stats.time = 0;
}


/* 
 * Real Functions
 */
// initialise an extendible cuckoo hash table
XuckooHashTable *new_xuckoo_hash_table() {
    /* Ask for mem for table and make sure you have it */
    XuckooHashTable *table = malloc(sizeof(*table));
    assert(table);

    /* Create and initialise both inner tables */
    InnerTable *table1 = malloc(sizeof(*table1));
    init_xuck_table(table1);
    table->table1 = table1;
    InnerTable *table2 = malloc(sizeof(*table2));
    init_xuck_table(table2);
    table->table2 = table2;

    return table;
}


// free all memory associated with 'table'
void free_xuckoo_hash_table(XuckooHashTable *table) {
	assert(table);
    
    /* Free Inner1 */
	// loop backwards through the array of pointers, freeing buckets only as we
	// reach their first reference
	// (if we loop through forwards, we wouldn't know which reference was last)
	int i;
    for (i = table->table1->size-1; i >= 0; i--) {
        if (table->table1->buckets[i]->id == i) {
            free(table->table1->buckets[i]);
        }
	}

	// free the array of bucket pointers
	free(table->table1->buckets);
    free(table->table1);
    /* Free Inner2 */
    for (i = table->table2->size-1; i >= 0; i--) {
        if (table->table2->buckets[i]->id == i) {
            free(table->table2->buckets[i]);
        }
	}

	// free the array of bucket pointers
	free(table->table2->buckets);
    free(table->table2);

	// free the table struct itself
	free(table);
}


// insert 'key' into 'table', if it's not in there already
// returns true if insertion succeeds, false if it was already in there
bool xuckoo_hash_table_insert(XuckooHashTable *table, int64 key) {
	assert(table);
	int start_time = clock(); // start timing
    InnerTable *ftable = table->table1;
	int hash = h1(key);
    int hashnum = 1;

    // Decide on table and hash algorithm
    if(table->table2->size < table->table1->size) {
        ftable = table->table2;
        hash = h2(key);
        hashnum = 2;
    }
    
    
	// calculate table address
	int address = rightmostnbits(table->depth, hash);

	// is this key already there?
	if (xuckoo_hash_table_lookup(table, key)) {
		table->stats.time += clock() - start_time; // add time elapsed
		return false;
	}

    /* If not, cuckoo insert the key until a space is found, resize the tables
    * if necessary */
	// if not, make space in the table until our target bucket has space
	while (table->buckets[address]->full) {
		split_bucket(table, address);

		// and recalculate address because we might now need more bits
		address = rightmostnbits(table->depth, hash);
	}

	// there's now space! we can insert this key
	table->buckets[address]->key = key;
	table->buckets[address]->full = true;
	table->stats.nkeys++;

	// add time elapsed to total CPU time before returning
	table->stats.time += clock() - start_time;
	return true;
}


// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool xuckoo_hash_table_lookup(XuckooHashTable *table, int64 key) {
	assert(table);
	int start_time = clock(); // start timing

	// calculate table address for this key
	int address = rightmostnbits(table->table1->depth, h1(key));

	// look for the key in table1 in that bucket (unless it's empty)
	bool found = false;
	if (table->table1->buckets[address]->full) {
		// found it? Only search table2 if the  h1 key has somehting in it.
		if(table->table1->buckets[address]->key == key) {
            found = true;
        } else {
	        address = rightmostnbits(table->table2->depth, h2(key));
		    if(table->table2->buckets[address] && 
                            table->table2->buckets[address]->key == key) {
                found = true;
            }
        }
	}

	// add time elapsed to total CPU time before returning result
	table->table1->stats.time += clock() - start_time;
	return found;
}


// print the contents of 'table' to stdout
void xuckoo_hash_table_print(XuckooHashTable *table) {
	assert(table != NULL);

	printf("--- table ---\n");

	// loop through the two tables, printing them
	InnerTable *innertables[2] = {table->table1, table->table2};
	int t;
	for (t = 0; t < 2; t++) {
		// print header
		printf("table %d\n", t+1);

		printf("  table:               buckets:\n");
		printf("  address | bucketid   bucketid [key]\n");
		
		// print table and buckets
		int i;
		for (i = 0; i < innertables[t]->size; i++) {
			// table entry
			printf("%9d | %-9d ", i, innertables[t]->buckets[i]->id);

			// if this is the first address at which a bucket occurs, print it
			if (innertables[t]->buckets[i]->id == i) {
				printf("%9d ", innertables[t]->buckets[i]->id);
				if (innertables[t]->buckets[i]->full) {
					printf("[%llu]", innertables[t]->buckets[i]->key);
				} else {
					printf("[ ]");
				}
			}

			// end the line
			printf("\n");
		}
	}
	printf("--- end table ---\n");
}


// print some statistics about 'table' to stdout
void xuckoo_hash_table_stats(XuckooHashTable *table) {
	fprintf(stderr, "not yet implemented\n");
	return;
}
