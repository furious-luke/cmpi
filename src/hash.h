/*
** Simple chained hashmap implementation.
**
** Designed to provide performance and simplicity for
** integer based hashing. The integer types used for keys
** and values are tunable, but just remember to update
** the HASH_INVALID definition to something appropriate.
**
** @author Luke Hodkinson, 2014
*/

#ifndef hash_h
#define hash_h

#include <limits.h>

#define HASH_KEY     unsigned
#define HASH_VAL     unsigned
#define HASH_INVALID UINT_MAX

typedef struct hash_node hash_node_t;
typedef struct hash      hash_t;

/*
** Create a new hashmap.
**
** @returns A hashmap allocated on the heap.
*/
hash_t*
hash_new();

/*
** Delete an existing hashmap.
**
** @param[in] obj  Pointer to an existing hashmap.
*/
void
hash_delete( hash_t* obj );

/*
** Insert an entry into a hashmap.
**
** @param[in] obj  An existing hashmap object.
** @param[in] key  The key of the new entry.
** @param[in] val  The value of the new entry.
*/
void
hash_insert( hash_t* obj,
             HASH_KEY key,
             HASH_VAL val );

/*
** Lookup a key in a hashmap.
**
** @param[in] obj  An existing hashmap object.
** @param[in] key  The key to lookukp.
** @returns Either the value associated with the key, or the
**          definition of HASH_INVALID.
*/
HASH_VAL
hash_lookup( hash_t const* obj,
             HASH_KEY key );

struct hash_node
{
   HASH_KEY     key;
   HASH_VAL     val;
   hash_node_t* next;
};

struct hash
{
   unsigned      size;
   unsigned      max_size;
   hash_node_t** tbl;
#ifdef HASH_STATS
   unsigned      n_cols;
   unsigned      longest_chain;
#endif
};

#endif
