#include <mpi.h>

struct dump
{
   unsigned n_lelems;
   unsigned n_gelems;
   unsigned const* displs;
   void const* elems;
   MPI_Datatype data_type;
   MPI_comm comm;
   unsigned gidx;
};
typedef struct dump dump_t;

void
dumpv_begin( dump_t* dump,
             unsigned n_elems,
             unsigned const* displs,
             void const* elems,
             MPI_Datatype data_type,
             MPI_Comm comm );

int
dumpv_done( dump_t* dump );

void
dumpv_next( dump_t* dump );

unsigned
dumpv_index( dump_t* dump );

void const*
dumpv_element( dump_t* dump );
