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
#define Comm_Rank MPI_Comm_rank
#define Comm_Size MPI_Comm_size
#define Finalize MPI_Finalize
#define Send MPI_Send
#define Recv MPI_Recv
#define INT MPI_INT
#define DOUBLE MPI_DOUBLE
#define Status MPI_Status
#define Broadcast MPI_Bcast

#define DAMP 0.85
#define ERR_THRESHOLD 1e-5

int rank_id, procs, N;
map<int, vector<int> > graph;
double danglingPG = 0;
vector<double> pageranks;
vector<double> new_pageranks;
vector<double> zero;

void read_graph(string filename)
{
	ifstream fin;
	fin.open(filename);
	int x, y, max_v = 0;
	while(fin >> x >> y)
	{
		graph[x].push_back(y);
		max_v = max(x, max(y, max_v));
	}
	fin.close();
	for(int i=0;i<=max_v;++i)
		graph[i];
	N = graph.size();
	for(int i=0; i<N; ++i)
	{
		pageranks.push_back(1.0 / N);
		zero.push_back(0);
	}
	new_pageranks = zero;
}

double sum(const vector<double> &v)
{
	double res = 0.0;
	for(int i=0;i<v.size();++i)
		res += v[i];
	return res;
}

void write_graph(string filename)
{
	ofstream fout(filename);
	for(int i=0;i<N;++i)
		fout << i << " = " << pageranks[i] << "\n";
	fout << "sum = " << sum(pageranks) <<"\n";
	fout.close();
}

void send_graph()
{
	for(int c=1; c<procs; ++c)
	{
		for(int i=c; i<N; i+=procs)
		{
			int size = graph[i].size();
			Send(&size, 1, INT, c, 0, WORLD);
			for(int j=0; j<size; ++j)
				Send(&graph[i][j], 1, INT, c, 0, WORLD);
		}		
	}
}

void receive_graph()
{
	Status status;
	for(int i=rank_id;i<N;i+=procs)
	{
		int size;
		Recv(&size, 1, INT, 0, 0, WORLD, &status);
		for(int j=0;j<size;++j)
		{
			int x;
			Recv(&x, 1, INT, 0, 0, WORLD, &status);
			graph[i].push_back(x);
		}
	}

	for(int i=0;i<N;++i)
	{
		pageranks.push_back(0);
		new_pageranks.push_back(0);
		zero.push_back(0);
	}
}

double get_error(const vector<double> &v1, const vector<double> &v2)
{
	double res = 0.0;
	for(int i=0;i<v1.size();++i)
		res += abs(v1[i] - v2[i]);
	return res;
}

void mymap(int id, KeyValue *kv, void *ptr)
{
	for(int i=id; i<N; i+=procs)
	{
		int size = graph[i].size();
		if(size == 0)
			danglingPG += pageranks[i];
		for(int j=0; j<size; ++j)
		{
			int a = graph[i][j];
			double b = pageranks[i] / size;
			kv->add((char*)(&a), sizeof(int), (char*)(&b), sizeof(double));
		}
	}
}

void myreduce(char *key, int keybytes, char *multivalue, 
	int nvalues, int *valuebytes, KeyValue *kv, void *ptr)
{
	double *ar = (double*)multivalue;
	double res = 0.0;
	for(int i=0; i<nvalues; ++i)
		res += ar[i];

	kv->add(key, keybytes, (char*)(&res), sizeof(double));
}

void myscan(char *key, int keybytes, char *value, int valuebytes, void *ptr)
{
	int a = *((int*)(key));
	double b = *((double*)(value));
	new_pageranks[a] = b;
}

int main(int argc, char *argv[])
{
	Init(&argc, &argv);
	Comm_Rank(WORLD, &rank_id);
	Comm_Size(WORLD, &procs);
	Status status;

	// INIT Process
	string filename(argv[1]);

	if(!rank_id)
		read_graph(filename + ".txt");
	
	Broadcast(&N, 1, INT, 0, WORLD);
	if(rank_id)
		receive_graph();
	else
		send_graph();

	double error = 1.0;

	MapReduce *mr = new MapReduce(WORLD);
	while(error > ERR_THRESHOLD)
	{
		for(int i=0;i<N;++i)
			Broadcast(&pageranks[i], 1, DOUBLE, 0, WORLD);
		mr->map(procs, &mymap, NULL);
		mr->collate(NULL);
		mr->reduce(&myreduce, NULL);
		mr->gather(1);
		new_pageranks = zero;
		mr->scan(&myscan, NULL);
		if(!rank_id)
		{
			// Dangling sum
			for(int c=1; c<procs; ++c)
			{
				double temp_sum;
				Recv(&temp_sum, 1, DOUBLE, c, 0, WORLD, &status);
				danglingPG += temp_sum;
			}

			for(int i=0;i<N;++i)
				new_pageranks[i] = (new_pageranks[i] * DAMP) + ((1 - DAMP) / N) + ((danglingPG * DAMP) / N);
			
			error = get_error(pageranks, new_pageranks);
			pageranks = new_pageranks;
		}
		else
		{
			Send(&danglingPG, 1, DOUBLE, 0, 0, WORLD);
		}
		Broadcast(&error, 1, DOUBLE, 0, WORLD);
		danglingPG = 0.0;
	}
	delete mr;

	if(!rank_id)
		write_graph(filename + "-pr-mpi-base.txt");

	Finalize();
	return 0;
}