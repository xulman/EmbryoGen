#include <iostream>
#include <functional>
#include <list>

class GeneralClass
{
public:
	void methodA()
	{ std::cout << "@" << (size_t)this << " methodA\n"; }

	void methodB()
	{ std::cout << "@" << (size_t)this << " methodB\n"; }

	void methodC(int qq)
	{ std::cout << "@" << (size_t)this << " methodC(" << qq << ")\n"; }
};


void callMethodB(std::list<GeneralClass*>& list)
{
	for (auto& c : list) c->methodB();
}

//-------------------------------------------------------------
template <class C>
void callNullaryMethod(const std::list<C*>& list,
                       const std::function< void(C*) >& m)
{
	for (auto& c : list) m(c);
}

template <class C,typename P>
void callUnaryMethod(const std::list<C*>& list,
                     const std::function< void(C*,P) >& m,
                     const P p)
{
	for (auto& c : list) m(c,p);
}

template <class C,typename P,typename Q>
void callBinaryMethod(const std::list<C*>& list,
                      const std::function< void(C*,P,Q) >& m,
                      const P p,
                      const Q q)
{
	for (auto& c : list) m(c,p,q);
}
//-------------------------------------------------------------


int main(void)
{
	std::list<GeneralClass*> list;
	list.push_back(new GeneralClass());
	list.push_back(new GeneralClass());
	list.push_back(new GeneralClass());

	callMethodB(list);
	std::cout << std::endl;

	callNullaryMethod<GeneralClass>(list, [](GeneralClass* c){ c->methodB(); } );
	std::cout << std::endl;

	callNullaryMethod<GeneralClass>(list, [](GeneralClass* c){ c->methodA(); } );
	std::cout << std::endl;

	callUnaryMethod<GeneralClass,int>(list, [](GeneralClass* c,int p){ c->methodC(p); }, 12 );
	std::cout << std::endl;

	callNullaryMethod<GeneralClass>(list, [](GeneralClass* c){ c->methodC(12); } );
	std::cout << std::endl;
}
