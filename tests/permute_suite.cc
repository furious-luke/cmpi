#include <mpi.h>
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "permute.h"

int
locate_rank( unsigned n_elems,
             int n_ranks,
             unsigned idx );

void
make_displs( unsigned size,
             unsigned const* cnts,
             unsigned* displs );

void
count_required( unsigned n_elems,
                unsigned n_idxs,
                unsigned const* idxs,
                int n_ranks,
                unsigned* req_cnts,
                unsigned* req_displs );

void
make_required( unsigned n_elems,
               unsigned n_idxs,
               unsigned const* idxs,
               int n_ranks,
               unsigned* req_idxs,
               unsigned* req_cnts,
               unsigned const* req_displs,
               unsigned* local );

TEST_CASE( "Locate which rank indices exist on" )
{
   REQUIRE( locate_rank( 1, 1, 0 ) == 0 );

   REQUIRE( locate_rank( 2, 1, 0 ) == 0 );
   REQUIRE( locate_rank( 2, 1, 1 ) == 0 );

   REQUIRE( locate_rank( 2, 2, 0 ) == 0 );
   REQUIRE( locate_rank( 2, 2, 1 ) == 1 );

   REQUIRE( locate_rank( 3, 1, 0 ) == 0 );
   REQUIRE( locate_rank( 3, 1, 1 ) == 0 );
   REQUIRE( locate_rank( 3, 1, 2 ) == 0 );

   REQUIRE( locate_rank( 3, 2, 0 ) == 0 );
   REQUIRE( locate_rank( 3, 2, 1 ) == 0 );
   REQUIRE( locate_rank( 3, 2, 2 ) == 1 );

   REQUIRE( locate_rank( 3, 3, 0 ) == 0 );
   REQUIRE( locate_rank( 3, 3, 1 ) == 1 );
   REQUIRE( locate_rank( 3, 3, 2 ) == 2 );

   REQUIRE( locate_rank( 5, 3, 0 ) == 0 );
   REQUIRE( locate_rank( 5, 3, 1 ) == 0 );
   REQUIRE( locate_rank( 5, 3, 2 ) == 1 );
   REQUIRE( locate_rank( 5, 3, 3 ) == 1 );
   REQUIRE( locate_rank( 5, 3, 4 ) == 2 );
}

TEST_CASE( "Make displacements from counts" )
{
   make_displs( 0, NULL, NULL ); // No errors.

   std::vector<unsigned> cnts( 3 ), displs( 3 );
   cnts[0] = 3;
   cnts[1] = 4;
   cnts[2] = 2;
   make_displs( 3, cnts.data(), displs.data() );
   REQUIRE( displs[0] == 0 );
   REQUIRE( displs[1] == 3 );
   REQUIRE( displs[2] == 7 );
}

TEST_CASE( "Count number of elements required from neighbors" )
{
   int n_ranks, rank;
   MPI_Comm_rank( MPI_COMM_WORLD, &rank );
   MPI_Comm_size( MPI_COMM_WORLD, &n_ranks );

   std::vector<unsigned> idxs( 3 ), req_cnts( n_ranks ), req_displs( n_ranks );
   idxs[0] = ((rank == 0) ? (n_ranks - 1) : (rank - 1))*3 + 2;
   idxs[1] = ((rank + 1)%n_ranks)*3;
   idxs[2] = rank*3 + 1;
   count_required( n_ranks*3, 3, idxs.data(), n_ranks, req_cnts.data(), req_displs.data() );
   if( n_ranks == 1 )
   {
      REQUIRE( req_cnts[0] == 3 );
      REQUIRE( req_displs[0] == 0 );
   }
   else if( n_ranks == 2 )
   {
      REQUIRE( req_cnts[rank] == 1 );
      REQUIRE( req_cnts[(rank + 1)%n_ranks] == 2 );
      // REQUIRE( req_displs[rank] == 2 );
      // REQUIRE( req_displs[(rank + 1)%n_ranks] == 1 );
   }
   else
   {
      REQUIRE( req_cnts[((rank == 0) ? (n_ranks - 1) : (rank - 1))] == 1 );
      REQUIRE( req_cnts[((rank + 1)%n_ranks)] == 1 );
      REQUIRE( req_cnts[rank] == 1 );
      if( rank == 0 )
      {
         REQUIRE( req_displs[rank] == 0 );
         REQUIRE( req_displs[rank + 1] == 1 );
         REQUIRE( req_displs[n_ranks - 1] == 2 );
      }
      else if( rank == n_ranks - 1 )
      {
         REQUIRE( req_displs[0] == 0 );
         REQUIRE( req_displs[rank - 1] == 1 );
         REQUIRE( req_displs[rank] == 2 );
      }
      else
      {
         REQUIRE( req_displs[rank - 1] == 0 );
         REQUIRE( req_displs[rank] == 1 );
         REQUIRE( req_displs[rank + 1] == 2 );
      }
   }
}

TEST_CASE( "Make indices of elements required from neighbors" )
{
   int n_ranks, rank;
   MPI_Comm_rank( MPI_COMM_WORLD, &rank );
   MPI_Comm_size( MPI_COMM_WORLD, &n_ranks );

   std::vector<unsigned> idxs( 3 ), req_cnts( n_ranks ), req_displs( n_ranks ), req_idxs, local;
   idxs[0] = ((rank == 0) ? (n_ranks - 1) : (rank - 1))*3 + 2;
   idxs[1] = ((rank + 1)%n_ranks)*3;
   idxs[2] = rank*3 + 1;
   count_required( n_ranks*3, 3, idxs.data(), n_ranks, req_cnts.data(), req_displs.data() );
   req_idxs.resize( req_displs[n_ranks - 1] + req_cnts[n_ranks - 1] );
   local.resize( req_displs[n_ranks - 1] + req_cnts[n_ranks - 1] );
   make_required( n_ranks*3, 3, idxs.data(), n_ranks, req_idxs.data(), req_cnts.data(), req_displs.data(), local.data() );

   if( n_ranks == 1 )
   {
      REQUIRE( req_idxs[0] == 2 );
      REQUIRE( req_idxs[1] == 0 );
      REQUIRE( req_idxs[2] == 1 );
      REQUIRE( local[0] == 0 );
      REQUIRE( local[1] == 1 );
      REQUIRE( local[2] == 2 );
   }
   else if( n_ranks == 2 )
   {
      if( rank == 0 )
      {
         REQUIRE( req_idxs[0] == 1 );
         REQUIRE( req_idxs[1] == 5 );
         REQUIRE( req_idxs[2] == 3 );
         REQUIRE( local[0] == 2 );
         REQUIRE( local[1] == 0 );
         REQUIRE( local[2] == 1 );
      }
      else
      {
         REQUIRE( req_idxs[0] == 2 );
         REQUIRE( req_idxs[1] == 0 );
         REQUIRE( req_idxs[2] == 4 );
         REQUIRE( local[0] == 0 );
         REQUIRE( local[1] == 1 );
         REQUIRE( local[2] == 2 );
      }
   }
}

TEST_CASE( "Permute a distributed array" )
{
   int n_ranks, rank;
   MPI_Comm_rank( MPI_COMM_WORLD, &rank );
   MPI_Comm_size( MPI_COMM_WORLD, &n_ranks );

   std::vector<unsigned> idxs( 3 );
   idxs[0] = ((rank == 0) ? (n_ranks - 1) : (rank - 1))*3 + 2;
   idxs[1] = ((rank + 1)%n_ranks)*3;
   idxs[2] = rank*3 + 1;
   std::vector<int> data( 3 );
   data[0] = rank*3 + 0;
   data[1] = rank*3 + 1;
   data[2] = rank*3 + 2;
   permute( n_ranks*3, 3, idxs.data(), data.data(), MPI_INT, MPI_COMM_WORLD );

   REQUIRE( data[0] == idxs[0] );
   REQUIRE( data[1] == idxs[1] );
   REQUIRE( data[2] == idxs[2] );
}

TEST_CASE( "Scatter a distributed array" )
{
   int n_ranks, rank;
   MPI_Comm_rank( MPI_COMM_WORLD, &rank );
   MPI_Comm_size( MPI_COMM_WORLD, &n_ranks );

   std::vector<unsigned> idxs( 3 ), displs( 4 );
   idxs[0] = ((rank == 0) ? (n_ranks - 1) : (rank - 1))*3 + 2;
   idxs[1] = ((rank + 1)%n_ranks)*3;
   idxs[2] = rank*3 + 1;
   displs[0] = 0;
   displs[1] = 1;
   displs[2] = 3;
   displs[3] = 6;
   std::vector<int> data( 6 );
   data[0] = rank*6 + 0;
   data[1] = rank*6 + 1;
   data[2] = rank*6 + 2;
   data[3] = rank*6 + 3;
   data[4] = rank*6 + 4;
   data[5] = rank*6 + 5;
   int* recv_data;
   unsigned* recv_displs;
   scatterv( n_ranks*3, displs.data(), 3, idxs.data(), data.data(), (void**)&recv_data, &recv_displs, MPI_INT, MPI_COMM_WORLD );

   REQUIRE( recv_displs[0] == 0 );
   REQUIRE( recv_displs[1] == 3 );
   REQUIRE( recv_displs[2] == 4 );
   REQUIRE( recv_displs[3] == 6 );
   REQUIRE( recv_data[0] == idxs[0]*2 - 1 );
   REQUIRE( recv_data[1] == idxs[0]*2 + 0 );
   REQUIRE( recv_data[2] == idxs[0]*2 + 1 );
   REQUIRE( recv_data[3] == idxs[1]*2 );
   REQUIRE( recv_data[4] == idxs[2]*2 - 1 );
   REQUIRE( recv_data[5] == idxs[2]*2 + 0 );

   free( recv_data );
   free( recv_displs );
}

TEST_CASE( "Scatter a distributed array to all" )
{
   int n_ranks, rank;
   MPI_Comm_rank( MPI_COMM_WORLD, &rank );
   MPI_Comm_size( MPI_COMM_WORLD, &n_ranks );

   std::vector<unsigned> idxs( n_ranks*3 ), displs( 4 );
   for( int ii = 0; ii < idxs.size(); ++ii )
      idxs[ii] = ii;
   displs[0] = 0;
   displs[1] = 1;
   displs[2] = 3;
   displs[3] = 6;
   std::vector<int> data( 6 );
   data[0] = rank*6 + 0;
   data[1] = rank*6 + 1;
   data[2] = rank*6 + 2;
   data[3] = rank*6 + 3;
   data[4] = rank*6 + 4;
   data[5] = rank*6 + 5;
   int* recv_data;
   unsigned* recv_displs;
   scatterv( n_ranks*3, displs.data(), idxs.size(), idxs.data(), data.data(), (void**)&recv_data, &recv_displs, MPI_INT, MPI_COMM_WORLD );

   for( unsigned ii = 0; ii < idxs.size(); ++ii )
   {
      if( ii%3 == 0 )
      {
         REQUIRE( recv_displs[ii] == (ii/3)*6 );
         REQUIRE( recv_data[recv_displs[ii] + 0] == idxs[ii]*2 + 0 );
      }
      else if( ii%3 == 1 )
      {
         REQUIRE( recv_displs[ii] == (ii/3)*6 + 1 );
         REQUIRE( recv_data[recv_displs[ii] + 0] == idxs[ii]*2 - 1 );
         REQUIRE( recv_data[recv_displs[ii] + 1] == idxs[ii]*2 + 0 );
      }
      else if( ii%3 == 2 )
      {
         REQUIRE( recv_displs[ii] == (ii/3)*6 + 3 );
         REQUIRE( recv_data[recv_displs[ii] + 0] == idxs[ii]*2 - 1 );
         REQUIRE( recv_data[recv_displs[ii] + 1] == idxs[ii]*2 + 0 );
         REQUIRE( recv_data[recv_displs[ii] + 2] == idxs[ii]*2 + 1 );
      }
   }
   REQUIRE( recv_displs[idxs.size()] == 6*n_ranks );

   free( recv_data );
   free( recv_displs );
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
