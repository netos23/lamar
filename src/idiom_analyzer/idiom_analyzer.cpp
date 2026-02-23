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

uint8_t instruction_length(lamar::OpCode opcode) noexcept {
    switch (opcode) {
        case lamar::ADD:
        case lamar::SUB:
        case lamar::MUL:
        case lamar::DIV:
        case lamar::MOD:
        case lamar::LT:
        case lamar::LE:
        case lamar::GT:
        case lamar::GE:
        case lamar::EQ:
        case lamar::NE:
        case lamar::AND:
        case lamar::OR:
            return sizeof(lamar::OpCode);
        case lamar::CONST:
            return sizeof(lamar::OpCode) + sizeof(uint32_t);
        case lamar::STRING:
            return sizeof(lamar::OpCode) + sizeof(uint32_t);
        case lamar::SEXP:
            return sizeof(lamar::OpCode) + 2 * sizeof(uint32_t);
        case lamar::STI:
        case lamar::STA:
            return sizeof(lamar::OpCode);
        case lamar::JMP:
            return sizeof(lamar::OpCode) + sizeof(uint32_t);
        case lamar::END:
        case lamar::RET:
        case lamar::DROP:
        case lamar::DUP:
        case lamar::SWAP:
        case lamar::ELEM:
            return sizeof(lamar::OpCode);
        case lamar::LD_G:
        case lamar::LD_L:
        case lamar::LD_A:
        case lamar::LD_C:
        case lamar::LDA_G:
        case lamar::LDA_L:
        case lamar::LDA_A:
        case lamar::LDA_C:
        case lamar::ST_G:
        case lamar::ST_L:
        case lamar::ST_A:
        case lamar::ST_C:
        case lamar::CJMPZ:
        case lamar::CJMPNZ:
            return sizeof(lamar::OpCode) + sizeof(uint32_t);
        case lamar::BEGIN:
        case lamar::CBEGIN:
        case lamar::CLOSURE:
            return sizeof(lamar::OpCode) + 2 * sizeof(uint32_t);
        case lamar::CALLC:
            break;
        case lamar::CALL:
        case lamar::TAG:
            return sizeof(lamar::OpCode) + 2 * sizeof(uint32_t);
        case lamar::ARRAY:
            return sizeof(lamar::OpCode) + sizeof(uint32_t);
        case lamar::FAIL:
            return sizeof(lamar::OpCode) + 2 * sizeof(uint32_t);
        case lamar::LINE:
            return sizeof(lamar::OpCode) + sizeof(uint32_t);
        case lamar::PATTEQSTR:
        case lamar::PATTSTRING:
        case lamar::PATTARRAY:
        case lamar::PATTSEXP:
        case lamar::PATTREF:
        case lamar::PATTVAL:
        case lamar::PATTFUN:
        case lamar::CALL_LREAD:
        case lamar::CALL_LWRITE:
        case lamar::CALL_LLENGTH:
        case lamar::CALL_LSTRING:
            return sizeof(lamar::OpCode);
        case lamar::CALL_BARRAY:
            return sizeof(lamar::OpCode) + sizeof(uint32_t);
        case lamar::EOF_:
            return sizeof(lamar::OpCode);
    }

    return -1;
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


void lamar::IdiomAnalyzer::analyze(const lamar::ByteFile &byte_file, std::ostream &output) const {
    std::vector<bool> reachable(byte_file.program_code.size(), false);
    std::vector<bool> jump_targets(byte_file.program_code.size(), false);

    std::deque<uint32_t> queue;

    for (auto &[_, offset]: byte_file.public_symbol_table) {
        if (offset >= byte_file.program_code.size()) {
            diagnostics::push_error_diagnostic("Public symbol offset_ out of bounds", offset);
            break;
        }

        reachable[offset] = true;
        jump_targets[offset] = true;
        queue.push_back(offset);
    }

    while (!queue.empty()) {
        auto offset = queue.back();
        queue.pop_back();

        auto opcode = byte_file.program_code[offset];

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
            auto address = offset + instruction_length(opcode);
            reachable[address] = true;
            queue.push_back(address);
        }
    }

    std::unordered_map<Idiom, uint32_t> idiom_occurrences;

    auto make_idiom = [&idiom_occurrences, &byte_file](uint32_t offset, uint32_t length) {
        auto idiom = Idiom(offset, length, byte_file);
        if (auto it = idiom_occurrences.find(idiom); it != idiom_occurrences.end()) {
            it->second += 1;
        } else {
            idiom_occurrences.emplace(idiom, 1);
        }
    };

    uint32_t ip = 0;
    while (ip < byte_file.program_code.size()) {
        if (!reachable[ip]) {
            ip++;
            continue;
        }

        auto opcode = byte_file.program_code[ip];
        auto length = instruction_length(opcode);
        if (ip + length >= byte_file.program_code.size()) {
            diagnostics::push_error_diagnostic("Not enough bytes for instruction", ip);
            return;
        }

        make_idiom(ip, length);

        if (has_next(opcode) && !is_call(opcode)) {
            size_t address = ip + length;
            auto next_opcode = byte_file.program_code[address];

            if (reachable[next_opcode] && !jump_targets[address]) {
                make_idiom(ip, length + instruction_length(next_opcode));
            }
        }

        ip += length;
    }

    if (idiom_occurrences.empty()) {
        output << "No idioms found." << std::endl;
        return;
    }

    std::vector<std::pair<Idiom, uint32_t>> sorted_idioms(idiom_occurrences.begin(), idiom_occurrences.end());
    std::sort(sorted_idioms.begin(), sorted_idioms.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });

    for (auto& [idiom, occurrences] : sorted_idioms) {
        output << "Occurrences: " << occurrences << "\n";
        disassembler_.disassemble_range(idiom.get_offset(), idiom.get_length(), byte_file, output);
        output << "\n\n";

    }
}

lamar::IdiomAnalyzer::IdiomAnalyzer(const lamar::Disassembler &disassembler) : disassembler_(disassembler) {}

lamar::Idiom::Idiom(uint32_t offset, uint32_t length, const lamar::ByteFile &byte_file) : offset_(offset),
                                                                                          length_(length),
                                                                                          byte_file_(byte_file) {}

lamar::Idiom::Idiom(const lamar::Idiom &other) noexcept : offset_(other.offset_),
                                                         length_(other.length_),
                                                         byte_file_(other.byte_file_) {}

lamar::Idiom::Idiom(lamar::Idiom &&other) noexcept : offset_(other.offset_),
                                                    length_(other.length_),
                                                    byte_file_(other.byte_file_) {}

lamar::Idiom &lamar::Idiom::operator=(const lamar::Idiom &other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (&byte_file_ != &other.byte_file_) {
        return *this;
    }

    offset_ = other.offset_;
    length_ = other.length_;
    return *this;
}

lamar::Idiom &lamar::Idiom::operator=(lamar::Idiom &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (&byte_file_ != &other.byte_file_) {
        return *this;
    }

    offset_ = other.offset_;
    length_ = other.length_;
    return *this;
}

bool lamar::Idiom::operator==(const lamar::Idiom &other) const noexcept {
    if (length_ != other.length_) {
        return false;
    }

    if (offset_ + length_ > byte_file_.program_code.size() ||
        other.offset_ + other.length_ > other.byte_file_.program_code.size()) {
        return false;
    }

    const auto *lhs = byte_file_.program_code.data() + offset_;
    const auto *rhs = other.byte_file_.program_code.data() + other.offset_;

    return std::memcmp(lhs, rhs, length_) == 0;
}

uint32_t lamar::Idiom::get_offset() const {
    return offset_;
}

uint32_t lamar::Idiom::get_length() const {
    return length_;
}

size_t std::hash<lamar::Idiom>::operator()(const lamar::Idiom &idiom) const noexcept {
    const auto total_size = idiom.length_;

    if (idiom.offset_ + total_size > idiom.byte_file_.program_code.size()) {
        return 0;
    }

    const auto *data = idiom.byte_file_.program_code.data() + idiom.offset_;

    size_t hash = 1469598103934665603ull; // FNV-1a offset basis
    constexpr size_t prime = 1099511628211ull;

    for (uint32_t i = 0; i < total_size; ++i) {
        hash ^= static_cast<size_t>(data[i]);
        hash *= prime;
    }

    hash ^= static_cast<size_t>(total_size);
    return hash;
}
