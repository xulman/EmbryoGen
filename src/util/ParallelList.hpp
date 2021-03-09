#include <list>
#include <functional>

template <class C>
void visitEveryObject(std::list<C>& listOfObjs,
                      const std::function< void(C) >& visitingFun)
{
	typedef typename std::list<C>::iterator it_t;
	it_t it = listOfObjs.begin();

	#pragma omp parallel shared(it)
	{
		bool doAdvance = true;
		it_t local_it;

		do
		{
			#pragma omp critical
			{
				//if the list is not yet fully processed...
				if (it != listOfObjs.end())
					//...progress on next item,...
					local_it = it++;
				else
					//...or just stay "relaxed"
					doAdvance = false;
			}

			if (doAdvance) visitingFun(*local_it);
		}
		while (doAdvance);
	}
}
