/*
** Demonstrate the use of the file loader interface and
** the permutation routines. We have three stages of operation here.
**
**  1. First load the particle IDs as a distributed array.
**
**  2. Load the ranges of particle IDs associated with each halo.
**     Using this, permute the particle IDs to have the correct IDs
**     distributed on the correct ranks.
**
**  3. Load the mapping of halos to galaxies. Permute the 2D array of
**     particle IDs as associated to halos into galaxy order.
**
** @author Luke Hodkinson, 2014
*/

#include <stdlib.h>
#include <stdio.h>
#include <argp.h>
#include <mpi.h>
#include "../src/load.h"
#include "../src/utils.h"

struct arguments
{
   char* pids_prefix;
   unsigned pids_n_files;
   char* halos_prefix;
   unsigned halos_n_files;
   char* gals_prefix;
   unsigned gals_n_files;
};
typedef struct arguments arguments_t;

static struct argp_option prog_options[] = { 0 };

static error_t
parse_option( int key,
              char* arg,
              struct argp_state* state )
{
   arguments_t *args = state->input;

   switch( key )
   {
      case ARGP_KEY_ARG:
         if( state->arg_num == 0 )
            args->pids_prefix = arg;
         else if( state->arg_num == 1 )
         {
            char *ptr;
            args->pids_n_files = (unsigned)strtol( arg, &ptr, 10 );
            if( !ptr )
               argp_usage( state );
         }
         else if( state->arg_num == 2 )
            args->halos_prefix = arg;
         else if( state->arg_num == 3 )
         {
            char *ptr;
            args->halos_n_files = (unsigned)strtol( arg, &ptr, 10 );
            if( !ptr )
               argp_usage( state );
         }
         else if( state->arg_num == 4 )
            args->gals_prefix = arg;
         else if( state->arg_num == 5 )
         {
            char *ptr;
            args->gals_n_files = (unsigned)strtol( arg, &ptr, 10 );
            if( !ptr )
               argp_usage( state );
         }
         else
            argp_usage( state );
         break;

      case ARGP_KEY_END:
         if( state->arg_num < 6 )
            argp_usage( state );
         break;

      default:
         return ARGP_ERR_UNKNOWN;
   }
   return 0;
}

static char args_doc[] = "PIDSPREFIX PIDSNFILES HALOSPREFIX HALOSNFILES";

static struct argp argp = { prog_options, parse_option, args_doc, NULL };

int
main( int argc,
      char** argv )
{
   int rank, n_ranks;
   arguments_t args;
   unsigned n_pids, n_local_pids, *pids_data;
   unsigned n_halos, n_local_halos, *halos_data, *halos_displs;
   unsigned n_gals, n_local_gals, *gals_data;
   unsigned n_local_idxs, *local_idxs;
   unsigned n_elems;
   file_loader_t fl;
   char fn[1000];
   FILE* file;
   int ii, jj, kk;

   /* Initialise MPI. */
   MPI_Init( &argc, &argv );
   MPI_Comm_size( MPI_COMM_WORLD, &n_ranks );
   MPI_Comm_rank( MPI_COMM_WORLD, &rank );

   /* Parse arguments. */
   argp_parse( &argp, argc, argv, 0, 0, &args );

   /*
    * We first read in the set of particle IDs. We use the file loader
    * interface to make sure we evenly distribute the IDs regardless
    * of how they are ordered in the source files and independent of
    * how many ranks were used in their generation and how many ranks
    * we are currently running with.
    */

   /* Initialise number of elements in each file. */
   for( fl_init_begin( &fl, args.pids_n_files, MPI_COMM_WORLD );
        !fl_init_done( &fl );
        fl_init_next( &fl ) )
   {
      /* Read in the number of elements in the current file. */
      sprintf( fn, "%s.%05d", args.pids_prefix, fl_init_file_index( &fl ) );
      file = fopen( fn, "r" );
      assert( file );
      fscanf( file, "%d", &n_elems );
      fl_init_set_n_file_elems( &fl, n_elems );
   }

   /* Allocate for local storage for the particle IDs. */
   n_pids = fl_n_elems( &fl );
   n_local_pids = fl_n_local_elems( &fl );
   pids_data = ALLOC( unsigned, n_local_pids );

   /* Load each chunk into storage. */
   for( fl_load_begin( &fl );
        !fl_load_done( &fl );
        fl_load_next( &fl ) )
   {
      sprintf( fn, "%s.%05d", args.pids_prefix, fl_chunk_file_index( &fl ) );
      file = fopen( fn, "r" );
      assert( file );

      /* Skip forward through the file to the beginning of the chunk. If we
         were using binary files we would just use fseek. */
      for( jj = 0; jj < fl_chunk_offset( &fl ) + 1; ++jj ) /* +1 for header */
      {
         unsigned dummy;
         fscanf( file, "%d", &dummy );
      }

      /* Read the data. */
      for( jj = 0; jj < fl_chunk_size( &fl ); ++jj )
         fscanf( file, "%d", pids_data + fl_data_offset( &fl, jj ) );
   }
   fl_free( &fl );

   /*
    * We now have a distributed array of the elements from the files.
    * However they are not distributed in the way we want. To fix this,
    * we must first load the index ranges we need. These index ranges
    * are the equivalent of the information about which particle IDs are
    * associated with which halo.
    */

   /* Initialise number of elements in each file. */
   for( fl_init_begin( &fl, args.halos_n_files, MPI_COMM_WORLD );
        !fl_init_done( &fl );
        fl_init_next( &fl ) )
   {
      /* Read in the number of halos in each file. */
      sprintf( fn, "%s.%05d", args.halos_prefix, fl_init_file_index( &fl ) );
      file = fopen( fn, "r" );
      assert( file );
      fscanf( file, "%d", &n_elems );
      fl_init_set_n_file_elems( &fl, n_elems );
   }

   /* Allocate for local storage for halo - particle ID info. */
   n_halos = fl_n_elems( &fl );
   n_local_halos = fl_n_local_elems( &fl );
   halos_data = ALLOC( unsigned, 2*n_local_halos );

   /* Load each chunk into storage. */
   for( fl_load_begin( &fl ); !fl_load_done( &fl ); fl_load_next( &fl ) )
   {
      unsigned dummy;

      sprintf( fn, "%s.%05d", args.halos_prefix, fl_chunk_file_index( &fl ) );
      file = fopen( fn, "r" );
      assert( file );
      fscanf( file, "%d", &dummy ); /* header */
      for( jj = 0; jj < fl_chunk_offset( &fl ); ++jj )
      {
         fscanf( file, "%d", &dummy );
         fscanf( file, "%d", &dummy );
      }

      /* For each halo we have a beginning and an end index into the global
         particle ID array. */
      for( jj = 0; jj < fl_chunk_size( &fl ); ++jj )
      {
         fscanf( file, "%d", halos_data + 2*fl_data_offset( &fl, jj ) + 0 );
         fscanf( file, "%d", halos_data + 2*fl_data_offset( &fl, jj ) + 1 );
      }
   }
   fl_free( &fl );

   /*
    * We have both the PIDs and the data that maps halos to PID ranges. Now
    * I need to convert the ranges to a set of indices I can use to permute
    * the PIDs into the correct order.
    */

   /* Sum up how many indices will exist on my rank, and also construct
      the displacements information for halo particle IDs. */
   n_local_idxs = 0;
   for( ii = 0; ii < n_local_halos; ++ii )
      n_local_idxs += halos_data[2*ii + 1] - halos_data[2*ii + 0];

   /* Allocate and generate indices. */
   local_idxs = ALLOC( unsigned, n_local_idxs );
   halos_displs = ALLOC( unsigned, n_local_halos + 1 );
   halos_displs[0] = 0;
   for( ii = 0, kk = 0; ii < n_local_halos; ++ii )
   {
      for( jj = halos_data[2*ii + 0]; jj < halos_data[2*ii + 1]; ++jj, ++kk )
         local_idxs[kk] = jj;
      halos_displs[ii + 1] = halos_displs[ii] + halos_data[2*ii + 1] - halos_data[2*ii];
   }

   /* Call the permutation. */
   permute( n_pids, n_local_idxs, local_idxs, &pids_data, MPI_UNSIGNED, MPI_COMM_WORLD );

   /*
    * At this point we have the sets of PIDs that are associated with the halos
    * we loaded, in the order we loaded them, all available locally on each process.
    * However, what we really want is to reorder the halos so that they are in
    * galaxy order.
    */

   /* Load the halos that are associated with each galaxy. */
   for( fl_init_begin( &fl, args.gals_n_files, MPI_COMM_WORLD );
        !fl_init_done( &fl );
        fl_init_next( &fl ) )
   {
      sprintf( fn, "%s.%05d", args.gals_prefix, fl_init_file_index( &fl ) );
      file = fopen( fn, "r" );
      assert( file );
      fscanf( file, "%d", &n_elems );
      fl_init_set_n_file_elems( &fl, n_elems );
   }

   /* Allocate for local storage. */
   n_gals = fl_n_elems( &fl );
   n_local_gals = fl_n_local_elems( &fl );
   gals_data = ALLOC( unsigned, n_local_gals );

   /* Load each chunk into storage. */
   for( fl_load_begin( &fl );
        !fl_load_done( &fl );
        fl_load_next( &fl ) )
   {
      sprintf( fn, "%s.%05d", args.gals_prefix, fl_chunk_file_index( &fl ) );
      file = fopen( fn, "r" );
      assert( file );
      for( jj = 0; jj < fl_chunk_offset( &fl ) + 1; ++jj ) /* +1 for header */
      {
         unsigned dummy;
         fscanf( file, "%d", &dummy );
      }
      for( jj = 0; jj < fl_chunk_size( &fl ); ++jj )
         fscanf( file, "%d", gals_data + fl_data_offset( &fl, jj ) );
   }
   fl_free( &fl );

   /* The galaxy data is just the index for the halo it's associated with.
      Now we can just perform a permute to place the PIDs associated with each
      halo on the correct process. */
   permutev( n_gals, &halos_displs, n_gals, gals_data, &halos_data, MPI_UNSIGNED, MPI_COMM_WORLD );

   /*
    * Now we have the particle IDs associated with each galaxy loaded, and
    * ordered in galaxy order, and also distributed evenly across the ranks.
    * From here you could do a gatherall to get the particle IDs for each
    * galaxy on all the ranks. Ideally, in the future, you would perform
    * an inversion of this information and create a distributed map.
    */

   MPI_Finalize();
   return EXIT_SUCCESS;
}
