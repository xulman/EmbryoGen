#include "../concepts.hpp"
#include "StaticVector.hpp"

template <typename T>
requires tools::concepts::basic_container<T>
void foo(T) {}

int main() {
	tools::structures::StaticVector4<int> x;
	foo(x);
}
