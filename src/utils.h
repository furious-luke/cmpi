#ifndef utils_h
#define utils_h

#include <assert.h>

#define ALLOC( type, size )                     \
   (type*)_alloc( sizeof(type)*(size) )

#define ALLOCZ( type, size )                    \
   (type*)_allocz( sizeof(type)*(size) )

#define FREE( ptr )                             \
   _free( ptr )

void*
_alloc( size_t size );

void*
_allocz( size_t size );

void
_free( void* ptr );

#define MIN( x, y ) (((x) < (y)) ? (x) : (y))
#define MAX( x, y ) (((x) > (y)) ? (x) : (y))

int
locate_rank( unsigned n_elems,
             int n_ranks,
             unsigned idx );

unsigned
local_size( unsigned n_elems,
            int n_ranks,
            int rank );

void
make_displs( unsigned size,
             unsigned const* cnts,
             unsigned* displs );

void
make_displs2( unsigned size,
              unsigned const* cnts,
              unsigned* displs );

void
make_displs_inplace( unsigned size,
                     unsigned* displs );

void
make_counts( unsigned size,
             unsigned const* displs,
             unsigned* cnts );

#ifndef NDEBUG

#define MPI_OK( func )                          \
   assert( func == MPI_SUCCESS )

#else

#define MPI_OK( func )                          \
   func

#endif

#endif
