//
// Created by Nikita Morozov on 25.01.2026.
//

#ifndef LAMAR_DISASSEMBLER_HPP
#define LAMAR_DISASSEMBLER_HPP

#include <ostream>
#include "bytecode.hpp"

namespace lamar {
    class Disassembler {

    public:
        void disassemble(const ByteFile &byte_file, std::ostream &output) const;
        uint32_t disassemble_instruction(const ByteFile &byte_file, std::ostream &output, uint32_t offset) const;
    };
}

#endif //LAMAR_DISASSEMBLER_HPP
