#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <mpi.h>
#include "mr_mpi_lib/mapreduce.h"
#include "mr_mpi_lib/keyvalue.h"

using namespace std;
using namespace MAPREDUCE_NS;

#define WORLD MPI_COMM_WORLD
#define Init MPI_Init
#define Finalize MPI_Finalize
#define Send MPI_Send
#define Recv MPI_Recv
#define INT MPI_INT
#define DOUBLE MPI_DOUBLE
#define Status MPI_Status
#define Broadcast MPI_Bcast

#define DAMP 0.85
#define ERR_THRESHOLD 1e-5

int rank_id, procs;

int main(int argc, char *argv[])
{
	Init(&argc, &argv);
	MapReduce *mr = new MapReduce(WORLD);
	
	mr->collate();
	delete mr;
	Finalize();
	return 0;
}