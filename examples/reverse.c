#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include "../utils.h"

int
main( int argc,
      char** argv )
{
   unsigned n_chunks, *chunks;
   unsigned n_files = 8, *n_file_elems;
   unsigned n_elems = 0, *idxs, *data;
   unsigned n_local_elems, elem_offs;
   char fn[1000];
   int rank, n_ranks;
   FILE* file;
   int ii, jj, kk;

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
      n_elems += n_file_elems[ii];
   }
   chunk_files( n_files, n_file_elems, &n_chunks, &chunks, MPI_COMM_WORLD );
   FREE( n_file_elems );

   n_local_elems = local_size( n_elems, n_ranks, rank );
   MPI_OK( MPI_Scan( &n_local_elems, &elem_offs, 1, MPI_UNSIGNED, MPI_SUM, MPI_COMM_WORLD ) );
   elem_offs -= n_local_elems;

   data = ALLOC( unsigned, n_local_elems );
   for( ii = 0, kk = 0; ii < n_chunks; ++ii )
   {
      sprintf( fn, "test_file.%05d", chunks[3*ii + 0] );
      file = fopen( fn, "r" );
      assert( file );
      for( jj = 0; jj < chunks[3*ii + 1] + 1; ++jj ) /* +1 for header */
      {
         unsigned dummy;
         fscanf( file, "%d", &dummy );
      }
      for( jj = 0; jj < chunks[3*ii + 2]; ++jj, ++kk )
      {
         assert( kk < n_local_elems );
         fscanf( file, "%d", data + kk );
      }
   }

   idxs = ALLOC( unsigned, n_local_elems );
   for( ii = 0; ii < n_local_elems; ++ii )
      idxs[ii] = (elem_offs + ii + 10)%n_elems;

   permute( n_elems, n_local_elems, idxs, data, MPI_UNSIGNED, MPI_COMM_WORLD );

   {
      unsigned *all_sizes, *all_data, *all_displs;
      all_sizes = ALLOC( unsigned, n_ranks );
      all_displs = ALLOC( unsigned, n_ranks );
      all_data = ALLOC( unsigned, n_elems );
      MPI_OK( MPI_Gather( &n_local_elems, 1, MPI_UNSIGNED, all_sizes, 1, MPI_UNSIGNED, 0, MPI_COMM_WORLD ) );
      make_displs( n_ranks, all_sizes, all_displs );
      MPI_OK( MPI_Gatherv( data, n_local_elems, MPI_UNSIGNED, all_data, all_sizes, all_displs, MPI_UNSIGNED, 0, MPI_COMM_WORLD ) );
      if( rank == 0 )
      {
         for( ii = 0; ii < n_elems; ++ii )
            printf( "%d ", all_data[ii] );
         printf( "\n" );
      }
   }

   FREE( chunks );
   MPI_Finalize();
   return EXIT_SUCCESS;
}
