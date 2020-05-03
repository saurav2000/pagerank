#include <mpi.h>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <cmath>
using namespace std;

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

int rank_id, procs, N;
map<int, vector<int> > graph;
double danglingPG = 0;
vector<double> pageranks;
vector<double> new_pageranks;
vector<double> zero_vector;
map<int, vector<double> > intermediates;
map<int, double> reduce_res;

void map_task(int key, vector<int> &value)
{
	intermediates[key].push_back(0);
	if(value.size() == 0)
		danglingPG += pageranks[key];
	for(int v: value)
		intermediates[v].push_back(pageranks[key] / value.size());
}

void reduce_task(int key, vector<double> &value)
{
	double res = 0.0;
	for(int i=0;i<value.size();++i)
		res += value[i];
	reduce_res[key] += res;
}

void read_graph(string filename)
{
	ifstream fin(filename);
	int x, y;
	while(fin >> x >> y)
	{
		graph[x]; graph[y];
		graph[x].push_back(y);
	}
	fin.close();
	N = graph.size();
	for(int i=0;i<N;++i)
	{
		pageranks.push_back(1.0 / N);
		new_pageranks.push_back(0);
		zero_vector.push_back(0);
	}
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
		zero_vector.push_back(0);
	}
}

void calc_part_pageranks()
{
	for(int i=rank_id; i<N; i+=procs)
		map_task(i, graph[i]);
	for(auto it=intermediates.begin(); it!=intermediates.end(); ++it)
		reduce_task(it->first, it->second);
}

double get_error(const vector<double> &v1, const vector<double> &v2)
{
	double res = 0.0;
	for(int i=0;i<v1.size();++i)
		res += abs(v1[i] - v2[i]);
	return res;
}

int main(int argc, char *argv[])
{
	Init(&argc, &argv);
	MPI_Comm_rank(WORLD, &rank_id);
	MPI_Comm_size(WORLD, &procs);
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
	int iter = 0;
	while(error > ERR_THRESHOLD)
	{
		for(int i=0;i<N;++i)
			Broadcast(&pageranks[i], 1, DOUBLE, 0, WORLD);
		calc_part_pageranks();
		if(!rank_id)
		{
			new_pageranks = zero_vector;
			// Dangling sum
			for(int c=1; c<procs; ++c)
			{
				double temp_sum;
				Recv(&temp_sum, 1, DOUBLE, c, 0, WORLD, &status);
				danglingPG += temp_sum;
			}
			// cout << "Dangling SUm " << danglingPG << "\n";
			// Pageranks from other processes
			for(int c=1; c<procs; ++c)
			{
				int size, index;
				double pg;
				Recv(&size, 1, INT, c, 0, WORLD, &status);
				for(int i=0;i<size;++i)
				{
					Recv(&index, 1, INT, c, 0, WORLD, &status);
					Recv(&pg, 1, DOUBLE, c, 0, WORLD, &status);
					new_pageranks[index] += pg;
				}
			}

			// pageranks of process 0
			for(auto it=reduce_res.begin(); it!=reduce_res.end(); ++it)
				new_pageranks[it->first] += it->second;

			for(int i=0;i<N;++i)
				new_pageranks[i] = (new_pageranks[i] * DAMP) + ((1 - DAMP) / N) + ((danglingPG * DAMP) / N);
			
			error = get_error(pageranks, new_pageranks);
			pageranks = new_pageranks;
		}
		else
		{
			Send(&danglingPG, 1, DOUBLE, 0, 0, WORLD);
			int size = reduce_res.size();
			Send(&size, 1, INT, 0, 0, WORLD);
			for(auto it=reduce_res.begin(); it!=reduce_res.end(); ++it)
			{
				int x = it->first;
				double y = it->second;
				Send(&x, 1, INT, 0, 0, WORLD);
				Send(&y, 1, DOUBLE, 0, 0, WORLD);
			}
		}
		Broadcast(&error, 1, DOUBLE, 0, WORLD);
		danglingPG = 0.0;
		intermediates.clear();
		reduce_res.clear();
	}

	if(!rank_id)
		write_graph(filename + "-pr-mpi.txt");
	
	Finalize();
	return 0;
}