#include <iostream>
#include <fstream>
#include <string>
#include "loader.hpp"
#include "disassembler.hpp"
#include "interpreter.hpp"

int main(int argc, char **argv) {
    auto print_usage = []() {
        std::cerr << "Usage: lamar  <bytecode-file>"
                  << '\n';
    };

    if (argc != 2) {
        std::cerr << "Exactly one argument is required" << '\n';
        print_usage();
        return 1;
    }

    std::string input_path = std::string(argv[1]);;

    if (input_path.empty()) {
        std::cerr << "Input bytecode file is required" << '\n';
        print_usage();
        return 1;
    }

    std::ifstream is(input_path, std::ios::binary | std::ios::in);
    if (!is) {
        std::cerr << "Failed to open input file: " << input_path << '\n';
        return 1;
    }

    lamar::Loader loader(is, input_path);
    auto file = loader.read_byte_file();

    lamar::Interpreter interpreter{std::move(file)};
    interpreter.interpret();


    return 0;
}
