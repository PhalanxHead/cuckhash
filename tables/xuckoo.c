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

/* Maximum length of a chain before rehashing the table. */
#define MAXDEP 34

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


// double the table of bucket pointers, duplicating the bucket pointers in the
// first half into the new second half of the table
static void double_table(InnerTable *table) {
	int size = table->size * 2;
	assert(size < MAX_TABLE_SIZE && "error: table has grown too large!");

	// get a new array of twice as many bucket pointers, and copy pointers down
	table->buckets = realloc(table->buckets, (sizeof *table->buckets) * size);
	assert(table->buckets);
	int i;
	for (i = 0; i < table->size; i++) {
		table->buckets[table->size + i] = table->buckets[i];
	}

	// finally, increase the table size and the depth we are using to hash keys
	table->size = size;
	table->depth++;
}

// reinsert a key into the hash table after splitting a bucket --- we can assume
// that there will definitely be space for this key because it was already
// inside the hash table previously
// use 'xtndbl1_hash_table_insert()' instead for inserting new keys
static void reinsert_key(InnerTable *table, int hashnum, int64 key) {
	int address;
    if(hashnum == 1) {
        address = rightmostnbits(table->depth, h1(key));
    } else {
        address = rightmostnbits(table->depth, h2(key));
    }
	table->buckets[address]->key = key;
	table->buckets[address]->full = true;
}

// split the bucket in 'table' at address 'address', growing table if necessary
static void split_bucket(InnerTable *table, int hashnum, int address) {

	// FIRST,
	// do we need to grow the table?
	if (table->buckets[address]->depth == table->depth) {
		// yep, this bucket is down to its last pointer
		double_table(table);
	}
	// either way, now it's time to split this bucket


	// SECOND,
	// create a new bucket and update both buckets' depth
	Bucket *bucket = table->buckets[address];
	int depth = bucket->depth;
	int first_address = bucket->id;

	int new_depth = depth + 1;
	bucket->depth = new_depth;

	// new bucket's first address will be a 1 bit plus the old first address
	int new_first_address = 1 << depth | first_address;
	Bucket *newbucket = malloc(sizeof(*newbucket));
    init_bucket(newbucket, new_first_address, new_depth);
	table->stats.nbuckets++;

	// THIRD,
	// redirect every second address pointing to this bucket to the new bucket
	// construct addresses by joining a bit 'prefix' and a bit 'suffix'
	// (defined below)

	// suffix: a 1 bit followed by the previous bucket bit address
	int bit_address = rightmostnbits(depth, first_address);
	int suffix = (1 << depth) | bit_address;

	// prefix: all bitstrings of length equal to the difference between the new
	// bucket depth and the table depth
	// use a for loop to enumerate all possible prefixes less than maxprefix:
	int maxprefix = 1 << (table->depth - new_depth);

	int prefix;
	for (prefix = 0; prefix < maxprefix; prefix++) {

		// construct address by joining this prefix and the suffix
		int a = (prefix << new_depth) | suffix;

		// redirect this table entry to point at the new bucket
		table->buckets[a] = newbucket;
	}

	// FINALLY,
	// filter the key from the old bucket into its rightful place in the new
	// table (which may be the old bucket, or may be the new bucket)

	// remove and reinsert the key
	int64 key = bucket->key;
	bucket->full = false;
	reinsert_key(table, hashnum, key);
}

/* Chain-inserts values until it finds a blank spot. Will
 * rehash and resize table if necessary. */
static void xuck_insert(XuckooHashTable *table, int hashnum, int64 key) {
    assert(table);
    int chainlen = 0;
    int hash, nexthash;
    bool flg_insrt = true;
    int64 oldkey;
    InnerTable *ftable;

    /* Loop until flagged, flagged if key inserted */
    while(flg_insrt) {
        /* Decide on Hash function and Inner Table */
        if(hashnum == 1) {
            ftable = table->table1;
            hash = h1(key);
            nexthash = 2;
        } else {
            ftable = table->table2;
            hash = h2(key);
            nexthash = 1;
        }

        // calculate table address
        int address = rightmostnbits(ftable->depth, hash);

        /* Rehashes table after MAXDEP cucks */
        if(chainlen > MAXDEP) {
            split_bucket(ftable, hashnum, address);
            chainlen = 0;
        }

        /* Check for cucks, breaks the loop if there are none */
        if(ftable->buckets[address]->full) {
            /* if(flg_first) { */
            /*     table->stat.collisions += 1; */
            /*     flg_first = false; */
            /* } */
            oldkey = ftable->buckets[address]->key;
            chainlen += 1;
            ftable->nkeys -= 1;
            /* table->stat.probes += 1; */
        } else {
            flg_insrt = false;
        }

        /* Insert the key and set up the cucked key if necessary. */
        ftable->buckets[address]->key = key;
        ftable->buckets[address]->full = true;
        ftable->stats.nkeys++;
        key = oldkey;
        hashnum = nexthash;
    }

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
    int newhash = 2;

    // Decide on table and hash algorithm
    if(table->table2->nkeys < table->table1->nkeys) {
        ftable = table->table2;
        hash = h2(key);
        newhash = 1;
    }

	// calculate table address
	int address = rightmostnbits(ftable->depth, hash);

	// is this key already there?
	if (xuckoo_hash_table_lookup(table, key)) {
		table->table1->stats.time += clock() - start_time; // add time elapsed
		return false;
	}

    /* If not, cuckoo insert the key until a space is found, resize the tables
    * if necessary */
	// if not, make space in the table until our target bucket has space
	if(ftable->buckets[address]->full) {
        int64 oldkey = ftable->buckets[address]->key;
        ftable->buckets[address]->key = key;

        xuck_insert(table, newhash, oldkey);

	} else {
        // there's now space! we can insert this key
        ftable->buckets[address]->key = key;
        ftable->buckets[address]->full = true;
        ftable->stats.nkeys++;
    }


	// add time elapsed to total CPU time before returning
	table->table1->stats.time += clock() - start_time;
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
	assert(table);

	printf("--- table stats ---\n");

	// print some stats about state of the table
	printf("current table1 size: %d\n", table->table1->size);
	printf("    number of keys: %d\n", table->table1->stats.nkeys);
	printf(" number of buckets: %d\n", table->table1->stats.nbuckets);
	printf("current table2 size: %d\n", table->table2->size);
	printf("    number of keys: %d\n", table->table2->stats.nkeys);
	printf(" number of buckets: %d\n", table->table2->stats.nbuckets);

	// also calculate CPU usage in seconds and print this
	float seconds = table->table1->stats.time * 1.0 / CLOCKS_PER_SEC;
	printf("    CPU time spent: %.6f sec\n", seconds);

	printf("--- end stats ---\n");
}
