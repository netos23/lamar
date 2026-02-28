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
    instruction_queue_(stack, MAX_FILE_SIZE / (2 * sizeof(InstructionInfo))),
    procedure_stack_(stack + MAX_STACK_SIZE / 2),
    verified_(reinterpret_cast<uint32_t *>(verified)),
    byte_file_(byte_file) {
    std::fill(verified_, verified_ + MAX_FILE_SIZE * sizeof(Frame) / sizeof(uint32_t), UNVISITED_INSTRUCTION);
}

void lamar::Verifier::verify() {
    auto main_offset = verify_public();
    verify_cfg(main_offset);
}

void lamar::Verifier::verify_cfg(uint32_t entry_point) {
    instruction_queue_.push({entry_point, 0});
    uint32_t height = 0;
    while (!instruction_queue_.empty()) {
        auto [offset, _] = instruction_queue_.pop();

        if (verified_[offset] != UNVISITED_INSTRUCTION && !IS_JUMP_TARGET(verified_[offset])) {
            if (instruction_queue_.empty()) {
                continue;
            }

            auto [next, _] = instruction_queue_.pop();
            height = GET_HEIGHT(verified_[next]);

            if (verified_[next] != UNVISITED_INSTRUCTION) {
                verified_[next] = MARK_JUMP_TARGET(height);
            }

            continue;
        }

        auto opcode = byte_file_.program_code[offset];
        if (!is_valid_opcode(opcode)) {
            diagnostics::push_error_diagnostic("Invalid opcode", offset);
            return;
        }

        auto length = instruction_length(offset);
        if (offset + length > byte_file_.program_code.size()) {
            diagnostics::push_error_diagnostic("Not enough bytes for instruction", offset);
            continue;
        }

        if (IS_JUMP_TARGET(verified_[offset])) {
            height = GET_HEIGHT(verified_[offset]);
        }
        auto [released, allocated] = get_stack_usage(offset);
        auto diff = static_cast<int32_t>(allocated) - static_cast<int32_t>(released);
        if (static_cast<int32_t>(height) - released < 0) {
            diagnostics::push_error_diagnostic("Stack underflow at instruction", offset);
            continue;
        }

        if (static_cast<int32_t>(height) + diff > MAX_STACK_SIZE) {
            diagnostics::push_error_diagnostic("Stack overflow at instruction", offset);
            continue;
        }

        height += diff;
        verified_[offset] = height;
        if (!procedure_stack_.empty()) {
            auto current_proc = procedure_stack_.peek();
            current_proc.max_stack_size = std::max(current_proc.max_stack_size, height);
        }

        if (opcode == BEGIN || opcode == CBEGIN) {
            procedure_stack_.push({offset, height});
        }

        verify_instruction(offset);

        if (opcode == JMP || opcode == CJMPZ || opcode == CJMPNZ) {
            auto location = read_uint(offset + 1);
            if (location >= byte_file_.program_code.size()) {
                diagnostics::push_error_diagnostic("Jump target out of bounds", location);
                continue;
            }

            if (verified_[location] != UNVISITED_INSTRUCTION) {
                auto [released_at_location, allocated_at_location] = get_stack_usage(offset);
                auto diff_at_location =
                        static_cast<int32_t>(allocated_at_location) - static_cast<int32_t>(released_at_location);
                if (GET_HEIGHT(verified_[location]) != height + diff_at_location) {
                    diagnostics::push_error_diagnostic("Inconsistent stack height at jump target", location);
                }
            } else {
                verified_[location] = MARK_JUMP_TARGET(height);
                instruction_queue_.push({location, get_priority(location)});
            }
        }

        if (opcode == JMP) {
            continue;
        }

        if (opcode == CALL || opcode == CLOSURE) {
            auto location = read_uint(offset + sizeof(OpCode));
            if (location >= byte_file_.program_code.size()) {
                diagnostics::push_error_diagnostic("Procedure address out of bounds", location);
                continue;
            }
            verified_[location] = MARK_JUMP_TARGET(0);
            instruction_queue_.push({location, get_priority(location)});
        }


        if (opcode == END || opcode == RET || opcode == FAIL) {
            auto [proc_address, max_stack_size] = procedure_stack_.pop();

            if (max_stack_size > 0xFFFFu) {
                diagnostics::push_error_diagnostic("Procedure requires too much stack space", proc_address);
            }

            auto begin_op = static_cast<OpCode>(byte_file_.program_code[proc_address]);
            if (begin_op == BEGIN || begin_op == CBEGIN) {
                auto arg_offset = proc_address + sizeof(OpCode);
                if (arg_offset + sizeof(uint32_t) <= byte_file_.program_code.size()) {
                    auto arg = read_uint(arg_offset);
                    arg = (arg & 0x0000FFFFu) | ((max_stack_size & 0xFFFFu) << 16);
                    std::memcpy(byte_file_.program_code.data() + arg_offset, &arg, sizeof(uint32_t));
                } else {
                    diagnostics::push_error_diagnostic("Index out of bounds while writing begin arg", arg_offset);
                }
            }


            if (instruction_queue_.empty())
                continue;

            auto [next_instr, next_proc] = instruction_queue_.peek();

            if (verified_[next_instr] != UNVISITED_INSTRUCTION) {
                verified_[next_instr] = MARK_JUMP_TARGET(verified_[next_instr]);
            }

            continue;
        }

        instruction_queue_.push({offset + length, get_priority(offset + length)});
    }
}

uint32_t lamar::Verifier::get_priority(uint32_t offset) const {
    if (byte_file_.program_code[offset] == END
        || byte_file_.program_code[offset] == RET
        || byte_file_.program_code[offset] == FAIL) {
        return (procedure_stack_.size() + 1) * 2 - 1; // prioritize procedure entries
    }

    return (procedure_stack_.size() + 1) * 2;
}

void lamar::Verifier::verify_instruction(uint32_t offset) const {
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
            auto locals_count = read_uint(procedure_stack_.peek().offset + sizeof(OpCode) + sizeof(uint32_t));

            if (index >= locals_count) {
                diagnostics::push_error_diagnostic("Local variable index out of bounds", offset);
            }
            break;
        }

        case lamar::OpCode::ST_A:
        case lamar::OpCode::LDA_A:
        case lamar::OpCode::LD_A: {
            auto index = read_uint(ip);
            auto args_count = read_uint(procedure_stack_.peek().offset + sizeof(OpCode));

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
                        auto locals_count = read_uint(
                                procedure_stack_.peek().offset + sizeof(OpCode) + sizeof(uint32_t));

                        if (address >= locals_count) {
                            diagnostics::push_error_diagnostic("Local variable index out of bounds", offset);
                        }
                        break;
                    }
                    case Arg: {
                        auto args_count = read_uint(procedure_stack_.peek().offset + sizeof(OpCode));
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

std::pair<uint32_t, uint32_t> lamar::Verifier::get_stack_usage(uint32_t offset) const {
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
