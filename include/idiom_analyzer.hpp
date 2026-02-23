//
// Created by Nikita Morozov on 23.02.2026.
//

#ifndef LAMAR_IDIOM_ANALYZER_HPP
#define LAMAR_IDIOM_ANALYZER_HPP

#include <functional>
#include <ostream>
#include "bytecode.hpp"
#include "disassembler.hpp"

namespace lamar {
    class Idiom {
    public:
        Idiom(uint32_t offset, uint32_t length, const ByteFile &byte_file);
        Idiom(const Idiom &other) noexcept;
        Idiom(Idiom &&other) noexcept;
        bool operator==(const Idiom &other) const noexcept;
        Idiom &operator=(const Idiom &other) noexcept;
        Idiom &operator=(Idiom &&other) noexcept;

        uint32_t get_offset() const;

        uint32_t get_length() const;

    private:
        uint32_t offset_;
        uint32_t length_;
        const ByteFile &byte_file_;
        friend struct std::hash<lamar::Idiom>;
    };


    class IdiomAnalyzer {
    public:
        explicit IdiomAnalyzer(const Disassembler &disassembler);

        void analyze(const ByteFile &byte_file, std::ostream &output) const;

    private:
       Disassembler disassembler_;
    };
}

namespace std {
    template<>
    struct hash<lamar::Idiom> {
        size_t operator()(const lamar::Idiom &idiom) const noexcept;
    };
}


#endif //LAMAR_IDIOM_ANALYZER_HPP
