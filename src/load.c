#include <assert.h>
#include "load.h"
#include "utils.h"

void
chunk_files( unsigned n_files,
             unsigned const* n_file_elems,
             unsigned* n_chunks,
             unsigned** chunks,
             MPI_Comm comm )
{
   int rank, n_ranks;
   unsigned n_elems, *file_displs, cur_offs, offs, n_local_elems, cur_file_offs;
   unsigned phase, ii;

   assert( !n_files || n_file_elems );
   assert( !n_files || n_chunks );
   assert( !n_files || chunks );

   MPI_OK( MPI_Comm_rank( comm, &rank ) );
   MPI_OK( MPI_Comm_size( comm, &n_ranks ) );

   file_displs = ALLOC( unsigned, n_files + 1 );
   make_displs2( n_files, n_file_elems, file_displs );
   n_elems = file_displs[n_files];

   n_local_elems = local_size( n_elems, n_ranks, rank );
   MPI_OK( MPI_Scan( &n_local_elems, &offs, 1, MPI_UNSIGNED, MPI_SUM, comm ) );
   offs -= n_local_elems;

   for( phase = 0; phase < 2; ++phase )
   {
      if( phase == 1 )
         *chunks = ALLOC( unsigned, 3*(*n_chunks) );
      *n_chunks = 0;
      cur_offs = offs;
      for( ii = 0; ii < n_files && cur_offs < offs + n_local_elems; ++ii )
      {
         if( cur_offs < file_displs[ii] + n_file_elems[ii] )
         {
            assert( cur_offs >= file_displs[ii] );
            if( phase == 1 )
            {
               (*chunks)[3*(*n_chunks) + 0] = ii;
               (*chunks)[3*(*n_chunks) + 1] = cur_offs - file_displs[ii];
            }
            cur_offs = MIN( cur_offs + n_file_elems[ii] - (cur_offs - file_displs[ii]), offs + n_local_elems );
            if( phase == 1 )
               (*chunks)[3*(*n_chunks) + 2] = cur_offs - ((*chunks)[3*(*n_chunks) + 1] + file_displs[ii]);
            ++(*n_chunks);
         }
      }
   }
}

void
fl_init_begin( file_loader_t* fl,
               unsigned n_files,
               MPI_Comm comm )
{
   fl->n_files = n_files;
   fl->n_file_elems = ALLOC( unsigned, n_files );
   fl->n_elems = 0;
   fl->ii = 0;
   fl->comm = comm;
}

int
fl_init_done( file_loader_t* fl )
{
   int done = fl->ii == fl->n_files;

   if( done )
   {
      int n_ranks, rank;

      MPI_Comm_size( fl->comm, &n_ranks );
      MPI_Comm_rank( fl->comm, &rank );

      chunk_files( fl->n_files, fl->n_file_elems, &fl->n_chunks, &fl->chunks, fl->comm );
      fl->n_local_elems = local_size( fl->n_elems, n_ranks, rank );
      MPI_OK( MPI_Scan( &fl->n_local_elems, &fl->elem_offs, 1, MPI_UNSIGNED, MPI_SUM, fl->comm ) );
      fl->elem_offs -= fl->n_local_elems;
   }

   return done;
}

void
fl_init_next( file_loader_t* fl )
{
   ++fl->ii;
}

unsigned
fl_init_file_index( file_loader_t const* fl )
{
   return fl->ii;
}

void
fl_init_set_n_file_elems( file_loader_t* fl,
                          unsigned n_elems )
{
   fl->n_file_elems[fl->ii] = n_elems;
   fl->n_elems += n_elems;
}

void
fl_load_begin( file_loader_t* fl )
{
   fl->data_offs = 0;
   fl->ii = 0;
}

int
fl_load_done( file_loader_t const* fl )
{
   return fl->ii == fl->n_chunks;
}

void
fl_load_next( file_loader_t* fl )
{
   fl->data_offs += fl_chunk_size( fl );
   ++fl->ii;
}

unsigned
fl_chunk_file_index( file_loader_t const* fl )
{
   return fl->chunks[3*fl->ii + 0];
}

unsigned
fl_chunk_offset( file_loader_t const* fl )
{
   return fl->chunks[3*fl->ii + 1];
}

unsigned
fl_chunk_size( file_loader_t const* fl )
{
   return fl->chunks[3*fl->ii + 2];
}

unsigned
fl_data_offset( file_loader_t const* fl,
                unsigned chunk_offs )
{
   assert( fl->data_offs + chunk_offs < fl->n_local_elems );
   return fl->data_offs + chunk_offs;
}

unsigned
fl_n_elems( file_loader_t const* fl )
{
   return fl->n_elems;
}

unsigned
fl_n_local_elems( file_loader_t const* fl )
{
   return fl->n_local_elems;
}

void
fl_free( file_loader_t* fl )
{
   FREE( fl->n_file_elems );
   FREE( fl->chunks );
}
