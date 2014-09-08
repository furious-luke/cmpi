/*!
** @file
** @author Luke Hodkinson, 2014
*/

#ifndef permute_h
#define permute_h

#include <mpi.h>

/*!
** Send/recv indexed data. Using an array of desired indices,
** scatter the implicitly ordered data to the appropriate
** locations.
**
** @param[in]  n_elems   number of global data elements
** @param[in]  n_idxs    number of local desired indices
** @param[in]  idxs      array of desired local indices
** @param[in]  data      array of local data elements
** @param[out] recv_data resulting data elements
** @param[in]  data_type MPI datatype of data elements
** @param[in]  comm      MPI communicator
*/
void
scatter( unsigned n_elems,
         unsigned n_idxs,
         unsigned const* idxs,
         void const* data,
         void** recv_data,
         MPI_Datatype data_type,
         MPI_Comm comm );

/*!
** Send/recv indexed CSR data. Using an array of desired indices,
** scatter the implicitly ordered data to the appropriate
** locations.
**
** @param[in]  n_elems     number of global data elements
** @param[in]  elem_displs displacements of local data elements
** @param[in]  n_idxs      number of local desired indices
** @param[in]  idxs        array of desired local indices
** @param[in]  data        array of local data elements
** @param[out] recv_data   resulting data elements
** @param[out] recv_displs resulting data element displacements
** @param[in]  data_type   MPI datatype of data elements
** @param[in]  comm        MPI communicator
*/
void
scatterv( unsigned n_elems,
          unsigned const* elem_displs,
          unsigned n_idxs,
          unsigned const* idxs,
          void const* data,
          void** recv_data,
          unsigned** recv_displs,
          MPI_Datatype data_type,
          MPI_Comm comm );

/*!
** Permute indexed data. Using an array of desired indices,
** permute the implicitly ordered data to the appropriate
** locations.
**
** @param[in]    n_elems   number of global data elements
** @param[in]    n_idxs    number of local desired indices
** @param[in]    idxs      array of desired local indices
** @param[inout] data      array of local data elements
** @param[in]    data_type MPI datatype of data elements
** @param[in]    comm      MPI communicator
*/
void
permute( unsigned n_elems,
         unsigned n_idxs,
         unsigned const* idxs,
         void** data,
         MPI_Datatype data_type,
         MPI_Comm comm );

/*!
** Permute indexed CSR data. Using an array of desired indices,
** permute the implicitly ordered data to the appropriate
** locations.
**
** @param[in]    n_elems     number of global data elements
** @param[inout] elem_displs displacements of local data elements
** @param[in]    n_idxs      number of local desired indices
** @param[in]    idxs        array of desired local indices
** @param[inout] data        array of local data elements
** @param[in]    data_type   MPI datatype of data elements
** @param[in]    comm        MPI communicator
*/
void
permutev( unsigned n_elems,
          unsigned** elem_displs,
          unsigned n_idxs,
          unsigned const* idxs,
          void** data,
          MPI_Datatype data_type,
          MPI_Comm comm );

#endif
