//
// Created by Nikita Morozov on 05.01.2026.
//

#ifndef LAMAR_LOADER_HPP
#define LAMAR_LOADER_HPP

#include <istream>
#include <memory>
#include "bytecode.hpp"

namespace lamar {

    class Loader {
    public:
        ByteFile read_byte_file();
    private:

        int32_t read_int32_t();
        uint32_t read_uint32_t();
        void read_bytes(char *buf, std::streamsize size);
        std::vector<OpCode> read_bytecode();

        std::istream input_stream;
    };
}
#endif //LAMAR_LOADER_HPP
