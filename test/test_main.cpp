#include "ut_config.hpp"

int main() {
    return static_cast<int>(boost::ut::cfg<>.run()); // explicitly run registered test suites
}
