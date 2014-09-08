#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "hash.h"

static unsigned const norm_size = 100000;
static unsigned const big_size = 10000000;

void
dump_stats( hash_t* hash )
{
#ifdef HASH_STATS
   printf( "Number of collisions = %d\n", hash->n_cols );
   printf( "Longest chain = %d\n", hash->longest_chain );
#endif
}

void
test_normal( hash_t* hash )
{
   unsigned ii;
   printf( "Testing normal operation.\n" );
   for( ii = 0; ii < 2*norm_size; ++ii )
      hash_insert( hash, 100*ii, ii );
   for( ii = 0; ii < norm_size; ++ii )
   {
      if( hash_lookup( hash, 100*ii ) != ii )
         printf( "Failed lookup = %d\n", 100*ii );
   }
   dump_stats( hash );
}

void
test_duplicates( hash_t* hash )
{
   unsigned ii;
   printf( "Testing duplicates.\n" );
   for( ii = 0; ii < norm_size; ++ii )
      hash_insert( hash, 100*ii, ii );
}

void
test_big( hash_t* hash )
{
   unsigned ii;
   printf( "Testing big.\n" );
   srand( time( NULL ) );
   for( ii = 0; ii < big_size; ++ii )
   {
      unsigned k = ((float)rand()/(float)RAND_MAX)*(UINT_MAX - 2);
      hash_insert( hash, k, ii );
   }
   for( ii = 0; ii < big_size; ++ii )
   {
      unsigned k = ((float)rand()/(float)RAND_MAX)*(UINT_MAX - 2);
      hash_lookup( hash, k );
   }
   dump_stats( hash );
}

int
main( int argc,
      char* argv[] )
{
   hash_t* hash;
   hash = hash_new( norm_size );
   test_normal( hash );
   test_duplicates( hash );
   hash_delete( hash );
   hash = hash_new( big_size );
   test_big( hash );
   hash_delete( hash );
   return EXIT_SUCCESS;
}
