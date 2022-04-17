#include "../src/util/tools.hpp"
#include <iostream>
#include <string>

class Person {
	std::string _name;

  public:
	Person(std::string name) : _name(std::move(name)) {}
	const std::string& name() { return _name; }
};

int main() {
	tools::structures::StaticVector<Person, 3> vec;

	vec.push_back(Person("Jaja"));
	vec.push_back(Person("Paja"));

	for (auto val : vec)
		std::cout << val.name() << " ";

	std::cout << '\n';
}
