//
// Created by Nikita Morozov on 23.02.2026.
//

#ifndef LAMAR_IDIOM_ANALYZER_HPP
#define LAMAR_IDIOM_ANALYZER_HPP

#include <functional>
#include <ostream>
#include <fstream>
#include "bytecode.hpp"
#include "disassembler.hpp"

#define HAS_NEXT_IDIOM(idiom) (((idiom) & 0x80000000u) != 0u)
#define IDIOM_OFFSET(idiom) ((idiom) & 0x7FFFFFFFu)
#define MARK_NEXT_IDIOM(idiom) ((idiom) | 0x80000000u)


namespace lamar {
    typedef uint32_t Idiom;


    class IdiomAnalyzer {
    public:
        explicit IdiomAnalyzer(const Disassembler &disassembler);

        void analyze(const ByteFile &byte_file, std::ostream &output);

        uint32_t instruction_length(const lamar::ByteFile &byte_file, uint32_t offset);

    private:
        std::ofstream ofs_{"/dev/null", std::ofstream::out | std::ofstream::app};
        Disassembler disassembler_;
    };
}


#endif //LAMAR_IDIOM_ANALYZER_HPP
