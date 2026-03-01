//
// Created by Nikita Morozov on 24.02.2026.
//

#ifndef LAMAR_VERIFIER_HPP
#define LAMAR_VERIFIER_HPP

#include <fstream>
#include <limits>
#include <vector>
#include "disassembler.hpp"
#include "diagnostics.hpp"
#include "runtime.hpp"
#include "utils.hpp"

#define MAX_FILE_SIZE (1u << 17)

namespace lamar {

    struct ProcedureInfo {
        uint32_t offset = 0;
        uint32_t max_stack_size = 0;
    };

    class Verifier {
    public:
        Verifier(ByteFile &byte_file, const Disassembler &disassembler);

        void verify();

    private:
        static constexpr uint16_t HEIGHT_UNKNOWN = std::numeric_limits<uint16_t>::max();

        uint32_t instruction_length(uint32_t offset);

        void verify_cfg(uint32_t entry_point);

        void verify_instruction(uint32_t offset) const;

        uint32_t verify_public() const;

        std::pair<uint32_t, uint32_t> get_stack_usage(uint32_t offset) const;

        uint32_t read_uint(uint32_t offset) const;

        void enqueue_instruction(uint32_t offset, uint32_t height);

        Disassembler disassembler_;
        std::vector<uint16_t> instruction_height_;
        std::vector<uint32_t> worklist_;
        std::vector<ProcedureInfo> procedure_stack_;
        ByteFile &byte_file_;
        std::ofstream ofs_{"/dev/null", std::ofstream::out | std::ofstream::app};
    };

}

#endif //LAMAR_VERIFIER_HPP
