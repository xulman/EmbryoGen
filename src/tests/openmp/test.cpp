#include <iostream>
#include <atomic>
#include <map>
#include <omp.h>


void process_a(const int a, const int workerSignature)
{
	std::cout << "thread # " << workerSignature
	          << " works on a = " << a << "\n";
	//random delay here
}


int main(void)
{
	omp_set_num_threads(5);

	const int a_start = 14;
	const int a_stop = 24;

	//std::atomic<int> a(14);
	//std::atomic_int a(14);
	int a = a_start;
	std::cout << "common start, a = " << a << "\n";

	std::map<int,int> whoSawWhat;

	#pragma omp parallel
	{
		const int workerSignature = omp_get_thread_num();
		int local_a;
		do
		{
			#pragma omp critical
			{
				//get next free 'a'
				local_a = ++a;
				std::cout << ">>thread # " << workerSignature
				          << " considers a = " << a << "<<\n";
				whoSawWhat[local_a] = workerSignature;
			}

			//process only valid items
			if (local_a < a_stop)
				process_a(local_a,workerSignature);
		} while (local_a < a_stop);
	}

	std::cout << "common end, a = " << a << "\n";

	//std::cout << "who saw what (thread ID -> value processed):\n";
	std::cout << "who saw what (value done -> by thread ID):\n";
	for (const auto& m : whoSawWhat)
		std::cout << m.first << " -> " << m.second << "\n";

	if (whoSawWhat.size() != (a_stop-a_start))
		std::cout << "stats: " << whoSawWhat.size()
		          << " vs. " << (a_stop-a_start) << "\n";

	return 0;
}
