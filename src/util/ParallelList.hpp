#include <functional>
#include <list>
#include <map>
#include "report.h"

#ifdef _OPENMP
	#define RUN_MODE " running in parallel"
#else
	#define RUN_MODE " running sequentially"
#endif

template <class CONTAINER_OF_ITEMS,typename ITEM_TYPE>
void visitEveryObject(typename CONTAINER_OF_ITEMS::iterator listOfObjs_iter,
                      typename CONTAINER_OF_ITEMS::iterator listOfObjs_end,
                      const std::function< void(ITEM_TYPE&) >& visitingFun)
{
	DEBUG_REPORT("RW workhorse" << RUN_MODE);
	#pragma omp parallel shared(listOfObjs_iter)
	{
		bool keepAdvancing = true;
		typename CONTAINER_OF_ITEMS::iterator local_it;

		do
		{
			#pragma omp critical
			{
				//if the list is not yet fully processed...
				if (listOfObjs_iter != listOfObjs_end)
					//...progress on next item,...
					local_it = listOfObjs_iter++;
				else
					//...or just stay "relaxed"
					keepAdvancing = false;
			}

			if (keepAdvancing) visitingFun(*local_it);
		}
		while (keepAdvancing);
	}
}

template <class CONTAINER_OF_ITEMS,typename ITEM_TYPE>
void visitEveryObject(typename CONTAINER_OF_ITEMS::const_iterator listOfObjs_iter,
                      typename CONTAINER_OF_ITEMS::const_iterator listOfObjs_end,
                      const std::function< void(const ITEM_TYPE&) >& visitingFun)
{
	DEBUG_REPORT("RO workhorse" << RUN_MODE);
	#pragma omp parallel shared(listOfObjs_iter)
	{
		bool keepAdvancing = true;
		typename CONTAINER_OF_ITEMS::const_iterator local_it;

		do
		{
			#pragma omp critical
			{
				//if the list is not yet fully processed...
				if (listOfObjs_iter != listOfObjs_end)
					//...progress on next item,...
					local_it = listOfObjs_iter++;
				else
					//...or just stay "relaxed"
					keepAdvancing = false;
			}

			if (keepAdvancing) visitingFun(*local_it);
		}
		while (keepAdvancing);
	}
}


template <typename C>
void visitEveryObject(std::list<C>& listOfObjs,
                      const std::function< void(C&) >& visitingFun)
{
	DEBUG_REPORT("RW list-wrapper" << RUN_MODE);
	visitEveryObject<std::list<C>,C>(
			listOfObjs.begin(),
			listOfObjs.end(),
			visitingFun
		);
}

template <typename C>
void visitEveryObject_const(const std::list<C>& listOfObjs,
                            const std::function< void(const C&) >& visitingFun)
{
	DEBUG_REPORT("RO list-wrapper" << RUN_MODE);
	visitEveryObject<std::list<C>,C>(
			listOfObjs.begin(),
			listOfObjs.end(),
			visitingFun
		);
}


template <typename K,typename V>
void visitEveryObject(std::map<K,V>& mapOfObjs,
                      const std::function< void(V&) >& visitingFun)
{
	DEBUG_REPORT("RW map-value-wrapper" << RUN_MODE);
	visitEveryObject< std::map<K,V>,std::pair<const K,V> >(
			mapOfObjs.begin(),
			mapOfObjs.end(),
			[&visitingFun](std::pair<const K,V>& item){ visitingFun(item.second); }
		);
}

template <typename K,typename V>
void visitEveryObject_const(const std::map<K,V>& mapOfObjs,
                            const std::function< void(const V&) >& visitingFun)
{
	DEBUG_REPORT("RO map-value-wrapper" << RUN_MODE);
	visitEveryObject< std::map<K,V>,std::pair<const K,V> >(
			mapOfObjs.cbegin(),
			mapOfObjs.cend(),
			[&visitingFun](const std::pair<const K,V>& item){ visitingFun(item.second); }
		);
}
