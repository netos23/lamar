//
// Created by Nikita Morozov on 25.01.2026.
//

#include "disassembler.hpp"

#include <cstddef>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <stdexcept>
#include <string_view>

namespace {
    using lamar::ByteFile;
    using lamar::OpCode;

    int32_t read_int32(const uint8_t *&ip, const uint8_t *end) {
        if (end - ip < static_cast<std::ptrdiff_t>(sizeof(int32_t))) {
            throw std::runtime_error("Unexpected end of bytecode while reading int32");
        }

        int32_t val = 0;
        std::memcpy(&val, ip, sizeof(val));
        ip += sizeof(val);
        return val;
    }
}

namespace lamar {
    void Disassembler::disassemble(const ByteFile &byte_file, std::ostream &output) const {
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
        const auto *ip = base;
        const auto *end = base + byte_file.program_code.size();

        constexpr const char *ops[] = {"+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "||"};
        constexpr const char *pats[] = {"=str", "#string", "#array", "#sexp", "#ref", "#val", "#fun"};
        constexpr const char *lds[] = {"LD", "LDA", "ST"};

        auto format_addr = [&](std::ptrdiff_t pos) {
            output << "0x" << std::hex << std::setw(8) << std::setfill('0') << static_cast<uint32_t>(pos)
                   << ":\t";
        };

        while (ip < end) {
            uint8_t opcode = *ip++;
            uint8_t h = static_cast<uint8_t>((opcode & 0xF0) >> 4);
            uint8_t l = static_cast<uint8_t>(opcode & 0x0F);

            format_addr(ip - base - 1);
            output << std::dec;  // ensure operands default to decimal unless explicitly hex

            switch (h) {
                case 0x0: { // BINOP
                    if (l == 0 || l > 0x0D) {
                        throw std::runtime_error("Invalid BINOP opcode");
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
                            output << "SEXP\t" << read_string(byte_file, read_int32(ip, end)) << ' ' << read_int32(ip, end);
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
                            throw std::runtime_error("Invalid opcode in 0x1x group");
                    }
                    break;
                }
                case 0x2:
                case 0x3:
                case 0x4: {
                    const auto group = static_cast<size_t>(h - 0x2);
                    if (group >= std::size(lds)) {
                        throw std::runtime_error("Invalid LD/LDA/ST group");
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
                            throw std::runtime_error("Invalid LD/LDA/ST opcode suffix");
                    }
                    break;
                }
                case 0x5: {
                    switch (l) {
                        case 0x0:
                            output << "CJMPz\t0x" << std::hex << std::setw(8) << std::setfill('0') << read_int32(ip, end);
                            break;
                        case 0x1:
                            output << "CJMPnz\t0x" << std::hex << std::setw(8) << std::setfill('0') << read_int32(ip, end);
                            break;
                        case 0x2:
                            output << "BEGIN\t" << std::dec << read_int32(ip, end) << ' ' << read_int32(ip, end);
                            break;
                        case 0x3:
                            output << "CBEGIN\t" << std::dec << read_int32(ip, end) << ' ' << read_int32(ip, end);
                            break;
                        case 0x4: {
                            output << "CLOSURE\t0x" << std::hex << std::setw(8) << std::setfill('0') << read_int32(ip, end);
                            const int32_t n = read_int32(ip, end);
                            for (int i = 0; i < n; ++i) {
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
                                        throw std::runtime_error("Invalid closure captured variable type");
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
                            output << "TAG\t" << read_string(byte_file, read_int32(ip, end)) << ' ' << std::dec << read_int32(ip, end);
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
                            throw std::runtime_error("Invalid opcode in 0x5x group");
                    }
                    break;
                }
                case 0x6: {
                    if (l >= std::size(pats)) {
                        throw std::runtime_error("Invalid pattern opcode");
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
                            throw std::runtime_error("Invalid builtin call opcode");
                    }
                    break;
                }
                case 0xF: {
                    output << "<end>";
                    output << '\n';
                    output << std::dec;
                    return;
                }
                default:
                    throw std::runtime_error("Unknown opcode group while disassembling");
            }

            output << '\n';
            output << std::dec;
        }

        output << "<end>\n";
    }
}
