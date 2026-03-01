//
// Created by Nikita Morozov on 24.02.2026.
//

#ifndef LAMAR_VERIFIER_HPP
#define LAMAR_VERIFIER_HPP

#include <fstream>
#include "disassembler.hpp"
#include "disassembler.hpp"
#include "diagnostics.hpp"
#include "runtime.hpp"
#include "utils.hpp"

#define MAX_FILE_SIZE (1u << 17)
#define MAX_VERIFIED_SIZE 0xFFFF

#define GET_HEIGHT(value) ((value) & ~(3u << 30))
#define MARK_JUMP_TARGET(value) ((value) | (1u << 30))
#define UNMARK_JUMP_TARGET(value) ((value) & ~(1u << 30))
#define IS_JUMP_TARGET(value) (((value) >> 30) & 0x1u)

namespace lamar {

    struct InstructionInfo {
        uint32_t offset = 0;
        uint32_t priority = 0;

        bool operator<(const InstructionInfo &other) const {
            if (priority == other.priority) {
                return offset > other.offset;
            }

            return priority < other.priority;
        };
    };

    struct ProcedureInfo {
        uint32_t offset = 0;
        uint32_t max_stack_size = 0;
    };

    class Verifier {
    public:
        Verifier(ByteFile &byte_file, const Disassembler &disassembler, auint *stack, void *verified);

        void verify();

    private:
        constexpr const static uint16_t UNVISITED_INSTRUCTION = 1u << 15;

        uint32_t instruction_length(uint32_t offset);

        void verify_cfg(uint32_t entry_point);

        void verify_instruction(uint32_t procedure_offset, uint32_t offset) const;

        uint32_t verify_public() const;

        std::pair<uint16_t, uint16_t> get_stack_usage(uint32_t offset) const;

        uint32_t read_uint(uint32_t offset) const;

        Disassembler disassembler_;
        uint16_t *verified_;
        util::Stack<uint32_t> procedure_stack_;
        ByteFile &byte_file_;
        std::ofstream ofs_{"/dev/null", std::ofstream::out | std::ofstream::app};
    };

}

#endif //LAMAR_VERIFIER_HPP
