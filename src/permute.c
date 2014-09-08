#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "permute.h"
#include "utils.h"

void
count_required( unsigned n_elems,
                unsigned n_idxs,
                unsigned const* idxs,
                int n_ranks,
                unsigned* req_cnts,
                unsigned* req_displs )
{
   int rank;
   unsigned ii;

   for( ii = 0; ii < n_idxs; ++ii )
   {
      rank = locate_rank( n_elems, n_ranks, idxs[ii] );
      assert( rank < n_ranks );
      ++req_cnts[rank];
   }

   make_displs( n_ranks, req_cnts, req_displs );
}

void
make_required( unsigned n_elems,
               unsigned n_idxs,
               unsigned const* idxs,
               int n_ranks,
               unsigned* req_idxs,
               unsigned* req_cnts,
               unsigned const* req_displs,
               unsigned* local )
{
   int rank;
   unsigned ii;

   memset( req_cnts, 0, sizeof(unsigned)*n_ranks );

   for( ii = 0; ii < n_idxs; ++ii )
   {
      rank = locate_rank( n_elems, n_ranks, idxs[ii] );
      assert( rank < n_ranks );
      req_idxs[req_displs[rank] + req_cnts[rank]] = idxs[ii];
      local[req_displs[rank] + req_cnts[rank]] = ii;
      req_cnts[rank]++;
   }
}

void
scatter( unsigned n_elems,
         unsigned n_idxs,
         unsigned const* idxs,
         void const* data,
         void** recv_data,
         MPI_Datatype data_type,
         MPI_Comm comm )
{
   MPI_Datatype *out_types, *inc_types;
   unsigned *req_cnts, *req_displs, *req_idxs, *local;
   unsigned *out_cnts, *out_displs, *out_idxs;
   unsigned *ones, *zeros;
   unsigned n_local_elems, base;
   MPI_Aint elem_size;
   void *inc_data;
   int n_ranks, rank, ii, jj;

   assert( !n_elems || idxs );
   assert( !n_elems || data );
   assert( !n_elems || comm );

   MPI_OK( MPI_Comm_size( comm, &n_ranks ) );
   MPI_OK( MPI_Comm_rank( comm, &rank ) );

   /* Count the number of required elements coming from
      each processor, using a full array. */
   req_cnts = ALLOCZ( unsigned, n_ranks );
   req_displs = ALLOC( unsigned, n_ranks );
   count_required( n_elems, n_idxs, idxs, n_ranks, req_cnts, req_displs );

   /* Calculate required indices. */
   req_idxs = ALLOC( unsigned, req_displs[n_ranks - 1] + req_cnts[n_ranks - 1] );
   local = ALLOC( unsigned, req_displs[n_ranks - 1] + req_cnts[n_ranks - 1] );
   make_required( n_elems, n_idxs, idxs, n_ranks, req_idxs, req_cnts, req_displs, local );

   /* Send information about sizes. */
   out_cnts = ALLOC( unsigned, n_ranks );
   out_displs = ALLOC( unsigned, n_ranks );
   MPI_OK( MPI_Alltoall( req_cnts, 1, MPI_UNSIGNED, out_cnts, 1, MPI_UNSIGNED, comm ) );
   make_displs( n_ranks, out_cnts, out_displs );

   /* Send information about required indices. */
   out_idxs = ALLOC( unsigned, out_displs[n_ranks - 1] + out_cnts[n_ranks - 1] );
   MPI_OK( MPI_Alltoallv( req_idxs, (int*)req_cnts, (int*)req_displs, MPI_UNSIGNED,
                          out_idxs, (int*)out_cnts, (int*)out_displs, MPI_UNSIGNED, comm ) );
   FREE( req_idxs );

   /* Use a scan to find my base. */
   n_local_elems = local_size( n_elems, n_ranks, rank );
   MPI_OK( MPI_Scan( &n_local_elems, &base, 1, MPI_UNSIGNED, MPI_SUM, comm ) );
   base -= n_local_elems;

   /* Ensure outgoing indices are local indices. */
   for( ii = 0; ii < out_displs[n_ranks - 1] + out_cnts[n_ranks - 1]; ++ii )
   {
      assert( out_idxs[ii] >= base && out_idxs[ii] < base + n_local_elems );
      out_idxs[ii] -= base;
   }

   /* Create datatypes for outgoing information. */
   out_types = ALLOC( MPI_Datatype, n_ranks );
   for( ii = 0; ii < n_ranks; ++ii )
   {
      ones = ALLOC( unsigned, out_cnts[ii] );
      for( jj = 0; jj < out_cnts[ii]; ++jj )
         ones[jj] = 1;
      MPI_OK( MPI_Type_indexed( out_cnts[ii], (int*)ones, (int*)(out_idxs + out_displs[ii]),
                                data_type, out_types + ii ) );
      MPI_OK( MPI_Type_commit( out_types + ii ) );
      FREE( ones );
   }
   FREE( out_cnts );
   FREE( out_idxs );
   FREE( out_displs );

   /* Create incoming datatypes to put information in the
      correct positions. */
   inc_types = ALLOC( MPI_Datatype, n_ranks );
   for( ii = 0; ii < n_ranks; ++ii )
   {
      ones = ALLOC( unsigned, req_cnts[ii] );
      for( jj = 0; jj < req_cnts[ii]; ++jj )
         ones[jj] = 1;
      MPI_OK( MPI_Type_indexed( req_cnts[ii], (int*)ones, (int*)(local + req_displs[ii]),
                                data_type, inc_types + ii ) );
      MPI_OK( MPI_Type_commit( inc_types + ii ) );
      FREE( ones );
   }
   FREE( req_cnts );
   FREE( local );
   FREE( req_displs );

   /* Send/copy data. */
   MPI_OK( MPI_Type_extent( data_type, &elem_size ) );
   inc_data = (void*)ALLOC( uint8_t, n_idxs*elem_size );
   zeros = ALLOCZ( unsigned, n_ranks );
   ones = ALLOC( unsigned, n_ranks );
   for( ii = 0; ii < n_ranks; ++ii )
      ones[ii] = 1;
   MPI_OK( MPI_Alltoallw( (void*)data, (int*)ones, (int*)zeros, out_types,
                          inc_data, (int*)ones, (int*)zeros, inc_types, comm ) );
   FREE( zeros );
   FREE( ones );

   /* Don't forget to free the types. */
   for( ii = 0; ii < n_ranks; ++ii )
   {
      MPI_OK( MPI_Type_free( out_types + ii ) );
      MPI_OK( MPI_Type_free( inc_types + ii ) );
   }
   FREE( out_types );
   FREE( inc_types );

   /* Store results. */
   *recv_data = inc_data;
}

void
scatterv( unsigned n_elems,
          unsigned const* elem_displs,
          unsigned n_idxs,
          unsigned const* idxs,
          void const* data,
          void** recv_data,
          unsigned** recv_displs,
          MPI_Datatype data_type,
          MPI_Comm comm )
{
   MPI_Datatype *out_types, *inc_types;
   unsigned *req_cnts, *req_displs, *req_idxs, *local;
   unsigned *out_cnts, *out_displs, *out_idxs;
   unsigned *ones, *zeros;
   unsigned n_local_elems, base;
   unsigned *elem_cnts, *inc_elem_displs, *inc_elem_cnts;
   MPI_Aint elem_size;
   void *inc_data;
   int n_ranks, rank, ii, jj;

   assert( !n_elems || idxs );
   assert( !n_elems || data );
   assert( !n_elems || comm );

   MPI_OK( MPI_Comm_size( comm, &n_ranks ) );
   MPI_OK( MPI_Comm_rank( comm, &rank ) );

   /* Count the number of required elements coming from
      each processor, using a full array. */
   req_cnts = ALLOCZ( unsigned, n_ranks );
   req_displs = ALLOC( unsigned, n_ranks );
   count_required( n_elems, n_idxs, idxs, n_ranks, req_cnts, req_displs );

   /* Calculate required indices. */
   req_idxs = ALLOC( unsigned, req_displs[n_ranks - 1] + req_cnts[n_ranks - 1] );
   local = ALLOC( unsigned, req_displs[n_ranks - 1] + req_cnts[n_ranks - 1] );
   make_required( n_elems, n_idxs, idxs, n_ranks, req_idxs, req_cnts, req_displs, local );

   /* Send information about sizes. */
   out_cnts = ALLOC( unsigned, n_ranks );
   out_displs = ALLOC( unsigned, n_ranks );
   MPI_OK( MPI_Alltoall( req_cnts, 1, MPI_UNSIGNED, out_cnts, 1, MPI_UNSIGNED, comm ) );
   make_displs( n_ranks, out_cnts, out_displs );

   /* Send information about required indices. */
   out_idxs = ALLOC( unsigned, out_displs[n_ranks - 1] + out_cnts[n_ranks - 1] );
   MPI_OK( MPI_Alltoallv( req_idxs, (int*)req_cnts, (int*)req_displs, MPI_UNSIGNED,
                          out_idxs, (int*)out_cnts, (int*)out_displs, MPI_UNSIGNED, comm ) );
   FREE( req_idxs );

   /* Use a scan to find my base. */
   n_local_elems = local_size( n_elems, n_ranks, rank );
   MPI_OK( MPI_Scan( &n_local_elems, &base, 1, MPI_UNSIGNED, MPI_SUM, comm ) );
   base -= n_local_elems;

   /* Ensure outgoing indices are local indices. */
   for( ii = 0; ii < out_displs[n_ranks - 1] + out_cnts[n_ranks - 1]; ++ii )
   {
      assert( out_idxs[ii] >= base && out_idxs[ii] < base + n_local_elems );
      out_idxs[ii] -= base;
   }

   /* Create datatypes for outgoing counts. */
   out_types = ALLOC( MPI_Datatype, n_ranks );
   for( ii = 0; ii < n_ranks; ++ii )
   {
      ones = ALLOC( unsigned, out_cnts[ii] );
      for( jj = 0; jj < out_cnts[ii]; ++jj )
         ones[jj] = 1;
      MPI_OK( MPI_Type_indexed( out_cnts[ii], (int*)ones, (int*)(out_idxs + out_displs[ii]),
                                MPI_UNSIGNED, out_types + ii ) );
      MPI_OK( MPI_Type_commit( out_types + ii ) );
      FREE( ones );
   }

   /* Create incoming datatypes to put counts int the
      correct positions. */
   inc_types = ALLOC( MPI_Datatype, n_ranks );
   for( ii = 0; ii < n_ranks; ++ii )
   {
      ones = ALLOC( unsigned, req_cnts[ii] );
      for( jj = 0; jj < req_cnts[ii]; ++jj )
         ones[jj] = 1;
      MPI_OK( MPI_Type_indexed( req_cnts[ii], (int*)ones, (int*)(local + req_displs[ii]),
                                MPI_UNSIGNED, inc_types + ii ) );
      MPI_OK( MPI_Type_commit( inc_types + ii ) );
      FREE( ones );
   }

   /* Create element block counts and send. */
   elem_cnts = ALLOC( unsigned, n_local_elems );
   make_counts( n_local_elems, elem_displs, elem_cnts );
   zeros = ALLOCZ( unsigned, n_ranks );
   ones = ALLOC( unsigned, n_ranks );
   for( ii = 0; ii < n_ranks; ++ii )
      ones[ii] = 1;
   inc_elem_cnts = ALLOC( unsigned, n_idxs );
   MPI_OK( MPI_Alltoallw( elem_cnts, (int*)ones, (int*)zeros, out_types,
                          inc_elem_cnts, (int*)ones, (int*)zeros, inc_types, comm ) );
   inc_elem_displs = ALLOC( unsigned, n_idxs + 1 );
   make_displs( n_idxs, inc_elem_cnts, inc_elem_displs );
   inc_elem_displs[n_idxs] = inc_elem_displs[n_idxs - 1] + inc_elem_cnts[n_idxs - 1];

   /* Create datatypes for outgoing data. */
   for( ii = 0; ii < n_ranks; ++ii )
   {
      unsigned *cnts, *displs;

      cnts = ALLOC( unsigned, out_cnts[ii] );
      displs = ALLOC( unsigned, out_cnts[ii] );
      for( jj = 0; jj < out_cnts[ii]; ++jj )
      {
         displs[jj] = elem_displs[(out_idxs + out_displs[ii])[jj]];
         cnts[jj] = elem_cnts[(out_idxs + out_displs[ii])[jj]];
      }

      MPI_OK( MPI_Type_free( out_types + ii ) );
      MPI_OK( MPI_Type_indexed( out_cnts[ii], (int*)cnts, (int*)displs, data_type, out_types + ii ) );
      MPI_OK( MPI_Type_commit( out_types + ii ) );

      FREE( cnts );
      FREE( displs );
   }
   FREE( out_idxs );
   FREE( out_cnts );
   FREE( out_displs );
   FREE( elem_cnts );

   /* Create incoming datatypes to put counts int the
      correct positions. */
   for( ii = 0; ii < n_ranks; ++ii )
   {
      unsigned *cnts, *displs;

      cnts = ALLOC( unsigned, req_cnts[ii] );
      displs = ALLOC( unsigned, req_cnts[ii] );
      for( jj = 0; jj < req_cnts[ii]; ++jj )
      {
         displs[jj] = inc_elem_displs[(local + req_displs[ii])[jj]];
         cnts[jj] = inc_elem_cnts[(local + req_displs[ii])[jj]];
      }

      MPI_OK( MPI_Type_free( inc_types + ii ) );
      MPI_OK( MPI_Type_indexed( req_cnts[ii], (int*)cnts, (int*)displs, data_type, inc_types + ii ) );
      MPI_OK( MPI_Type_commit( inc_types + ii ) );

      FREE( cnts );
      FREE( displs );
   }
   FREE( req_cnts );
   FREE( req_displs );
   FREE( local );

   /* Send/copy data. */
   MPI_OK( MPI_Type_extent( data_type, &elem_size ) );
   inc_data = (void*)ALLOC( uint8_t, elem_size*(inc_elem_displs[n_idxs - 1] + inc_elem_cnts[n_idxs - 1]) );
   for( ii = 0; ii < n_ranks; ++ii )
      ones[ii] = 1;
   MPI_OK( MPI_Alltoallw( (void*)data, (int*)ones, (int*)zeros, out_types,
                          inc_data, (int*)ones, (int*)zeros, inc_types, comm ) );
   FREE( zeros );
   FREE( ones );
   FREE( inc_elem_cnts );

   /* Don't forget to free the types. */
   for( ii = 0; ii < n_ranks; ++ii )
   {
      MPI_OK( MPI_Type_free( out_types + ii ) );
      MPI_OK( MPI_Type_free( inc_types + ii ) );
   }
   FREE( out_types );
   FREE( inc_types );

   /* Store results. */
   *recv_displs = inc_elem_displs;
   *recv_data = inc_data;
}

void
permute( unsigned n_elems,
         unsigned n_idxs,
         unsigned const* idxs,
         void** data,
         MPI_Datatype data_type,
         MPI_Comm comm )
{
   void* recv_data;

   scatter( n_elems, n_idxs, idxs, *data, &recv_data, data_type, comm );
   free( *data );
   *data = recv_data;
}

void
permutev( unsigned n_elems,
          unsigned** elem_displs,
          unsigned n_idxs,
          unsigned const* idxs,
          void** data,
          MPI_Datatype data_type,
          MPI_Comm comm )
{
   void *recv_data;
   unsigned *recv_displs;

   scatterv( n_elems, *elem_displs, n_idxs, idxs, *data, &recv_data, &recv_displs, data_type, comm );
   free( *data );
   free( *elem_displs );
   *data = recv_data;
   *elem_displs = recv_displs;
}
