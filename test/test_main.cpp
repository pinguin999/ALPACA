#include <catch2/catch_session.hpp>

// Not using Catch2::Catch2WithMain, because libschnacker.a also contains a main() which the linker
// would pick instead.
int main(int argc, char* argv[]) {
    return Catch::Session().run(argc, argv);
}
