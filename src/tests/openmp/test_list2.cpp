#include <iostream>
#include <atomic>
#include <vector>
#include <list>
#include <map>
#include <thread>
#include <cstdlib>
#include <omp.h>

#define DEBUG_REPORT(x) std::cout << x << "\n";
#include "/home/ulm0023/devel/EmbryoGen_PM/src/util/ParallelList.hpp"


class Job
{
public:
	Job() : processed(0), id(-1) {}
	Job(const Job& j) { processed.store( j.processed.load() ); id = j.id; }

	std::atomic<int> processed;
	int id;

	void touchMe() { processed.fetch_add(1); }
	void touchMeFake() const { }
	void reportMe()
	{
		std::cout << "job " << id << " @" << (size_t)this
		  << " was processed " << processed << "\n";
	}
};


void process_a(const int a, const int workerSignature)
{
	long waitingTime = 300*(long)std::rand()/RAND_MAX + 200;

	std::cout << "thread # " << workerSignature
	          << " works on a = " << a
	          << " for " << waitingTime << " millis\n";

	std::this_thread::sleep_for(std::chrono::milliseconds( waitingTime ));
}


#ifndef _OPENMP
void omp_set_num_threads(int numThreads) {}
int omp_get_thread_num() { return 0; }
#endif


int main(void)
{
#ifdef _OPENMP
	std::cout << "OpenMP present!\n";
#else
	std::cout << "OpenMP absent!\n";
#endif

	const int numThreads = 6;
	omp_set_num_threads(numThreads);

	//std::list<int> List  { 10, 20, 30, 40, 50, 60, 70, 80, 15,25,35,45,55,65,75,85,90};
	//std::vector<int> Vec { 10, 20, 30, 40, 50, 60, 70, 80, 15,25,35,45,55,65,75,85,90};
	std::vector<Job> Vec;
	std::list<Job*> List;
	std::map<int,Job*> Map;
	Job templateJob;
	for (int i=0; i < 20; ++i)
	{
		templateJob.id++;
		Vec.push_back(templateJob);
		Job* j = new Job(); j->id = templateJob.id;
		List.push_back(j);
		Map[templateJob.id] = j;
	}

	/*
	#pragma omp parallel for
	for (auto it=Vec.begin(); it != Vec.end(); ++it)
	{
		it->touchMe();
		process_a(it->id,omp_get_thread_num());
	}
	*/


	std::cout << "=========================================\n";
	#pragma omp barrier
	std::cout << "=========================================\n";

	// DEBUG
	int iterCounts[50];
	int trueWorks[50];
	for (int i=0; i < 50; ++i) { iterCounts[i]=0; trueWorks[i]=0; }
	// DEBUG

	visitEveryObject<Job*>(List, [&trueWorks](Job*& j) {
			j->touchMe();
			process_a(j->id,omp_get_thread_num());
			trueWorks[omp_get_thread_num()]++;;
		} );

	visitEveryObject<std::list<Job*>,Job*>(List.cbegin(),List.cend(),
	                                       [&trueWorks](Job* const & j) {
			j->touchMe();
			process_a(j->id,omp_get_thread_num());
			trueWorks[omp_get_thread_num()]++;;
		} );

	visitEveryObject< std::map<int,Job*>, std::pair<const int,Job*> >(
					Map.begin(),Map.end(),
					[&trueWorks](auto p) { // auto = std::pair<const int,Job*>&
			p.second->touchMe();
			process_a(p.second->id,omp_get_thread_num());
			trueWorks[omp_get_thread_num()]++;;
		} );

	visitEveryObject_const<int,Job*>(Map, [&trueWorks](Job* const & p) {
			p->touchMe();
			process_a(p->id,omp_get_thread_num());
			trueWorks[omp_get_thread_num()]++;;
		} );

	int workSum = 0;
	for (int i=0; i < numThreads; ++i)
	{
		std::cout << "iterCounts[" << i << "]=" << iterCounts[i] << ", ";
		std::cout << "trueWorks[" << i << "]=" << trueWorks[i] << "\n";
		workSum += trueWorks[i];
	}
	std::cout << "total amount of work done: " << workSum << "\n";


	//std::cout << "=========================================\n";
	//for (auto it=Vec.begin(); it != Vec.end(); ++it) it->reportMe();
	std::cout << "=========================================\n";
	for (auto it=List.begin(); it != List.end(); ++it) (*it)->reportMe();

	return 0;
}
