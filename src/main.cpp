#include <iostream>
#include <fstream>
#include "loader.hpp"

int main(int arc, char** argv) {

    std::ifstream is(argv[1], std::ios::binary | std::ios::in);
    lamar::Loader loader(is);
    auto file = loader.read_byte_file();
    return 0;
}
