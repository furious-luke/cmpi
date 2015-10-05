#include <mpi.h>
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "dump.h"

TEST_CASE()
{
   dump_t dump;
   unsigned cnt = 0;

   std::vector<unsigned> displs( 4 ), elems( 5 );
   displs[0] = 0; displs[1] = 1; displs[2] = 3; displs[3] = 5;
   for( int ii = 0; ii < 5; ++ii )
      elems[ii] = rank*5 + ii;

   for( dumpv_begin( &dump, 3, displs.data(), elems, MPI_INT, MPI_COMM_WORLD );
        !dumpv_done();
        dumpv_next() )
   {
      REQUIRE( dumpv_index( &dump ) == cnt );
      REQUIRE( dumpv_index( &dump ) == cnt );
      ++cnt;
   }
}
