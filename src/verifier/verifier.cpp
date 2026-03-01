//
// Created by Nikita Morozov on 24.02.2026.
//

#include "verifier.hpp"
#include "interpreter.hpp"
#include <algorithm>
#include <cstring>
#include <limits>


uint32_t lamar::Verifier::instruction_length(uint32_t offset) {
    return disassembler_.disassemble_instruction(byte_file_, ofs_, offset);
}


lamar::Verifier::Verifier(
        lamar::ByteFile &byte_file,
        const lamar::Disassembler &disassembler
) : disassembler_(disassembler),
    instruction_height_(byte_file.program_code.size(), HEIGHT_UNKNOWN),
    byte_file_(byte_file) {
    worklist_.reserve(byte_file_.program_code.size());
}

void lamar::Verifier::verify() {
    auto main_offset = verify_public();
    verify_cfg(main_offset);
}

void lamar::Verifier::verify_cfg(uint32_t entry_point) {
    std::fill(instruction_height_.begin(), instruction_height_.end(), HEIGHT_UNKNOWN);
    worklist_.clear();
    enqueue_instruction(entry_point, 0);

    while (!worklist_.empty()) {
        const auto offset = worklist_.back();
        worklist_.pop_back();

        const auto height = instruction_height_[offset];

        const auto opcode = byte_file_.program_code[offset];
        if (!is_valid_opcode(opcode)) {
            diagnostics::push_error_diagnostic("Invalid opcode", offset);
            continue;
        }

        const auto length = instruction_length(offset);
        if (offset + length > byte_file_.program_code.size()) {
            diagnostics::push_error_diagnostic("Not enough bytes for instruction", offset);
            continue;
        }

        const auto [released, allocated] = get_stack_usage(offset);
        const auto after_pop = static_cast<int32_t>(height) - static_cast<int32_t>(released);
        if (after_pop < 0) {
            diagnostics::push_error_diagnostic("Stack underflow at instruction", offset);
            continue;
        }

        const auto next_height_signed = after_pop + static_cast<int32_t>(allocated);
        if (next_height_signed > MAX_STACK_SIZE) {
            diagnostics::push_error_diagnostic("Stack overflow at instruction", offset);
            continue;
        }

        const auto next_height = static_cast<uint32_t>(next_height_signed);
        const auto current_height = static_cast<uint32_t>(height);

        if (!procedure_stack_.empty()) {
            auto &proc = procedure_stack_.back();
            proc.max_stack_size = std::max(proc.max_stack_size, current_height);
            proc.max_stack_size = std::max(proc.max_stack_size, next_height);
        }

        if (opcode == BEGIN || opcode == CBEGIN) {
            procedure_stack_.push_back({offset, next_height});
        }

        verify_instruction(offset);

        if (opcode == JMP) {
            const auto location = read_uint(offset + sizeof(OpCode));
            if (location >= byte_file_.program_code.size()) {
                diagnostics::push_error_diagnostic("Jump target out of bounds", location);
            } else {
                enqueue_instruction(location, next_height);
            }
            continue;
        }

        if (opcode == CJMPZ || opcode == CJMPNZ) {
            const auto location = read_uint(offset + sizeof(OpCode));
            if (location >= byte_file_.program_code.size()) {
                diagnostics::push_error_diagnostic("Jump target out of bounds", location);
            } else {
                enqueue_instruction(location, next_height);
            }
            enqueue_instruction(offset + length, next_height);
            continue;
        }

        if (opcode == CALL || opcode == CLOSURE) {
            const auto location = read_uint(offset + sizeof(OpCode));
            if (location >= byte_file_.program_code.size()) {
                diagnostics::push_error_diagnostic("Procedure address out of bounds", location);
            } else {
                enqueue_instruction(location, 0);
            }
        }

        if (opcode == END || opcode == RET || opcode == FAIL) {
            if (procedure_stack_.empty()) {
                diagnostics::push_error_diagnostic("END/RET/FAIL outside procedure", offset);
                continue;
            }

            auto proc_info = procedure_stack_.back();
            proc_info.max_stack_size = std::max(proc_info.max_stack_size, next_height);
            procedure_stack_.pop_back();

            if (proc_info.max_stack_size > 0xFFFFu) {
                diagnostics::push_error_diagnostic("Procedure requires too much stack space", proc_info.offset);
            }

            auto begin_op = static_cast<OpCode>(byte_file_.program_code[proc_info.offset]);
            if (begin_op == BEGIN || begin_op == CBEGIN) {
                auto arg_offset = proc_info.offset + sizeof(OpCode);
                if (arg_offset + sizeof(uint32_t) <= byte_file_.program_code.size()) {
                    auto arg = read_uint(arg_offset);
                    arg = (arg & 0x0000FFFFu) | ((proc_info.max_stack_size & 0xFFFFu) << 16);
                    std::memcpy(byte_file_.program_code.data() + arg_offset, &arg, sizeof(uint32_t));
                } else {
                    diagnostics::push_error_diagnostic("Index out of bounds while writing begin arg", arg_offset);
                }
            }

            continue;
        }

        enqueue_instruction(offset + length, next_height);
    }
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
            if (procedure_stack_.empty()) {
                diagnostics::push_error_diagnostic("Local variable access outside procedure", offset);
                break;
            }
            auto locals_count = read_uint(procedure_stack_.back().offset + sizeof(OpCode) + sizeof(uint32_t));

            if (index >= locals_count) {
                diagnostics::push_error_diagnostic("Local variable index out of bounds", offset);
            }
            break;
        }

        case lamar::OpCode::ST_A:
        case lamar::OpCode::LDA_A:
        case lamar::OpCode::LD_A: {
            auto index = read_uint(ip);
            if (procedure_stack_.empty()) {
                diagnostics::push_error_diagnostic("Argument access outside procedure", offset);
                break;
            }
            auto args_count = read_uint(procedure_stack_.back().offset + sizeof(OpCode));

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
                        if (procedure_stack_.empty()) {
                            diagnostics::push_error_diagnostic("Local variable access outside procedure", offset);
                            break;
                        }
                        auto locals_count = read_uint(
                                procedure_stack_.back().offset + sizeof(OpCode) + sizeof(uint32_t));

                        if (address >= locals_count) {
                            diagnostics::push_error_diagnostic("Local variable index out of bounds", offset);
                        }
                        break;
                    }
                    case Arg: {
                        if (procedure_stack_.empty()) {
                            diagnostics::push_error_diagnostic("Argument access outside procedure", offset);
                            break;
                        }
                        auto args_count = read_uint(procedure_stack_.back().offset + sizeof(OpCode));
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
        return 0;
    }

    uint32_t res = 0;
    std::memcpy(&res, byte_file_.program_code.data() + offset, sizeof(uint32_t));
    return res;
}

void lamar::Verifier::enqueue_instruction(uint32_t offset, uint32_t height) {
    if (offset >= byte_file_.program_code.size()) {
        diagnostics::push_error_diagnostic("Jump target out of bounds", offset);
        return;
    }

    if (height > std::numeric_limits<uint16_t>::max()) {
        diagnostics::push_error_diagnostic("Stack height exceeds encodable range", offset);
        return;
    }

    auto &stored_height = instruction_height_[offset];
    if (stored_height == HEIGHT_UNKNOWN) {
        stored_height = static_cast<uint16_t>(height);
        worklist_.push_back(offset);
        return;
    }

    if (stored_height != height) {
        diagnostics::push_error_diagnostic("Inconsistent stack height at instruction", offset);
    }
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
