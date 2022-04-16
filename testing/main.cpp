#include <iostream>

int foo(int x) {
	if (x < 10)
		return 10;

	return foo(x - 1) + x;
}

void bar() { std::cout << "Hello world\n"; }

int main() {
	bar();
	std::cout << foo(900) << "\n";
}
