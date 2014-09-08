#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "../utils.h"

int
main( int argc,
      char** argv )
{
   unsigned n_chunks, *chunks;
   unsigned n_files = 10, *n_file_elems;
   char fn[1000];
   int rank, n_ranks;
   FILE* file;
   int ii;

   MPI_Init( &argc, &argv );
   MPI_Comm_size( MPI_COMM_WORLD, &n_ranks );
   MPI_Comm_rank( MPI_COMM_WORLD, &rank );

   n_file_elems = ALLOC( unsigned, n_files );
   for( ii = 0; ii < n_files; ++ii )
   {
      sprintf( fn, "test_file.%05d", ii );
      file = fopen( fn, "r" );
      assert( file );
      fscanf( file, "%d", n_file_elems + ii );
   }
   chunk_files( n_files, n_file_elems, &n_chunks, &chunks, MPI_COMM_WORLD );
   FREE( n_file_elems );

   if( rank == 0 )
   {
      printf( "n_chunks: %d\n", n_chunks );
   }

   FREE( chunks );
   MPI_Finalize();
   return EXIT_SUCCESS;
}
