//Standard Libs
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>

#include <boost/config.hpp>
#if defined(BOOST_MSVC)
#   pragma warning(disable: 4127)

// turn off checked iterators to avoid performance hit
#   if !defined(__SGI_STL_PORT)  &&  !defined(_DEBUG)
#       define _SECURE_SCL 0
#       define _HAS_ITERATOR_DEBUGGING 0
#   endif
#endif

#include "mr_cpp_lib/mapreduce.hpp"
using namespace std;

#define DAMP 0.85
#define ERR_THRESHOLD 1e-5

// Code starts here

map<int, vector<int> > graph;
int N = 0;
double danglingPG = 0;
vector<double> pageranks;
vector<double> new_pageranks;

template<typename MapTask>
class graphsource : mapreduce::detail::noncopyable
{
private:
	int _sequence;
	
public:
	graphsource() : _sequence(0) {}
	typedef typename MapTask::key_type KeyT;
	typedef typename MapTask::value_type ValueT;

	bool const setup_key(KeyT &key)
	{
		key = _sequence++;
		return _sequence <= N;
	}

	bool const get_data(KeyT const &key, ValueT &value)
	{
		vector<int> v = graph.at(key);
		value.clear();
		for(int i=0;i<v.size(); ++i)
			value.push_back(v[i]);

		return true;
	}
};

struct map_task: public mapreduce::map_task<int, vector<int> >
{
	template<typename Runner>
	void operator()(Runner &runner, const key_type &key, const value_type &value)
	{
		typedef typename Runner::reduce_task_type::key_type emit_key_type;
		runner.emit_intermediate(key, 0);
		if(value.size() == 0)
			danglingPG += pageranks[key];
		for(const int &v: value)
		{
			emit_key_type emit_key = v;
			runner.emit_intermediate(v, pageranks[key] / value.size());
		}
	}

};

struct reduce_task: public mapreduce::reduce_task<int, double>
{
	template<typename Runner, typename It>
	void operator()(Runner &runner, const key_type &key, It it, It ite)
	{
		if(it == ite)
			return;

		value_type result = 0.0;
		for(It it1=it; it1!=ite; ++it1)
			result += ((*it1) * DAMP);
		result += ((1 - DAMP) / N) + ((danglingPG * DAMP) / N);
		runner.emit(key, result);
	}
};

typedef mapreduce::job<map_task, 
						reduce_task, 
						mapreduce::null_combiner,
						graphsource<map_task> > pagerank_job;

double get_error(const vector<double> &v1, const vector<double> &v2)
{
	double res = 0.0;
	for(int i=0;i<v1.size();++i)
		res += abs(v1[i] - v2[i]);
	return res;
}

double sum(const vector<double> &v)
{
	double res = 0.0;
	for(int i=0;i<v.size();++i)
		res += v[i];
	return res;
}

void read_graph(string filename)
{
	ifstream fin;
	fin.open(filename);
	int x, y;
	while(fin >> x >> y)
	{
		graph[x]; graph[y];
		graph[x].push_back(y);
	}
	fin.close();
	N = graph.size();
	danglingPG = 0;
	for(int i=0; i<N; ++i)
		pageranks.push_back(1.0 / N);
	new_pageranks = pageranks;
}

int main(int argc, char const *argv[])
{
	string filename(argv[1]);
	read_graph(filename + ".txt");

	mapreduce::specification spec;
	spec.reduce_tasks = max(1U, thread::hardware_concurrency());

	double error = 1;
	int iter = 0;
	while(error > ERR_THRESHOLD)
	{
		pagerank_job::datasource_type data;
		pagerank_job job(data, spec);
		mapreduce::results result;
		job.run<mapreduce::schedule_policy::sequential<pagerank_job> >(result);
		int count = 0;
		for(auto it=job.begin_results(); it!=job.end_results(); ++it)
		{
			count++;
			new_pageranks[it->first] = it->second;
		}

		if(count != N)
			cout << count << " error\n";
		error = get_error(pageranks, new_pageranks);
		// cout << error << "\n";
		pageranks = new_pageranks;
		danglingPG = 0;
		++iter;
	}

	cout << iter << "\n";

	filename += "-pr-cpp.txt";
	ofstream fout(filename);
	for(int i=0;i<N;++i)
		fout << i << " = " << pageranks[i] << "\n";
	fout << "sum = " << sum(pageranks) <<"\n";
	fout.close();
	return 0;
}