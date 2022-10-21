#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
using namespace std;

struct data
{
	string id;
	string doc;
	map<string, int> term_fre;
	long long int var;
};
data input[51];
pthread_t threads[51];
double ave[51];
bool count[51], var[51], finish[51];
map<string, int> all;
bool count_done, var_done, end_p;
long long int times, total;

void print_tid()
{
	cout << "[TID=" << pthread_self() << "] ";
}

void print_term_fre(long long int index)
{
	data *now = input + index; //get the space which the thread can use
	print_tid();
	cout << "DocID:" << now->id << " [";
	map<string, int>::iterator it = all.begin();
	now->var += now->term_fre[it->first] * now->term_fre[it->first];
	cout << now->term_fre[it->first];
	it++;
	for (; it != all.end(); it++)  //var will be used many times so save it
	{
		cout << "," << now->term_fre[it->first];
		now->var += now->term_fre[it->first] * now->term_fre[it->first];
	}
	var[index] = true;
	cout << "]\n";
}

void similarity(long long int index)
{
	ave[index] = 0;
	for (int i = 0; i < times; i++)
	{
		double cos = 0;
		if (i == index)
			continue;
		map<string, int>::iterator it = (input + i)->term_fre.begin();
		for (; it != (input + i)->term_fre.end(); it++)
			cos += input[index].term_fre[it->first] * input[i].term_fre[it->first];
		cos = cos / sqrt(input[index].var * input[i].var);
		print_tid();
		cout << "cosine(" << input[index].id << "," << input[i].id << ")=" << cos << endl;
		ave[index] += cos;
	}
	ave[index] /= (times - 1);
	print_tid();
	cout << "Avg_cosine: " << ave[index] << endl;
}

void *childTh(void *pos)
{
	struct timespec time_it = { 0, 0 };
	long long int index = (long long int)pos;
	data *now = input + index; //get the space which the thread can use
	stringstream buffer(now->doc);
	string temp;
	now->var = 0;
	while (buffer >> temp)
	{
		bool alpha = false;
		for (int i = 0; i < temp.size(); i++)
			if (!isalpha(temp[i]))
				alpha = true;
		if (alpha)
			continue;
		now->term_fre[temp]++; //std::map store trem_frequency
		all[temp]++;
	}
	count[index] = true; //this thread is done
	while (!count_done); //wait all threads
	print_term_fre(index);
	while (!var_done); //wait calculate var
	similarity(index);
	print_tid();
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time_it); //get cpu time
	long long int t = time_it.tv_nsec / 1000000.0;
	total += t;
	cout << "CPU time: " << t << "ms" << endl; //print cpu time 
	finish[index] = true;
	pthread_exit(NULL);
}

void* check(void* x)
{
	bool leave = false;
	while (!count_done && !leave) //check all of data has been store
	{
		leave = true;
		for (int i = 0; i < times; i++)
			if (!count[i])
				leave = false;
	}
	count_done = true;
	leave = false;
	while (!var_done && !leave)
	{
		leave = true;
		for (int i = 0; i < times; i++)
			if (!var[i])
				leave = false;
	}
	var_done = true;
	leave = false;
	while (!end_p && !leave)
	{
		leave = true;
		for (int i = 0; i < times; i++)
			if (!finish[i])
				leave = false;
	}
	end_p = true;
}

void find_key()
{
	double max = 0;
	int index;
	for (int i = 0; i < times; i++)
		if (ave[i] > max)
		{
			max = ave[i];
			index = i;
		}
	cout << "[Main thread] KeyDocID:" << input[index].id
		<< " Highest Average Cosine: " << max << endl;
}

void print_cpu_time()
{
	cout << "[Main thread] CPU time: " << total << "ms\n";
}

int main(int argc, char *argv[])
{
	total = times = 0;
	count_done = false;
	pthread_t term_done;
	ifstream infile;
	infile.open(argv[1]);
	while (getline(infile, input[times].id, '\n')) //get ID
	{
		getline(infile, input[times].doc, '\n'); //get document
		pthread_create(&threads[times], NULL, childTh, (void *)times); //creat thread
		cout << "[Main thread]: create TID:" << threads[times]
			<< ", DocID:" << input[times].id << endl; //show that the thread which main thread creates
		times++;
	}
	pthread_create(&term_done, NULL, check, (void*)times);
	while (!count_done);
	while (!var_done);
	while (!end_p);
	find_key();
	print_cpu_time();
	pthread_exit(NULL);
}