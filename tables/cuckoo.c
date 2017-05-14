/* * * * * * * * *
 * Dynamic hash table using cuckoo hashing, resolving collisions by switching
 * keys between two tables with two separate hash functions
 *
 * created for COMP20007 Design of Algorithms - Assignment 2, 2017
 * by Matt F

 * Modified and completed by:
 * Author: 	Luke Hedt
 * StuID:	832153
 * Date:	11/05/2017
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "cuckoo.h"

/* Removing the Magic Numbers */
#define DOUBSIZE 2
/* Arbitrary size to call a 'cycle' */
#define MAXDEP 35

/* Holds stats and info for stat calculations  */
typedef struct stats {
    int time;       // Keeps a track of the time taken to run the commands.
    int collisions; // Keeps a track of how many keys collide on their first
                    // insert.
    int probes;     // Keeps a track of the total number of kicked items
} Stats;

// an inner table represents one of the two internal tables for a cuckoo
// hash table. it stores two parallel arrays: 'slots' for storing keys and
// 'inuse' for marking which entries are occupied
typedef struct inner_table {
	int64 *slots;	// array of slots holding keys
	bool  *inuse;	// is this slot in use or not?
    int filled;     // Keeps a count of the number of filled slots.
} InnerTable;

// a cuckoo hash table stores its keys in two inner tables
struct cuckoo_table {
	InnerTable *table1; // first table
	InnerTable *table2; // second table
	int size;			// size of each table
    Stats stat;         // holds stats for the stats function.
};


 /*
  * Helper Functions - Based on supplied code in linear.c
  *
  *	Set up the internals of an inner table struct with new
  * arrays of size 'size'
  */
 static void initialise_inner_table(InnerTable *i_table, int size) {
	/* Each single table can't be bigger than the max table size */
	assert(size < MAX_TABLE_SIZE && "error: table has grown too large!");

    /* Create slots table */
	i_table->slots = malloc((sizeof(*i_table->slots)) * size);
 	assert(i_table->slots);
    /* Creates an inuse table */
 	i_table->inuse = malloc((sizeof(*i_table->inuse)) * size);
 	assert(i_table->inuse);
    /* Creates the stats struct */

    /* Set up the inuse table as false */
 	int i;
 	for (i = 0; i < size; i++) {
 		i_table->inuse[i] = false;
 	}
    /* Stats setup */
    i_table->filled = 0;
}

 /*
  *	Sets up the cuckoo table with inner arrays of size 'size'
  */
 static void initialise_cuck_table(CuckooHashTable *o_table, int size) {

	InnerTable *inner1 = malloc(sizeof(*inner1));
	assert(inner1);

 	initialise_inner_table(inner1, size);

	o_table->table1 = inner1;

	InnerTable *inner2 = malloc(sizeof(*inner2));
	assert(inner2);

 	initialise_inner_table(inner2, size);
	o_table->table2 = inner2;

 	o_table->size = size;
    o_table->stat.time = 0;
 }

/* Frees an inner table */
static void free_inner(InnerTable *i_table) {
    assert(i_table != NULL);

    /*  Free the Innards    */
    free(i_table->slots);
    free(i_table->inuse);

    /* Free the Table */
    free(i_table);
}



/* Rehashes the given cuckoo table. Based on code in linear.c */
static void rehash_table(CuckooHashTable *o_table) {
    /* Check you're operating on a real set of tables. */
    assert(o_table != NULL);
    assert(o_table->table1 != NULL);
    assert(o_table->table2 != NULL);

    /* Make stuff easy to reference and less confusing. */
	InnerTable *old_in1 = o_table->table1;
	InnerTable *old_in2 = o_table->table2;

    int old_size = o_table->size;

    /* Double the size of the hash table  */
    initialise_cuck_table(o_table, old_size * DOUBSIZE);

    /* Insert items from the smaller tables */
    int i;
    for(i=0; i<old_size; i++) {
        if(old_in1->inuse[i] == true) {
            cuckoo_hash_table_insert(o_table, old_in1->slots[i]);
        }
        if(old_in2->inuse[i] == true) {
            cuckoo_hash_table_insert(o_table, old_in2->slots[i]);
        }
    }

    free_inner(old_in1);
    free_inner(old_in2);
}


/* Real Functions */

// initialise a cuckoo hash table with 'size' slots in each table
CuckooHashTable *new_cuckoo_hash_table(int size) {

	CuckooHashTable *o_table = malloc(sizeof *o_table);
	assert(o_table);

	// set up the internals of the table struct with arrays of size 'size'
	initialise_cuck_table(o_table, size);
    o_table->stat.collisions = 0;

	return o_table;
}


// free all memory associated with 'table'
void free_cuckoo_hash_table(CuckooHashTable *table) {
    assert(table != NULL);

    /* Free the inner tables, then free the main table */
    free_inner(table->table1);
    free_inner(table->table2);

    free(table);
}


// insert 'key' into 'table', if it's not in there already
// returns true if insertion succeeds, false if it was already in there
bool cuckoo_hash_table_insert(CuckooHashTable *table, int64 key) {
    /* Don't operate on a non-existent table */
    assert(table);
    int start_time = clock(); // start timing

    /* Don't try to rehash the same item! */
    if(cuckoo_hash_table_lookup(table, key)) {
        return false;
    }

    /* Only define the variables you need after you know you need them */
    int chainlen = 0;
    bool flg_insrt = true;
    bool flg_first = true;
    int hashnum = 1;
    int hash;
    int64 oldkey = -1;
    InnerTable *cur_table;

    /* Repeat for as long as there are cucks to kick */
    while(flg_insrt) {
        /* Rehashes table after MAXDEP cucks */
        if(chainlen > MAXDEP) {
            rehash_table(table);
            chainlen = 0;
        }

    /* Choose which hash table and hash function to use. */
        if(hashnum == 1) {
            hashnum = 2;
            hash = h1(key) % table->size;
            cur_table = table->table1;
        } else {
            hashnum = 1;
            hash = h2(key) % table->size;
            cur_table = table->table2;
        }

        /* Check for cucks, breaks the loop if there are none */
        if(cur_table->inuse[hash]) {
            if(flg_first) {
                table->stat.collisions += 1;
                flg_first = false;
            }
            oldkey = cur_table->slots[hash];
            chainlen += 1;
            cur_table->filled -= 1;
            table->stat.probes += 1;
        } else {
            flg_insrt = false;
        }

        /* Insert the key and set up the cucked key if necessary. */
        cur_table->inuse[hash] = true;
        cur_table->slots[hash] = key;
        cur_table->filled += 1;
        key = oldkey;
    }
    /* Success! */
    // add time elapsed to total CPU time before returning
	table->stat.time += clock() - start_time;
    return true;
}

// lookup whether 'key' is inside 'table'
// returns true if found, false if not
bool cuckoo_hash_table_lookup(CuckooHashTable *table, int64 key) {
    assert(table);
    int start_time = clock(); // start timing

    /* Make for easy referencing */
    int hash1 = h1(key) % table->size;
    int hash2 = h2(key) % table->size;

    /* Check the data's not garbage before checking if your key is there. */
    if((table->table1->inuse[hash1] == true) && (table->table1->slots[hash1]==key)) {
        // add time elapsed to total CPU time before returning
    	table->stat.time += clock() - start_time;
        return true;
    }

    if((table->table2->inuse[hash2] == true) && (table->table2->slots[hash2]==key)) {
        // add time elapsed to total CPU time before returning
    	table->stat.time += clock() - start_time;
        return true;
    }

    /* If it hasn't been found it's not in here. */
    // add time elapsed to total CPU time before returning
	table->stat.time += clock() - start_time;
    return false;
}


// print the contents of 'table' to stdout
void cuckoo_hash_table_print(CuckooHashTable *table) {
	assert(table);
	printf("--- table size: %d\n", table->size);

	// print header
	printf("                    table one         table two\n");
	printf("                  key | address     address | key\n");

	// print rows of each table
	int i;
	for (i = 0; i < table->size; i++) {

		// table 1 key
		if (table->table1->inuse[i]) {
			printf(" %20llu ", table->table1->slots[i]);
		} else {
			printf(" %20s ", "-");
		}

		// addresses
		printf("| %-9d %9d |", i, i);

		// table 2 key
		if (table->table2->inuse[i]) {
			printf(" %llu\n", table->table2->slots[i]);
		} else {
			printf(" %s\n",  "-");
		}
	}

	// done!
	printf("--- end table ---\n");
}


// print some statistics about 'table' to stdout
void cuckoo_hash_table_stats(CuckooHashTable *table) {
	assert(table != NULL);
	printf("--- table stats ---\n");

	// print some information about the table
	printf("Current size: %d slots\n", table->size);
	printf("Filled slots: %d slots\n",
            table->table1->filled + table->table2->filled);
	printf("Load Percentage in t1: %.2f% \n", table->table1->filled
                / (float)table->size * 100);
	printf("Load Percentage in t2: %.2f% \n", table->table2->filled
                / (float)table->size * 100);

    printf("Number of collisions: %d \n", table->stat.collisions);
    printf("Averge probe length: %.2f \n",
                    (float)table->stat.probes/table->stat.collisions);

	// also calculate CPU usage in seconds and print this
	float seconds = table->stat.time * 1.0 / CLOCKS_PER_SEC;
	printf("CPU time spent: %.6f sec\n", seconds);

	printf("--- end stats ---\n");
}
