#include <stdlib.h>
#include <string.h>
#include <assert.h>

void*
_alloc( size_t size )
{
   void* ptr = malloc( size );
   assert( !size || ptr );
   return ptr;
}

void*
_allocz( size_t size )
{
   void* ptr = _alloc( size );
   memset( ptr, 0, size );
   return ptr;
}

void
_free( void* ptr )
{
   if( ptr )
      free( ptr );
}

int
locate_rank( unsigned n_elems,
             int n_ranks,
             unsigned idx )
{
   unsigned upp = n_elems/n_ranks, rem = n_elems%n_ranks;
   assert( idx < n_elems );
   if( idx < rem*(upp + 1) )
      return idx/(upp + 1);
   else
      return rem + (idx - rem*(upp + 1))/upp;
}

unsigned
local_size( unsigned n_elems,
            int n_ranks,
            int rank )
{
   return n_elems/n_ranks + ((rank < (n_elems%n_ranks)) ? 1 : 0);
}

void
make_displs( unsigned size,
             unsigned const* cnts,
             unsigned* displs )
{
   if( size )
   {
      unsigned ii;

      displs[0] = 0;
      for( ii = 1; ii < size; ++ii )
         displs[ii] = displs[ii - 1] + cnts[ii - 1];
   }
}

void
make_displs2( unsigned size,
              unsigned const* cnts,
              unsigned* displs )
{
   if( size )
   {
      unsigned ii;

      displs[0] = 0;
      for( ii = 1; ii <= size; ++ii )
         displs[ii] = displs[ii - 1] + cnts[ii - 1];
   }
}

void
make_displs_inplace( unsigned size,
                     unsigned* displs )
{
   if( size )
   {
      unsigned prev = displs[0], tmp, ii;

      displs[0] = 0;
      for( ii = 1; ii <= size; ++ii )
      {
         tmp = displs[ii];
         displs[ii] = displs[ii - 1] + prev;
         prev = tmp;
      }
   }
}

void
make_counts( unsigned size,
             unsigned const* displs,
             unsigned* cnts )
{
   if( size )
   {
      unsigned ii;

      for( ii = 0; ii < size; ++ii )
         cnts[ii] = displs[ii + 1] - displs[ii];
   }
}
