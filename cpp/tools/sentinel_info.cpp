// Prints build facts. Used by scripts/verify_env.sh and later embedded in
// benchmark output.
#include <iostream>

#include "sentinel/build_info.hpp"

int main() {
    const auto info = sentinel::build_info();
    std::cout << "sentinel version : " << info.version << '\n'
              << "build type       : " << info.build_type << '\n'
              << "compiler         : " << info.compiler << '\n'
              << "__cplusplus      : " << info.cpp_standard << '\n'
              << "NEON enabled     : " << (info.neon ? "yes" : "no") << '\n';
    return 0;
}
