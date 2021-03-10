#include <functional>
#include <list>
#include <map>
#include "report.h"

#ifdef _OPENMP
	#include <omp.h>
	#define RUN_MODE "running in parallel"
#else
	#define RUN_MODE "running sequentially"
	int omp_get_max_threads() { return 1; }
#endif

/**
 * This is a summary of params that influence how visitEveryObject*()
 * functions shall behave. One can adjust the params here...
 *
 * The plan is that a certain TaskSize (a summary number of tasks that
 * need to be computed) is split among 'numThreads' workers that work
 * in parallel. The workers share the load equally, yielding a certain
 * (ideal) number of tasks in total per worker. This number is additionally
 * divided by 'wantedRoundsNum', or further reduced to make sure the
 * number of tasks per worker is for sure below 'maxThreadLoad'.
 *
 * We are not using a fixed, pre-planned splitting of the TaskSize.
 * Instead, each worker comes to take 'groupSize' (see visitEveryObject())
 * number of tasks and, after it is done, it asks for another 'groupSize'.
 * If one worker works a lot faster than the others, this schema allows it
 * to take more in total (compared to waiting for the slower ones in the
 * pre-planned model). A compromise between not having too small 'groupSizes'
 * (and waiting for critical sections too often) on one hand and not doing its
 * fair-share uninterrupted in one go on the other hand (having no ability to
 * load balance), we plan the workers to interrupt their work 'wantedRoundsNum'-times.
 * It could, however, create still way too large 'groupSizes' (an "atomic" block
 * of work that could possibly take way too long of wall-time) so we limit its
 * size to 'maxThreadLoad'.
 *
 * The 'numThreads' above is a maximum between the currently available threads
 * in the system (as reported by the openMP itself) and user preference given
 * in 'maxNumThreads' below.
 */
struct VisitingPolicy
{
public:
	/** max number of workers (threads) to allow to work in parallel */
	static const int maxNumThreads = 64;

	/** in how many portions should a fair-share of work of one worker be be split into */
	static const int wantedRoundsNum = 3;

	/** no work of one worker should consists of more than this number of jobs */
	static const int maxThreadLoad = 10000;

	static int getGroupSizeOptimalToThisTaskSize(const int taskSize)
	{
		int numThreads = omp_get_max_threads();
		if (numThreads > maxNumThreads) numThreads = maxNumThreads;

		int fairShare = taskSize / (numThreads*wantedRoundsNum)  +1;
		return (fairShare < maxThreadLoad ? fairShare : maxThreadLoad);
	}
};


template <class CONTAINER_OF_ITEMS,typename ITEM_TYPE>
void visitEveryObject(typename CONTAINER_OF_ITEMS::iterator listOfObjs_iter,
                      typename CONTAINER_OF_ITEMS::iterator listOfObjs_end,
                      const std::function< void(ITEM_TYPE&) >& visitingFun,
                      int groupSize = 500)
{
	if (groupSize <= 0)
	{
		groupSize = VisitingPolicy::maxThreadLoad;
		REPORT("No auto-adjustment of load is available, using default groupSize.");
	}

	DEBUG_REPORT("RW workhorse " << RUN_MODE << " with groupSize=" << groupSize);
	#pragma omp parallel shared(listOfObjs_iter)
	{
		bool keepAdvancing = true;
		int howManyToYetCompute = 0;
		typename CONTAINER_OF_ITEMS::iterator local_it;

		do
		{
			#pragma omp critical
			{
				//"reserve" the upcoming group of potential jobs for us
				//by remembering the beginning of it
				local_it = listOfObjs_iter;

				//"mark" beginning of some next group of potential jobs
				//by advancing the global shared iterator
				while (listOfObjs_iter != listOfObjs_end && howManyToYetCompute < groupSize)
				{
					++listOfObjs_iter;
					++howManyToYetCompute;
				}

				//flag if it is worth looking into another round of jobs
				//(after the current/upcoming group of jobs is done)
				keepAdvancing = listOfObjs_iter != listOfObjs_end;
			}

			while (howManyToYetCompute > 0)
			{
				visitingFun(*local_it);
				++local_it;
				--howManyToYetCompute;
			}
		}
		while (keepAdvancing);
	}
}


template <class CONTAINER_OF_ITEMS,typename ITEM_TYPE>
void visitEveryObject(typename CONTAINER_OF_ITEMS::const_iterator listOfObjs_iter,
                      typename CONTAINER_OF_ITEMS::const_iterator listOfObjs_end,
                      const std::function< void(const ITEM_TYPE&) >& visitingFun,
                      int groupSize = 500)
{
	if (groupSize <= 0)
	{
		groupSize = VisitingPolicy::maxThreadLoad;
		REPORT("No auto-adjustment of load is available, using default groupSize.");
	}

	DEBUG_REPORT("RO workhorse " << RUN_MODE << " with groupSize=" << groupSize);
	#pragma omp parallel shared(listOfObjs_iter)
	{
		bool keepAdvancing = true;
		int howManyToYetCompute = 0;
		typename CONTAINER_OF_ITEMS::const_iterator local_it;

		do
		{
			#pragma omp critical
			{
				//"reserve" the upcoming group of potential jobs for us
				//by remembering the beginning of it
				local_it = listOfObjs_iter;

				//"mark" beginning of some next group of potential jobs
				//by advancing the global shared iterator
				while (listOfObjs_iter != listOfObjs_end && howManyToYetCompute < groupSize)
				{
					++listOfObjs_iter;
					++howManyToYetCompute;
				}

				//flag if it is worth looking into another round of jobs
				//(after the current/upcoming group of jobs is done)
				keepAdvancing = listOfObjs_iter != listOfObjs_end;
			}

			while (howManyToYetCompute > 0)
			{
				visitingFun(*local_it);
				++local_it;
				--howManyToYetCompute;
			}
		}
		while (keepAdvancing);
	}
}


template <typename C>
void visitEveryObject(std::list<C>& listOfObjs,
                      const std::function< void(C&) >& visitingFun,
                      const int groupSize = -1)
{
	DEBUG_REPORT("RW list-wrapper " << RUN_MODE);
	visitEveryObject<std::list<C>,C>(
			listOfObjs.begin(),
			listOfObjs.end(),
			visitingFun,
			groupSize > 0 ? groupSize
			              : VisitingPolicy::getGroupSizeOptimalToThisTaskSize((int)listOfObjs.size())
		);
}

template <typename C>
void visitEveryObject_const(const std::list<C>& listOfObjs,
                            const std::function< void(const C&) >& visitingFun,
                            const int groupSize = -1)
{
	DEBUG_REPORT("RO list-wrapper " << RUN_MODE);
	visitEveryObject<std::list<C>,C>(
			listOfObjs.begin(),
			listOfObjs.end(),
			visitingFun,
			groupSize > 0 ? groupSize
			              : VisitingPolicy::getGroupSizeOptimalToThisTaskSize((int)listOfObjs.size())
		);
}


template <typename K,typename V>
void visitEveryObject(std::map<K,V>& mapOfObjs,
                      const std::function< void(V&) >& visitingFun,
                      const int groupSize = -1)
{
	DEBUG_REPORT("RW map-value-wrapper " << RUN_MODE);
	visitEveryObject< std::map<K,V>,std::pair<const K,V> >(
			mapOfObjs.begin(),
			mapOfObjs.end(),
			[&visitingFun](std::pair<const K,V>& item){ visitingFun(item.second); },
			groupSize > 0 ? groupSize
			              : VisitingPolicy::getGroupSizeOptimalToThisTaskSize((int)mapOfObjs.size())
		);
}

template <typename K,typename V>
void visitEveryObject_const(const std::map<K,V>& mapOfObjs,
                            const std::function< void(const V&) >& visitingFun,
                            const int groupSize = -1)
{
	DEBUG_REPORT("RO map-value-wrapper " << RUN_MODE);
	visitEveryObject< std::map<K,V>,std::pair<const K,V> >(
			mapOfObjs.cbegin(),
			mapOfObjs.cend(),
			[&visitingFun](const std::pair<const K,V>& item){ visitingFun(item.second); },
			groupSize > 0 ? groupSize
			              : VisitingPolicy::getGroupSizeOptimalToThisTaskSize((int)mapOfObjs.size())
		);
}
