#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <mpi.h>
#include "mapreduce.h"
#include "keyvalue.h"

using namespace std;
using namespace MAPREDUCE_NS

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

int main(int argc, char const *argv[])
{
	cout << "Hello\n";
	return 0;
}