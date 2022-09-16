#include "../src/util/report.hxx"

int main() {
	using namespace std::string_literals;
	std::string xxd("This is completely valid string");

	report::message(xxd);
}
