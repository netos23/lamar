//
// Created by Nikita Morozov on 23.02.2026.
//

#include <iostream>
#include <fstream>
#include <string>
#include "loader.hpp"
#include "disassembler.hpp"

int main(int argc, char **argv) {
    auto print_usage = []() {
        std::cerr << "Usage: lamar_disassembler [--print-disassemble|--disassemble-file=<path>]  <bytecode-file>"
                  << '\n';
    };

    bool print_disassemble = false;
    std::string disassemble_file_path;
    std::string input_path;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--print-disassemble") {
            print_disassemble = true;
        } else if (arg.rfind("--disassemble-file=", 0) == 0) {
            disassemble_file_path = arg.substr(std::string("--disassemble-file=").size());
            if (disassemble_file_path.empty()) {
                std::cerr << "Missing path for --disassemble-file" << '\n';
                print_usage();
                return 1;
            }
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << '\n';
            print_usage();
            return 1;
        } else {
            if (!input_path.empty()) {
                std::cerr << "Too many positional arguments" << '\n';
                print_usage();
                return 1;
            }
            input_path = std::move(arg);
        }
    }

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

    lamar::Disassembler disassembler;

    if (!disassemble_file_path.empty()) {
        std::ofstream ofs(disassemble_file_path, std::ios::out | std::ios::trunc);
        if (!ofs) {
            std::cerr << "Failed to open disassemble output file: " << disassemble_file_path << '\n';
            return 1;
        }
        disassembler.disassemble(file, ofs);
    }

    if (print_disassemble) {
        disassembler.disassemble(file, std::cout);
    }


    return 0;
}