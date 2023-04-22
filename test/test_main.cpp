#include <boost/ut.hpp>

int main() {
	return boost::ut::cfg<>.run(); // explicitly run registered test suites
}
