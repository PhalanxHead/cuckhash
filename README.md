# Assignment 2 for Design of Algorithms 2017 S1

## This Assignment was in 5 parts, outlined in specification.pdf

### An Excerpt is shown below:

###Coding Tasks

#### Part 1: Cuckoo Hashing (4 marks)

Complete `tables/cuckoo.c` with an implementation of a hash table that uses cuckoo hashing with two tables. An initial struct cuckoo table is provided for you, but you may modify this as you see fit. You may find the code in `tables/linear.c` helpful. You will need to complete each of the functions described in tables/cuckoo.h:

- `new_cuckoo_hash_table(size)`: Create a hash table with two tables, each with space (initially) for size keys (so, 2× size keys in total). Return its pointer.
- `cuckoo_hash_table_insert(table, key)`: Insert key into table, if it is not there already. You should always begin by inserting key into the first of the two tables, using `h1()` from `inthash.h`. Upon a collision, the preexisting key should be moved to the second table using `h2()`. Upon collisions in the second table, the preexisting key should be moved back to the first table using `h1()` again, and so forth. Returns `true` if key was inserted, `false` if it was already in table.
- `cuckoo_hash_table_lookup(table, key)`: Returns `true` if key is in table, `false` otherwise.
- `cuckoo_hash_table_print(table)`: Implementation provided. If you modify the provided structs, you must ensure the output format is unchanged (it will be used to test your program).
- `cuckoo_hash_table_stats(table)`: Print any data you have gathered about hash table use. The output format is up to you.
- `free_cuckoo_hash_table(table)`: Free all memory allocated to table.

Your hash table must be capable of storing an arbitrary number of keys. Therefore, you must enable it to grow in size as more keys are added. You are free to decide on when and how to grow your hash table, for example you might like to double the size of each table whenever an insertion is about to fail because of a cycle, or whenever you encounter a very long chain of cuckoos.

Warning: a small bug could cause your hash table to continually grow in size, consuming a lot of
memory. Also, the hash functions inside `inthash.c` only work for tables of bounded size. Whenever
you increase the size of your table, you should use assert to ensure that your hash table is not larger
than `inthash.h`’s `MAX_TABLE_SIZE` (see `tables/linear.c` for an example).

#### Part 2: Multi-key Extendible Hashing (4 mark)

The code in `tables/xtndbl1.c` provides a working implementation of extendible hashing where each bucket can store at most 1 key. Complete `tables/xtndbln.c` by adapting this code to provide an extendible hash table where each bucket can store up to `bucketsize` keys, where `bucketsize` is the parameter passed to the new `xtndbln_hash_table()` function. Once again, initial structs are provided for you, but you may modify them as you see fit. You will need to complete each of the functions described in `tables/xtndbln.h`:

Note: in the context of this part, command-line parameter starting size takes on a new meaning as the fixed size of each bucket, rather than the initial size of the internal table. The table of bucket pointers should grow like a regular extendible hash table.

- `new_xtndbln_hash_table(bucketsize)`: Create an empty extendible hash table with one slots, initially pointing to an empty bucket. The buckets in this hash table will contain up to `bucketsize` keys. Return the table’s pointer.
- `xtndbln_hash_table_insert(table, key)`: Insert `key` into `table`, if it is not there already. To insert a key, take the right-most depth bits from the result of `inthash.h`’s `h1()` function, where depth is $log_2(current table size)$. The resulting value should be used as an index into the table of bucket pointers, and the key should be inserted in the corresponding bucket. Returns `true` if `key` was inserted, `false` if it was already in table.
- `xtndbln_hash_table_lookup(table, key)`: Returns true if key is in table, false otherwise.
- `xtndbln_hash_table_print(table)`: Implementation provided. If you modify the provided structs, you must ensure the output format is unchanged (it will be used to test your program).
- `xtndbln_hash_table_stats(table)`: Print any data you have gathered about hash table use, for use gathering data for your report. The format of this function’s output is up to you.
- `free_xtndbln_hash_table(table)`: Free all memory allocated to table.

Your hash table must be capable of storing an arbitrary number of keys. Therefore, you must enable it to grow in size as more keys are added. You should grow your hash table by splitting buckets when a key is to be inserted into a bucket that is already full. When splitting a bucket, the table may have to be doubled in size.

Warning: the same maximum table size warning from part 1 applies.

#### Part 3: Combining Cuckoo Hashing and Extendible Hashing (4 marks)

Complete `tables/xuckoo.c` with an implementation of a hash table that uses a combination of cuckoo hashing and extendible hashing, where each extendible hashing bucket store up to one key. Once again, initial structs are provided, and you can base your implementation off of `tables/xtndbl1.c`. You will need to complete each of the functions described in `tables/xuckoo.h`:

Note: in the context of this part, command-line parameter starting size is ignored.

- `new_xuckoo_hash_table(size)`: Create an empty extendible hash table with two tables, each with one slot pointing to an empty bucket (separate buckets for each table). The buckets in this hash table will contain up to one key. Return the table’s pointer.
- `xuckoo_hash_table_insert(table, key)`: Insert key into table, if it is not there already. Use `h1()` when inserting into table’s first table, and `h2()` when inserting into its second table. You should always begin by attempting to insert key into the table with fewer keys, or table’s first table if both have the same number of keys. When a key is hashed to a full bucket in the first table, before splitting the bucket, the pre-existing key should be replaced with the new key, and the replaced key should be inserted into the second table. Likewise, if a key is hashed to a full bucket in the second table, the pre-existing key should be replaced and inserted into the first table, and so forth. Returns true if key was inserted, false if it was already in table.
- `xuckoo_hash_table_lookup(table, key)`: Returns true if key is in table, false otherwise.
- `xuckoo_hash_table_print(table)`: Implementation provided. If you modify the provided structs, you must ensure the output format is unchanged (it will be used to test your program).
- `xuckoo_hash_table_stats(table)`: Print any data you have gathered about hash table use. The output format is up to you.
- `free_xuckoo_hash_table(table)`: Free all memory allocated to table.

Your hash table must be capable of storing an arbitrary number of keys. Therefore, you must enable it to grow in size as more keys are added. Therefore, you must enable it to grow in size as more keys are added. You are free to decide on when and how to grow your hash table. For example, you might like to stop moving between tables when you encounter a cycle, or after a very long chain of replacements. At this point, the final full bucket can be split like in a normal extendible hash table. Warning: the same maximum table size warning from part 1 applies.