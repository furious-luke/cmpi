/*
** @author Luke Hodkinson, 2014
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "hash.h"

unsigned
_hash_max_size( unsigned size );

hash_t*
hash_new( unsigned size )
{
   hash_t* obj = (hash_t*)malloc( sizeof(hash_t) );
   unsigned ii;
   obj->size = size;
   obj->max_size = _hash_max_size( size );
   obj->tbl = (hash_node_t**)malloc( obj->max_size*sizeof(hash_node_t*) );
   memset( obj->tbl, 0, obj->max_size*sizeof(hash_node_t*) );
#ifdef HASH_STATS
   obj->n_cols = 0;
   obj->longest_chain = 0;
#endif
   return obj;
}

void
hash_delete( hash_t* obj )
{
   assert( obj );
   if( obj->tbl )
   {
      unsigned ii;
      for( ii = 0; ii < obj->max_size; ++ii )
      {
         hash_node_t* node = obj->tbl[ii];
         while( node )
         {
            hash_node_t* next = node->next;
            free( node );
            node = next;
         }
      }
      free( obj->tbl );
   }
   free( obj );
}

void
hash_insert( hash_t* obj,
             HASH_KEY key,
             HASH_VAL val )
{
   unsigned idx = key%obj->max_size;
   hash_node_t** np = obj->tbl + idx;
   unsigned lc = 0;
#ifdef HASH_STATS
   if( *np )
      ++obj->n_cols;
#endif
   while( *np && (*np)->key != key )
   {
      np = &(*np)->next;
#ifdef HASH_STATS
      ++lc;
#endif
   }
#ifdef HASH_STATS
   if( lc > obj->longest_chain )
      obj->longest_chain = lc;
#endif
   if( !*np )
   {
      *np = (hash_node_t*)malloc( sizeof(hash_node_t) );
      (*np)->key = key;
      (*np)->next = NULL;
   }
   (*np)->val = val;
}

HASH_VAL
hash_lookup( hash_t const* obj,
             HASH_KEY key )
{
   unsigned idx = key%obj->max_size;
   hash_node_t* np = obj->tbl[idx];
   while( np && np->key != key )
      np = np->next;
   if( np )
      return np->val;
   return HASH_INVALID;
}

unsigned
_sieve_of_eratosthenes( unsigned N,
                        unsigned max_primes,
                        unsigned* primes )
{
   int* array = (int*)malloc( N*sizeof(int) );
   unsigned sqrt_N = (unsigned)sqrt( (float)N );
   unsigned ii, jj;
   memset( array, 0, N*sizeof(int) );
   for( ii = 2; ii <= sqrt_N; ++ii )
   {
      if( array[ii - 1] == 0 )
      {
         for( jj = ii*ii; jj <= N; jj += ii )
            array[jj - 1] = 1;
      }
   }
   for( ii = 0, jj = 0; ii < N && jj < max_primes; ++ii )
   {
      if( array[ii] == 0 )
         primes[jj++] = ii + 1;
   }
   free( array );
   return jj;
}

unsigned
_hash_max_size( unsigned size )
{
   static float const factor = 1.3;
   unsigned max_size = factor*(float)size;
   unsigned n_primes, max_primes = 10000;
   unsigned* primes;
   unsigned ii;

   if( max_size <= size )
      max_size *= 2;
   if( max_size%2 == 0 )
      ++max_size;

   primes = (unsigned*)malloc( max_primes*sizeof(unsigned) );
   n_primes = _sieve_of_eratosthenes( 100000, max_primes, primes );
   do
   {
      for( ii = 2; ii < n_primes; ++ii )
      {
         if( max_size%primes[ii] == 0 )
            max_size += 2;
      }
   }
   while( ii < n_primes );
   free( primes );

   return max_size;
}
