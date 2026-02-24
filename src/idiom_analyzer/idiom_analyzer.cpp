//
// Created by Nikita Morozov on 23.02.2026.
//

#include <vector>
#include <deque>
#include <unordered_map>
#include <cstring>
#include <utility>
#include <cstddef>
#include <iomanip>
#include <iterator>
#include <stdexcept>
#include "idiom_analyzer.hpp"
#include "diagnostics.hpp"


bool is_call(lamar::OpCode opcode) noexcept {
    switch (opcode) {
        case lamar::OpCode::CALL:
        case lamar::OpCode::CALLC:
            return true;

        default:
            return false;
    }
}

bool has_jump(lamar::OpCode opcode) noexcept {
    switch (opcode) {
        case lamar::OpCode::JMP:
        case lamar::OpCode::CJMPZ:
        case lamar::OpCode::CJMPNZ:
        case lamar::OpCode::CALL:
        case lamar::OpCode::CALLC:
            return true;

        default:
            return false;
    }
}

bool has_next(lamar::OpCode opcode) noexcept {
    switch (opcode) {
        case lamar::OpCode::JMP:
        case lamar::OpCode::END:
        case lamar::OpCode::RET:
        case lamar::OpCode::FAIL:
        case lamar::OpCode::EOF_:
            return false;

        default:
            return true;
    }
}

uint32_t lamar::IdiomAnalyzer::instruction_length(const lamar::ByteFile &byte_file, uint32_t offset) {
    return disassembler_.disassemble_instruction(byte_file, ofs_, offset);
}

uint32_t read_uint(const lamar::ByteFile &byte_file, uint32_t offset) {

    if (offset + sizeof(uint32_t) > byte_file.program_code.size()) {
        lamar::diagnostics::push_error_diagnostic("Index out of bounds, while read constant from code", offset);
        return 0;
    }

    uint32_t res = 0;
    std::memcpy(&res, byte_file.program_code.data() + offset, sizeof(uint32_t));

    return res;
}


void lamar::IdiomAnalyzer::analyze(const lamar::ByteFile &byte_file, std::ostream &output) {
    if(byte_file.program_code.size()  >= (1u << 31)) {
        diagnostics::push_error_diagnostic("Program code size exceeds 2GB limit", 0);
        return;
    }


    std::vector<bool> reachable(byte_file.program_code.size(), false);
    std::vector<bool> jump_targets(byte_file.program_code.size(), false);

    std::deque<uint32_t> queue;

    for (auto &[_, offset]: byte_file.public_symbol_table) {
        if (offset >= byte_file.program_code.size()) {
            diagnostics::push_error_diagnostic("Public symbol offset_ out of bounds", offset);
            break;
        }

        if (reachable[offset]) {
            continue;
        }

        reachable[offset] = true;
        jump_targets[offset] = true;
        queue.push_back(offset);
    }

    while (!queue.empty()) {
        auto offset = queue.back();
        queue.pop_back();

        auto opcode = byte_file.program_code[offset];
        auto length = instruction_length(byte_file, offset);

        if (has_jump(opcode)) {
            auto address = read_uint(byte_file, offset + sizeof(OpCode)); // skip operand

            if (address >= byte_file.program_code.size()) {
                diagnostics::push_error_diagnostic("Jump target out of bounds", address);
                continue;
            }

            jump_targets[address] = true;
            if (!reachable[address]) {
                reachable[address] = true;
                queue.push_back(address);
            }
        }

        if (has_next(opcode)) {
            auto address = offset + length;

            if (address >= byte_file.program_code.size()) {
                diagnostics::push_error_diagnostic("Next instruction out of bounds", offset);
                continue;
            }

            if (reachable[address]) {
                continue;
            }

            reachable[address] = true;
            queue.push_back(address);
        }
    }

    std::vector<std::pair<Idiom, uint32_t>> idiom_occurrences;

    uint32_t ip = 0;
    while (ip < byte_file.program_code.size()) {
        if (!reachable[ip]) {
            ip++;
            continue;
        }

        auto opcode = byte_file.program_code[ip];
        auto length = instruction_length(byte_file, ip);

        if (ip + length > byte_file.program_code.size()) {
            diagnostics::push_error_diagnostic("Not enough bytes for instruction", ip);
            return;
        }

        idiom_occurrences.emplace_back(ip, 0);

        if (has_next(opcode) && !is_call(opcode)) {
            size_t address = ip + length;

            if (address >= byte_file.program_code.size()) {
                diagnostics::push_error_diagnostic("Next instruction out of bounds", ip);
                ip += length;
                continue;
            }

            auto next_length = instruction_length(byte_file, address);

            if (address + next_length > byte_file.program_code.size()) {
                diagnostics::push_error_diagnostic("Not enough bytes for instruction", address);
                ip += length;
                continue;
            }

            if (reachable[address] && !jump_targets[address]) {
                idiom_occurrences.emplace_back(MARK_NEXT_IDIOM(ip), 0);
            }
        }

        ip += length;
    }

    if (idiom_occurrences.empty()) {
        output << "No idioms found." << std::endl;
        return;
    }

    auto idiom_length = [&](Idiom idiom) -> uint32_t {
        auto base = instruction_length(byte_file, IDIOM_OFFSET(idiom));

        if (HAS_NEXT_IDIOM(idiom)) {
            auto next_length = instruction_length(byte_file, IDIOM_OFFSET(idiom) + base);
            return base + next_length;
        }

        return base;
    };

    auto cmp_idioms = [&](const auto lhs, const auto rhs) -> int {
        auto lhs_length = idiom_length(lhs);
        auto rhs_length = idiom_length(rhs);

        if (lhs_length != rhs_length) {
            return lhs_length - rhs_length;
        }

        auto lhs_begin = byte_file.program_code.data() + IDIOM_OFFSET(lhs);
        auto rhs_begin = byte_file.program_code.data() + IDIOM_OFFSET(rhs);

        return std::memcmp(lhs_begin, rhs_begin, lhs_length);
    };

    auto print_idiom = [&](Idiom idiom) {
        disassembler_.disassemble_instruction(byte_file, output, IDIOM_OFFSET(idiom));
        if (HAS_NEXT_IDIOM(idiom)) {
            disassembler_.disassemble_instruction(
                    byte_file,
                    output,
                    IDIOM_OFFSET(idiom) + instruction_length(byte_file, IDIOM_OFFSET(idiom))
            );
        }
    };


    std::sort(
            idiom_occurrences.begin(), idiom_occurrences.end(),
            [&](const auto &a, const auto &b) -> bool {
                auto lhs = a.first;
                auto rhs = b.first;

                return cmp_idioms(lhs, rhs) < 0;
            }
    );

    auto inserter = idiom_occurrences.begin();

    for (auto &idiom_occurrence: idiom_occurrences) {
        if (cmp_idioms(idiom_occurrence.first, inserter->first) == 0) {
            inserter->second++;
        } else {
            inserter++;
            *inserter = idiom_occurrence;
            inserter->second++;
        }
    }

    idiom_occurrences.erase(inserter + 1, idiom_occurrences.end());
    std::sort(
            idiom_occurrences.begin(), idiom_occurrences.end(),
            [&](const auto &a, const auto &b) -> bool {
                return a.second > b.second;
            }
    );


    for (auto &[idiom, occurrences]: idiom_occurrences) {
        output << "Occurrences: " << occurrences << "\n";
        print_idiom(idiom);
        output << "\n\n";

    }
}

lamar::IdiomAnalyzer::IdiomAnalyzer(const lamar::Disassembler &disassembler) : disassembler_(disassembler) {}

