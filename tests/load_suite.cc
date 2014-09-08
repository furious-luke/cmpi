#include <mpi.h>
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "load.h"
#include "utils.h"

TEST_CASE( "Chunk files" )
{
   int n_ranks, rank;
   MPI_Comm_rank( MPI_COMM_WORLD, &rank );
   MPI_Comm_size( MPI_COMM_WORLD, &n_ranks );

   std::vector<unsigned> n_file_elems( 3 );
   n_file_elems[0] = 3;
   n_file_elems[1] = 7;
   n_file_elems[2] = 5;
   unsigned n_chunks, *chunks;
   chunk_files( 3, n_file_elems.data(), &n_chunks, &chunks, MPI_COMM_WORLD );

   if( n_ranks == 1 )
   {
      REQUIRE( n_chunks == 3 );
      REQUIRE( chunks[3*0 + 0] == 0 );
      REQUIRE( chunks[3*0 + 1] == 0 );
      REQUIRE( chunks[3*0 + 2] == 3 );
      REQUIRE( chunks[3*1 + 0] == 1 );
      REQUIRE( chunks[3*1 + 1] == 0 );
      REQUIRE( chunks[3*1 + 2] == 7 );
      REQUIRE( chunks[3*2 + 0] == 2 );
      REQUIRE( chunks[3*2 + 1] == 0 );
      REQUIRE( chunks[3*2 + 2] == 5 );
   }
   else if( n_ranks == 2 )
   {
      if( rank == 0 )
      {
         REQUIRE( n_chunks == 2 );
         REQUIRE( chunks[3*0 + 0] == 0 );
         REQUIRE( chunks[3*0 + 1] == 0 );
         REQUIRE( chunks[3*0 + 2] == 3 );
         REQUIRE( chunks[3*1 + 0] == 1 );
         REQUIRE( chunks[3*1 + 1] == 0 );
         REQUIRE( chunks[3*1 + 2] == 5 );
      }
      else
      {
         REQUIRE( n_chunks == 2 );
         REQUIRE( chunks[3*0 + 0] == 1 );
         REQUIRE( chunks[3*0 + 1] == 5 );
         REQUIRE( chunks[3*0 + 2] == 2 );
         REQUIRE( chunks[3*1 + 0] == 2 );
         REQUIRE( chunks[3*1 + 1] == 0 );
         REQUIRE( chunks[3*1 + 2] == 5 );
      }
   }
   else if( n_ranks == 3 )
   {
      if( rank == 0 )
      {
         REQUIRE( n_chunks == 2 );
         REQUIRE( chunks[3*0 + 0] == 0 );
         REQUIRE( chunks[3*0 + 1] == 0 );
         REQUIRE( chunks[3*0 + 2] == 3 );
         REQUIRE( chunks[3*1 + 0] == 1 );
         REQUIRE( chunks[3*1 + 1] == 0 );
         REQUIRE( chunks[3*1 + 2] == 2 );
      }
      else if( rank == 1 )
      {
         REQUIRE( n_chunks == 1 );
         REQUIRE( chunks[3*0 + 0] == 1 );
         REQUIRE( chunks[3*0 + 1] == 2 );
         REQUIRE( chunks[3*0 + 2] == 5 );
      }
      else
      {
         REQUIRE( n_chunks == 1 );
         REQUIRE( chunks[3*0 + 0] == 2 );
         REQUIRE( chunks[3*0 + 1] == 0 );
         REQUIRE( chunks[3*0 + 2] == 5 );
      }
   }
   else if( n_ranks == 4 )
   {
      if( rank == 0 )
      {
         REQUIRE( n_chunks == 2 );
         REQUIRE( chunks[3*0 + 0] == 0 );
         REQUIRE( chunks[3*0 + 1] == 0 );
         REQUIRE( chunks[3*0 + 2] == 3 );
         REQUIRE( chunks[3*1 + 0] == 1 );
         REQUIRE( chunks[3*1 + 1] == 0 );
         REQUIRE( chunks[3*1 + 2] == 1 );
      }
      else if( rank == 1 )
      {
         REQUIRE( n_chunks == 1 );
         REQUIRE( chunks[3*0 + 0] == 1 );
         REQUIRE( chunks[3*0 + 1] == 1 );
         REQUIRE( chunks[3*0 + 2] == 4 );
      }
      else if( rank == 2 )
      {
         REQUIRE( n_chunks == 2 );
         REQUIRE( chunks[3*0 + 0] == 1 );
         REQUIRE( chunks[3*0 + 1] == 5 );
         REQUIRE( chunks[3*0 + 2] == 2 );
         REQUIRE( chunks[3*1 + 0] == 2 );
         REQUIRE( chunks[3*1 + 1] == 0 );
         REQUIRE( chunks[3*1 + 2] == 2 );
      }
      else
      {
         REQUIRE( n_chunks == 1 );
         REQUIRE( chunks[3*0 + 0] == 2 );
         REQUIRE( chunks[3*0 + 1] == 2 );
         REQUIRE( chunks[3*0 + 2] == 3 );
      }
   }

   FREE( chunks );
}

int
main( int argc,
      char** argv )
{
   MPI_Init( &argc, &argv );
   int result = Catch::Session().run( argc, argv );
   MPI_Finalize();
   return EXIT_SUCCESS;
}
