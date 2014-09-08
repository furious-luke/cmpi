/*!
** @file
** @author Luke Hodkinson, 2014
*/

#ifndef load_h
#define load_h

#include <mpi.h>

struct file_loader
{
   unsigned  n_files;
   unsigned* n_file_elems;
   unsigned  n_elems;
   unsigned  n_chunks;
   unsigned* chunks;
   unsigned  n_local_elems;
   unsigned  elem_offs;
   unsigned  data_offs;
   unsigned  ii;
   MPI_Comm  comm;
};
typedef struct file_loader file_loader_t;

/*!
** Calculate file chunks to load. Using the number of files and
** an array of the number of elements in each file, calculate
** which parts of the set of files this rank should load to be
** in implicit order. The chunks are an array of triples,
** (file index, element offset, number of elements). The array
** must be freed when finished with.
**
** @param[in]  n_files      number of files to load
** @param[in]  n_file_elems number of elements in each file
** @param[out] n_chunks     number of chunks to load
** @param[out] chunks       array of chunks to load
** @param[in]  comm         MPI communicator
*/
void
chunk_files( unsigned n_files,
             unsigned const* n_file_elems,
             unsigned* n_chunks,
             unsigned** chunks,
             MPI_Comm comm );

/*!
** Begin initialising a file loader.
**
** @param[inout] fl      file loader structure to initialise
** @param[in]    n_files number of files to load
** @param[in]    comm    MPI communicator
*/
void
fl_init_begin( file_loader_t* fl,
               unsigned n_files,
               MPI_Comm comm );

/*!
** Test for completion of file loader initialisation.
**
** @param[in] fl file loader structure
*/
int
fl_init_done( file_loader_t* fl );

/*!
** Advance file loader initialisation.
**
** @param[in] fl file loader structure
*/
void
fl_init_next( file_loader_t* fl );

unsigned
fl_init_file_index( file_loader_t const* fl );

void
fl_init_set_n_file_elems( file_loader_t* fl,
                          unsigned n_elems );

void
fl_load_begin( file_loader_t* fl );

int
fl_load_done( file_loader_t const* fl );

void
fl_load_next( file_loader_t* fl );

unsigned
fl_chunk_file_index( file_loader_t const* fl );

unsigned
fl_chunk_offset( file_loader_t const* fl );

unsigned
fl_chunk_size( file_loader_t const* fl );

unsigned
fl_data_offset( file_loader_t const* fl,
                unsigned chunk_offs );

unsigned
fl_n_elems( file_loader_t const* fl );

unsigned
fl_n_local_elems( file_loader_t const* fl );

void
fl_free( file_loader_t* fl );

#endif
