#include <iostream>
#include <atomic>
#include <vector>
#include <list>
#include <thread>
#include <cstdlib>
#include <omp.h>


class Job
{
public:
	Job() : processed(0), id(-1) {}
	Job(const Job& j) { processed.store( j.processed.load() ); id = j.id; }

	std::atomic<int> processed;
	int id;

	void touchMe() { processed.fetch_add(1); }
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
	std::list<Job> List;
	Job templateJob;
	for (int i=0; i < 20; ++i)
	{
		templateJob.id++;
		Vec.push_back(templateJob);
		List.push_back(templateJob);
	}

	#pragma omp parallel for
	for (auto it=Vec.begin(); it != Vec.end(); ++it)
	{
		it->touchMe();
		process_a(it->id,omp_get_thread_num());
	}


	std::cout << "=========================================\n";
	#pragma omp barrier
	std::cout << "=========================================\n";

	// DEBUG
	int iterCounts[50];
	int trueWorks[50];
	for (int i=0; i < 50; ++i) { iterCounts[i]=0; trueWorks[i]=0; }
	// DEBUG

	std::list<Job>::iterator it = List.begin();
	#pragma omp parallel shared(it)
	{
		/*
		std::cout << "Hi from thread " << omp_get_thread_num()
		  << " it@" << (size_t)&it
		  << " and local_it@" << (size_t)&local_it << "\n";
		*/

		bool doAdvance = true;
		std::list<Job>::iterator local_it;

		do
		{
			#pragma omp critical
			{
				//if the task is not yet over...
				if (it != List.end())
					//...do progress,...
					local_it = it++;
				else
					//...or just stay "relaxed"
					doAdvance = false;
			}

			if (doAdvance)
			{
				//local_it->reportMe();
				local_it->touchMe();
				process_a(local_it->id,omp_get_thread_num());
				trueWorks[omp_get_thread_num()]++;;
			}

			iterCounts[omp_get_thread_num()]++;;
		}
		while (doAdvance);
	}

	int workSum = 0;
	for (int i=0; i < numThreads; ++i)
	{
		std::cout << "iterCounts[" << i << "]=" << iterCounts[i] << ", ";
		std::cout << "trueWorks[" << i << "]=" << trueWorks[i] << "\n";
		workSum += trueWorks[i];
	}
	std::cout << "total amount of work done: " << workSum << "\n";


	std::cout << "=========================================\n";
	for (auto it=Vec.begin(); it != Vec.end(); ++it) it->reportMe();
	std::cout << "=========================================\n";
	for (auto it=List.begin(); it != List.end(); ++it) it->reportMe();

	return 0;
}
