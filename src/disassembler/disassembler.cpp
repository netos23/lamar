//
// Created by Nikita Morozov on 25.01.2026.
//

#include "disassembler.hpp"
#include "diagnostics.hpp"

#include <cstddef>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <stdexcept>
#include <string_view>


int32_t read_int32(const uint8_t *&ip, const uint8_t *end) {
    if (end - ip < static_cast<std::ptrdiff_t>(sizeof(int32_t))) {
        lamar::diagnostics::push_error_diagnostic("Unexpected end of bytecode while reading int32", end - ip);
    }

    int32_t val = 0;
    std::memcpy(&val, ip, sizeof(val));
    ip += sizeof(val);
    return val;
}

std::string_view read_string(const lamar::ByteFile &file, int32_t offset) {
    if (offset < 0 || static_cast<uint32_t>(offset) >= file.string_table_size) {
        lamar::diagnostics::push_error_diagnostic("Invalid string offset in bytecode", offset);
    }

    const char *start = file.string_table.get() + offset;
    const auto max_len = file.string_table_size - static_cast<uint32_t>(offset);
    size_t len = 0;
    while (len < max_len && start[len] != '\0') {
        ++len;
    }
    return {start, len};
}


constexpr const char *ops[] = {"+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "||"};
constexpr const char *pats[] = {"=str", "#string", "#array", "#sexp", "#ref", "#val", "#fun"};
constexpr const char *lds[] = {"LD", "LDA", "ST"};

uint32_t disassemble_instruction_impl(
        const lamar::ByteFile &byte_file,
        std::ostream &output,
        uint32_t offset,
        const uint8_t *base,
        const uint8_t *end,
        bool &hit_end
) {
    auto format_addr = [&](std::ptrdiff_t pos) {
        output << "0x" << std::hex << std::setw(8) << std::setfill('0') << static_cast<uint32_t>(pos)
               << ":\t";
    };

    const auto *inst_start = base + offset;
    if (inst_start >= end) {
        lamar::diagnostics::push_error_diagnostic("Unexpected end of bytecode while reading opcode", offset);
    }

    const uint8_t *ip = inst_start;
    uint8_t opcode = *ip++;
    uint8_t h = static_cast<uint8_t>((opcode & 0xF0) >> 4);
    uint8_t l = static_cast<uint8_t>(opcode & 0x0F);

    format_addr(inst_start - base);
    output << std::dec;

    switch (h) {
        case 0x0: { // BINOP
            if (l == 0 || l > 0x0D) {
                lamar::diagnostics::push_error_diagnostic("Invalid BINOP opcode", ip - base);
            }
            output << "BINOP\t" << ops[l - 1];
            break;
        }
        case 0x1: {
            switch (l) {
                case 0x0:
                    output << "CONST\t" << read_int32(ip, end);
                    break;
                case 0x1:
                    output << "STRING\t" << read_string(byte_file, read_int32(ip, end));
                    break;
                case 0x2:
                    output << "SEXP\t" << read_string(byte_file, read_int32(ip, end)) << ' '
                           << read_int32(ip, end);
                    break;
                case 0x3:
                    output << "STI";
                    break;
                case 0x4:
                    output << "STA";
                    break;
                case 0x5:
                    output << "JMP\t0x" << std::hex << std::setw(8) << std::setfill('0') << read_int32(ip, end);
                    break;
                case 0x6:
                    output << "END";
                    break;
                case 0x7:
                    output << "RET";
                    break;
                case 0x8:
                    output << "DROP";
                    break;
                case 0x9:
                    output << "DUP";
                    break;
                case 0xA:
                    output << "SWAP";
                    break;
                case 0xB:
                    output << "ELEM";
                    break;
                default:
                    lamar::diagnostics::push_error_diagnostic("Invalid opcode in 0x1x group", ip - base);
            }
            break;
        }
        case 0x2:
        case 0x3:
        case 0x4: {
            const auto group = static_cast<size_t>(h - 0x2);
            if (group >= std::size(lds)) {
                lamar::diagnostics::push_error_diagnostic("Invalid LD/LDA/ST group", ip - base);
            }
            output << lds[group] << '\t';
            switch (l) {
                case 0x0:
                    output << 'G' << '(' << read_int32(ip, end) << ')';
                    break;
                case 0x1:
                    output << 'L' << '(' << read_int32(ip, end) << ')';
                    break;
                case 0x2:
                    output << 'A' << '(' << read_int32(ip, end) << ')';
                    break;
                case 0x3:
                    output << 'C' << '(' << read_int32(ip, end) << ')';
                    break;
                default:
                    lamar::diagnostics::push_error_diagnostic("Invalid LD/LDA/ST opcode suffix", ip - base);
            }
            break;
        }
        case 0x5: {
            switch (l) {
                case 0x0:
                    output << "CJMPz\t0x" << std::hex << std::setw(8) << std::setfill('0')
                           << read_int32(ip, end);
                    break;
                case 0x1:
                    output << "CJMPnz\t0x" << std::hex << std::setw(8) << std::setfill('0')
                           << read_int32(ip, end);
                    break;
                case 0x2:
                    output << "BEGIN\t" << std::dec << read_int32(ip, end) << ' ' << read_int32(ip, end);
                    break;
                case 0x3:
                    output << "CBEGIN\t" << std::dec << read_int32(ip, end) << ' ' << read_int32(ip, end);
                    break;
                case 0x4: {
                    output << "CLOSURE\t0x" << std::hex << std::setw(8) << std::setfill('0')
                           << read_int32(ip, end);
                    const int32_t n = read_int32(ip, end);
                    for (int i = 0; i < n; ++i) {
                        if (ip >= end) {
                            lamar::diagnostics::push_error_diagnostic(
                                    "Unexpected end of bytecode while reading closure captured variable type",
                                    ip - base
                            );
                        }
                        uint8_t scope = *ip++;
                        switch (scope) {
                            case 0x0:
                                output << "G(" << std::dec << read_int32(ip, end) << ')';
                                break;
                            case 0x1:
                                output << "L(" << std::dec << read_int32(ip, end) << ')';
                                break;
                            case 0x2:
                                output << "A(" << std::dec << read_int32(ip, end) << ')';
                                break;
                            case 0x3:
                                output << "C(" << std::dec << read_int32(ip, end) << ')';
                                break;
                            default:
                                lamar::diagnostics::push_error_diagnostic(
                                        "Invalid closure captured variable type",
                                        ip - base
                                );
                        }
                    }
                    break;
                }
                case 0x5:
                    output << "CALLC\t" << std::dec << read_int32(ip, end);
                    break;
                case 0x6:
                    output << "CALL\t0x" << std::hex << std::setw(8) << std::setfill('0') << read_int32(ip, end)
                           << ' ' << std::dec << read_int32(ip, end);
                    break;
                case 0x7:
                    output << "TAG\t" << read_string(byte_file, read_int32(ip, end)) << ' ' << std::dec
                           << read_int32(ip, end);
                    break;
                case 0x8:
                    output << "ARRAY\t" << std::dec << read_int32(ip, end);
                    break;
                case 0x9:
                    output << "FAIL\t" << std::dec << read_int32(ip, end) << read_int32(ip, end);
                    break;
                case 0xA:
                    output << "LINE\t" << std::dec << read_int32(ip, end);
                    break;
                default:
                    lamar::diagnostics::push_error_diagnostic("Invalid opcode in 0x5x group", ip - base);
            }
            break;
        }
        case 0x6: {
            if (l >= std::size(pats)) {
                lamar::diagnostics::push_error_diagnostic("Invalid pattern opcode", ip - base);
            }
            output << "PATT\t" << pats[l];
            break;
        }
        case 0x7: {
            switch (l) {
                case 0x0:
                    output << "CALL\tLread";
                    break;
                case 0x1:
                    output << "CALL\tLwrite";
                    break;
                case 0x2:
                    output << "CALL\tLlength";
                    break;
                case 0x3:
                    output << "CALL\tLstring";
                    break;
                case 0x4:
                    output << "CALL\tBarray\t" << std::dec << read_int32(ip, end);
                    break;
                default:
                    lamar::diagnostics::push_error_diagnostic("Invalid builtin call opcode", ip - base);
            }
            break;
        }
        case 0xF: {
            output << "<end>";
            output << '\n';
            output << std::dec;
            hit_end = true;
            return static_cast<uint32_t>(ip - inst_start);
        }
        default:
            lamar::diagnostics::push_error_diagnostic("Unknown opcode group while disassembling", ip - base);
    }

    output << '\n';
    output << std::dec;
    return static_cast<uint32_t>(ip - inst_start);
}


void lamar::Disassembler::disassemble(const ByteFile &byte_file, std::ostream &output) const {
    output << "String table size       : " << byte_file.string_table_size << '\n';
    output << "Global area size        : " << byte_file.global_area_size << '\n';
    output << "Number of public symbols: " << byte_file.public_symbols_number << '\n';
    output << "Public symbols          :" << '\n';

    for (uint32_t i = 0; i < byte_file.public_symbols_number; ++i) {
        const auto &sym = byte_file.public_symbol_table.at(i);
        output << "   0x" << std::hex << std::setw(8) << std::setfill('0') << sym.offset
               << ": " << read_string(byte_file, static_cast<int32_t>(sym.name_offset)) << '\n';
    }

    output << std::dec << "Code:" << '\n';

    const auto *base = reinterpret_cast<const uint8_t *>(byte_file.program_code.data());
    const auto code_size = static_cast<uint32_t>(byte_file.program_code.size());

    const auto *range_end = base + code_size;

    uint32_t cursor = 0;
    while (base + cursor < range_end) {
        bool hit_end = false;
        const uint32_t consumed = disassemble_instruction_impl(byte_file, output, cursor, base, range_end, hit_end);
        if (consumed == 0) {
            diagnostics::push_error_diagnostic("Decoded instruction with zero length", cursor);
        }
        cursor += consumed;
        if (hit_end) {
            return;
        }
    }
}

uint32_t lamar::Disassembler::disassemble_instruction(
        const ByteFile &byte_file,
        std::ostream &output,
        uint32_t offset
) const {
    const auto *base = reinterpret_cast<const uint8_t *>(byte_file.program_code.data());
    const auto *end = base + byte_file.program_code.size();
    bool hit_end = false;
    return disassemble_instruction_impl(byte_file, output, offset, base, end, hit_end);
}

