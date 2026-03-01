//
// Created by Nikita Morozov on 24.02.2026.
//

#include "verifier.hpp"
#include "interpreter.hpp"
#include <cstring>


uint32_t lamar::Verifier::instruction_length(uint32_t offset) {
    return disassembler_.disassemble_instruction(byte_file_, ofs_, offset);
}


lamar::Verifier::Verifier(
        lamar::ByteFile &byte_file,
        const lamar::Disassembler &disassembler,
        auint *stack,
        void *verified
) : disassembler_(disassembler),
    procedure_stack_(stack),
    verified_(reinterpret_cast<uint16_t *>(verified)),
    byte_file_(byte_file) {
    std::fill(verified_, verified_ + MAX_FILE_SIZE * sizeof(Frame) / sizeof(uint32_t), UNVISITED_INSTRUCTION);
}

void lamar::Verifier::verify() {
    auto main_offset = verify_public();
    verify_cfg(main_offset);
}

void lamar::Verifier::verify_cfg(uint32_t entry_point) {
    procedure_stack_.push(entry_point);
    uint32_t height = 0;

    while (!procedure_stack_.empty()) {
        auto offset = procedure_stack_.pop();
        uint32_t max_stack_size = 0;
        uint32_t procedure_offset = 0;

        if (IS_JUMP_TARGET(offset)) {
            procedure_offset = procedure_stack_.pop();
            offset = GET_OFFSET(offset);
            height = verified_[offset];

            auto args_count = read_uint(procedure_offset + sizeof(OpCode));
            max_stack_size = ((args_count >> 16) & 0xFFFFu);;
        } else {
            procedure_offset = offset;
        }


        while (true) {

            auto opcode = byte_file_.program_code[offset];
            if (!is_valid_opcode(opcode)) {
                diagnostics::push_error_diagnostic("Invalid opcode", offset);
                return;
            }

            auto length = instruction_length(offset);
            if (offset + length > byte_file_.program_code.size()) {
                diagnostics::push_error_diagnostic("Not enough bytes for instruction", offset);
                return;
            }

            auto [released, allocated] = get_stack_usage(offset);
            auto diff = static_cast<int32_t>(allocated) - static_cast<int32_t>(released);
            if (static_cast<int32_t>(height) - released < 0) {
                diagnostics::push_error_diagnostic("Stack underflow at instruction", offset);
                return;
            }

            if (static_cast<int32_t>(height) + diff > MAX_VERIFIED_SIZE) {
                diagnostics::push_error_diagnostic("Stack overflow at instruction", offset);
                return;
            }

            height += diff;
            verified_[offset] = height;
            max_stack_size = std::max(max_stack_size, height);

            verify_instruction(procedure_offset, offset);

            if (opcode == JMP || opcode == CJMPZ || opcode == CJMPNZ) {
                auto location = read_uint(offset + sizeof(OpCode));
                if (location >= byte_file_.program_code.size()) {
                    diagnostics::push_error_diagnostic("Jump target out of bounds", location);
                    return;
                }

                if (verified_[location] != UNVISITED_INSTRUCTION) {
                    auto [released_at_location, allocated_at_location] = get_stack_usage(offset);
                    auto diff_at_location =
                            static_cast<int32_t>(allocated_at_location) - static_cast<int32_t>(released_at_location);

                    if (verified_[location] != height + diff_at_location) {
                        diagnostics::push_error_diagnostic("Inconsistent stack height at jump target", location);
                        return;
                    }
                } else {
                    verified_[location] = height;
                    procedure_stack_.push(procedure_offset);
                    procedure_stack_.push(MARK_JUMP_TARGET(location));
                }

                if (opcode == JMP) {
                    offset = location;
                    continue;
                }
            }


            if (opcode == CALL || opcode == CLOSURE) {
                auto location = read_uint(offset + sizeof(OpCode));
                if (location >= byte_file_.program_code.size()) {
                    diagnostics::push_error_diagnostic("Procedure address out of bounds", location);
                    return;
                }

                if (verified_[location] != UNVISITED_INSTRUCTION) {
                    procedure_stack_.push(location);
                }
            }


            if (opcode == END || opcode == RET || opcode == FAIL) {

                if (max_stack_size > MAX_VERIFIED_SIZE) {
                    diagnostics::push_error_diagnostic("Procedure requires too much stack space", procedure_offset);
                    return;
                }

                auto begin_op = static_cast<OpCode>(byte_file_.program_code[procedure_offset]);
                if (begin_op == BEGIN || begin_op == CBEGIN) {
                    auto arg_offset = procedure_offset + sizeof(OpCode);
                    if (arg_offset + sizeof(uint32_t) <= byte_file_.program_code.size()) {
                        auto arg = read_uint(arg_offset);
                        arg = (arg & 0x0000FFFFu) | ((max_stack_size & 0xFFFFu) << 16);
                        std::memcpy(byte_file_.program_code.data() + arg_offset, &arg, sizeof(uint32_t));
                    } else {
                        diagnostics::push_error_diagnostic("Index out of bounds while writing begin arg", arg_offset);
                    }
                }

                break;
            }

            offset += length;
        }
    }
}

void lamar::Verifier::verify_instruction(uint32_t procedure_offset, uint32_t offset) const {
    auto opcode = static_cast<OpCode>(byte_file_.program_code[offset]);
    auto ip = offset + sizeof(OpCode);

    switch (opcode) {
        case STRING: {
            auto index = read_uint(ip);
            if (index >= byte_file_.string_table_size) {
                diagnostics::push_error_diagnostic("String index out of bounds", offset);
            }
            break;
        }

        case SEXP: {
            auto tag = read_uint(ip);
            auto size = read_uint(ip + sizeof(uint32_t));
            (void) size;

            if (tag >= byte_file_.string_table_size) {
                diagnostics::push_error_diagnostic("String index out of bounds", offset);
            }
            break;
        }

        case lamar::OpCode::LD_G:
        case lamar::OpCode::ST_G:
        case lamar::OpCode::LDA_G: {
            auto index = read_uint(ip);
            if (index >= byte_file_.global_area_size) {
                diagnostics::push_error_diagnostic("Global variable index out of bounds", offset);
            }
            break;
        }

        case lamar::OpCode::LD_L:
        case lamar::OpCode::LDA_L:
        case lamar::OpCode::ST_L: {
            auto index = read_uint(ip);
            auto locals_count = read_uint(procedure_offset + sizeof(OpCode) + sizeof(uint32_t));

            if (index >= locals_count) {
                diagnostics::push_error_diagnostic("Local variable index out of bounds", offset);
            }
            break;
        }

        case lamar::OpCode::ST_A:
        case lamar::OpCode::LDA_A:
        case lamar::OpCode::LD_A: {
            auto index = read_uint(ip);
            auto args_count = read_uint(procedure_offset + sizeof(OpCode));

            if (index >= args_count) {
                diagnostics::push_error_diagnostic("Argument index out of bounds", offset);
            }
            break;
        }


        case JMP:
        case CJMPZ:
        case CJMPNZ: {
            auto location = read_uint(ip);
            if (location >= byte_file_.program_code.size()) {
                diagnostics::push_error_diagnostic("Jump out of bounds", offset);
            }
            break;
        }

        case CLOSURE: {
            auto location = read_uint(ip);
            if (location >= byte_file_.program_code.size()) {
                diagnostics::push_error_diagnostic("Closure address out of bounds", offset);
                break;
            }
            auto closure_args_count = read_uint(ip + sizeof(uint32_t));
            auto allow_begin = true;


            ip += sizeof(uint32_t) * 2;
            for (uint32_t i = 0; i < closure_args_count; i++) {
                auto type = ClosureArgType(byte_file_.program_code[ip]);
                if (!is_valid_closure_arg_type(type)) {
                    diagnostics::push_error_diagnostic("Invalid closure argument type", offset);
                    break;
                }

                ip += sizeof(ClosureArgType);

                auto address = read_uint(ip);

                ip += sizeof(uint32_t);

                switch (type) {
                    case Global:
                        if (address >= byte_file_.global_area_size) {
                            diagnostics::push_error_diagnostic("Global variable index out of bounds", offset);
                        }
                        break;
                    case Local: {
                        auto locals_count = read_uint(procedure_offset + sizeof(OpCode) + sizeof(uint32_t));

                        if (address >= locals_count) {
                            diagnostics::push_error_diagnostic("Local variable index out of bounds", offset);
                        }
                        break;
                    }
                    case Arg: {
                        auto args_count = read_uint(procedure_offset + sizeof(OpCode));
                        if (address >= args_count) {
                            diagnostics::push_error_diagnostic("Argument index out of bounds", offset);
                        }
                        break;
                    }
                    case Capture:
                        allow_begin = false;
                        break;
                }
            }

            auto entry = byte_file_.program_code[location];
            if ((entry != BEGIN || !allow_begin) && entry != CBEGIN) {
                diagnostics::push_error_diagnostic("Closure entry must be begin or cbegin", offset);
            }
            break;
        }

        case CALL: {
            auto proc_address = read_uint(ip);
            if (proc_address >= byte_file_.program_code.size()) {
                diagnostics::push_error_diagnostic("Closure address out of bounds", offset);
                break;
            }

            auto entry = byte_file_.program_code[proc_address];
            if (entry != BEGIN && entry != CBEGIN) {
                diagnostics::push_error_diagnostic("Procedure entry must be begin or cbegin", offset);
            }
            break;
        }

        case TAG: {
            auto tag = read_uint(ip);
            auto size = read_uint(ip + sizeof(uint32_t));
            (void) size;

            if (tag >= byte_file_.string_table_size) {
                diagnostics::push_error_diagnostic("String index out of bounds", offset);
            }
            break;
        }

        default:
            break;
    }
}

uint32_t lamar::Verifier::verify_public() const {
    auto main_offset = static_cast<uint32_t>(-1);

    for (const auto &[index, offset]: byte_file_.public_symbol_table) {
        if (offset >= byte_file_.program_code.size()) {
            diagnostics::push_error_diagnostic("Symbol points outside program code", offset);
            continue;
        }


        if (index >= byte_file_.string_table_size) {
            diagnostics::push_error_diagnostic("String index out of bounds", offset);
        }

        auto name = std::string_view(&byte_file_.string_table.get()[index]);

        if (name == "main") {
            main_offset = offset;
        }
    }

    if (main_offset == static_cast<uint32_t>(-1)) {
        diagnostics::push_error_diagnostic("Public symbol \"main\" not found", 0);
        return 0;
    }

    return main_offset;
}

uint32_t lamar::Verifier::read_uint(uint32_t offset) const {

    if (offset + sizeof(uint32_t) > byte_file_.program_code.size()) {
        diagnostics::push_error_diagnostic("Index out of bounds while reading uint", offset);
    }

    uint32_t res = 0;
    std::memcpy(&res, byte_file_.program_code.data() + offset, sizeof(uint32_t));
    return res;
}

std::pair<uint16_t, uint16_t> lamar::Verifier::get_stack_usage(uint32_t offset) const {
    auto code = static_cast<OpCode>(byte_file_.program_code[offset]);
    uint32_t ip = offset + sizeof(OpCode);

    switch (code) {
        case ADD:
        case SUB:
        case MUL:
        case DIV:
        case MOD:
        case LT:
        case LE:
        case GT:
        case GE:
        case EQ:
        case NE:
        case AND:
        case OR:
            return {2, 1};

        case CONST:
        case STRING:
            return {0, 1};

        case SEXP: {
            auto size = read_uint(ip + sizeof(uint32_t));
            return {size, 1};
        }

        case STI:
            return {2, 1};

        case STA:
            return {3, 1};

        case JMP:
            return {0, 0};

        case END:
        case RET:
            return {1, 1};

        case DROP:
            return {1, 0};

        case DUP:
            return {1, 2};

        case SWAP:
            return {2, 2};

        case ELEM:
            return {2, 1};

        case LD_G:
        case LD_L:
        case LD_A:
        case LD_C:
        case LDA_G:
        case LDA_L:
        case LDA_A:
        case LDA_C:
            return {0, 1};

        case ST_G:
        case ST_L:
        case ST_A:
        case ST_C:
            return {1, 1};

        case CJMPZ:
        case CJMPNZ:
            return {1, 0};

        case BEGIN:
        case CBEGIN:
            return {0, 0};
        case CLOSURE: {
            auto captures = read_uint(ip + sizeof(uint32_t));
            return {0, 1 + captures};
        }
        case CALLC: {
            auto args = read_uint(ip);
            return {args + 1, 1};
        }
        case CALL: {
            auto args = read_uint(ip + sizeof(uint32_t));
            return {args, 1};
        }

        case TAG:
        case ARRAY:
            return {1, 1};

        case FAIL:
            return {1, 0};

        case LINE:
            return {0, 0};

        case PATTEQSTR:
            return {2, 1};

        case PATTSTRING:
        case PATTARRAY:
        case PATTSEXP:
        case PATTREF:
        case PATTVAL:
        case PATTFUN:
            return {1, 1};

        case CALL_LREAD:
            return {0, 1};

        case CALL_LWRITE:
        case CALL_LLENGTH:
        case CALL_LSTRING:
            return {1, 1};

        case CALL_BARRAY: {
            auto len = read_uint(ip);
            return {len, 1};
        }

        case EOF_:
        default:
            return {0, 0};
    }
}
