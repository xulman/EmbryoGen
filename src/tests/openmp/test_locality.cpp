#include <iostream>
#include <atomic>
#include <map>
#include <omp.h>
#include <thread>
#include <cstdlib>


void process_a(const int a, const int workerSignature)
{
	long waitingTime = 300*(long)std::rand()/RAND_MAX + 200;

	std::cout << "thread # " << workerSignature
	          << " works on a = " << a
	          << " for " << waitingTime << " millis\n";

	std::this_thread::sleep_for(std::chrono::milliseconds( waitingTime ));
}


int main(void)
{
	omp_set_num_threads(8);

	const int a_start = 0;
	const int a_stop = 20;

	//std::atomic<int> a(14);
	//std::atomic_int a(14);
	std::atomic_int c(a_start);
	int a = a_start;
	int b = a_start;

	#pragma omp parallel
	{
		int local_a;
		int local_c;

		local_c = c.fetch_add(1);

		#pragma omp critical
		local_a = ++a;
		++b;

		process_a(local_a,omp_get_thread_num());

	}

	std::cout << "a=" << a << "\n";
	std::cout << "b=" << b << "\n";

	return 0;
}
