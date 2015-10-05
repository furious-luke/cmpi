#include "dump.h"

void
dumpv_begin( dump_t* dump,
             unsigned n_elems,
             unsigned const* displs,
             void const* elems,
             MPI_Datatype data_type,
             MPI_Comm comm )
{
   dump->n_lelems = n_elems;
   dump->displs = displs;
   dump->elems = elems;
   dump->data_type = data_type;
   dump->comm = comm;

   MPI_OK( MPI_Comm_size( dump->comm, &dump->n_ranks ) );
   MPI_OK( MPI_Comm_rank( dump->comm, &dump->rank ) );
   MPI_OK( MPI_Allreduce( &dump->n_lelems, 1, MPI_UNSIGNED, &dump->n_gelems, 1, MPI_UNSIGNED, MPI_SUM, dump->comm ) );
   MPI_OK( MPI_Datatype_size( dump->data_type, &dump->elem_size ) );

   if( dump->rank != 0 )
   {
      while( dump->n_gelems )
         dump_transfer();
   }
   else
   {
      dump->rank_i = 0;
      dump->buf_i = 0;
      dump->gidx = 0;
      dump->buf = ALLOC( char, 100*dump->elem_size );
      dumpv_transfer();
   }
}

int
dumpv_done( dump_t const* dump )
{
   return dump->n_gelems == 0;
}

void
dumpv_next( dump_t* dump )
{
   if( ++dump->buf_i == dump->n_buf_elems )
      dumpv_transfer();
}

void
dumpv_transfer( dump_t* dump )
{
   if( dump->rank == dump->rank_i )
      dump->n_buf_elems = MIN( 100, dump->n_elems );
   MPI_OK( MPI_Bcast( &dump->n_buf_elems, 1, MPI_UNSIGNED, dump->rank_i, dump->comm ) );

   if( dump->rank == 0 )
   {
      if( dump->rank_i == 0 )
      {
         memcpy( dump->buf, dump->elems, n_elems*dump->elem_size );
      }
      else
      {
         MPI_OK( MPI_Irecv( &dump->n_buf_elems, 1, MPI_UNSIGNED, dump->rank_i, dump->comm ) );
         MPI_OK( MPI_Irecv( &dump->buf, dump->n_buf_elems, dump->data_type, dump->rank_i, dump->comm ) );
      }
   }
   else if( dump->rank == dump->rank_i )
   {
      MPI_OK( MPI_Send( &n_elems, 1, MPI_UNSIGNED, dump->rank_i, dump->comm ) );
      MPI_OK( MPI_Send( dump->elems, n_elems, MPI_UNSIGNED, dump->rank_i, dump->comm ) );
   }

   if( dump->rank == dump->rank_i )
      dump->elems += dump->n_buf_elems;
   dump->n_elems -= dump->n_buf_elems;
}

unsigned
dumpv_index( dump_t const* dump )
{
   return dump->gidx;
}

void const*
dumpv_element( dump_t const* dump )
{
   return dump->buf + dump->buf_i*dump->elem_size;
}
