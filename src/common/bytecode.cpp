//
// Created by Nikita Morozov on 07.01.2026.
//

#include "bytecode.hpp"


lamar::PublicSymbol::PublicSymbol(uint32_t nameOffset, uint32_t offset) : name_offset(nameOffset), offset(offset) {}

bool lamar::is_valid_opcode(uint8_t opcode) {
    switch (opcode) {
        case lamar::OpCode::ADD:
        case lamar::OpCode::SUB:
        case lamar::OpCode::MUL:
        case lamar::OpCode::DIV:
        case lamar::OpCode::MOD:
        case lamar::OpCode::LT:
        case lamar::OpCode::LE:
        case lamar::OpCode::GT:
        case lamar::OpCode::GE:
        case lamar::OpCode::EQ:
        case lamar::OpCode::NE:
        case lamar::OpCode::AND:
        case lamar::OpCode::OR:
        case lamar::OpCode::CONST:
        case lamar::OpCode::STRING:
        case lamar::OpCode::SEXP:
        case lamar::OpCode::STI:
        case lamar::OpCode::STA:
        case lamar::OpCode::JMP:
        case lamar::OpCode::END:
        case lamar::OpCode::RET:
        case lamar::OpCode::DROP:
        case lamar::OpCode::DUP:
        case lamar::OpCode::SWAP:
        case lamar::OpCode::ELEM:
        case lamar::OpCode::LD_G:
        case lamar::OpCode::LD_L:
        case lamar::OpCode::LD_A:
        case lamar::OpCode::LD_C:
        case lamar::OpCode::LDA_G:
        case lamar::OpCode::LDA_L:
        case lamar::OpCode::LDA_A:
        case lamar::OpCode::LDA_C:
        case lamar::OpCode::ST_G:
        case lamar::OpCode::ST_L:
        case lamar::OpCode::ST_A:
        case lamar::OpCode::ST_C:
        case lamar::OpCode::CJMPZ:
        case lamar::OpCode::CJMPNZ:
        case lamar::OpCode::BEGIN:
        case lamar::OpCode::CBEGIN:
        case lamar::OpCode::CLOSURE:
        case lamar::OpCode::CALLC:
        case lamar::OpCode::CALL:
        case lamar::OpCode::TAG:
        case lamar::OpCode::ARRAY:
        case lamar::OpCode::FAIL:
        case lamar::OpCode::LINE:
        case lamar::OpCode::PATTEQSTR:
        case lamar::OpCode::PATTSTRING:
        case lamar::OpCode::PATTARRAY:
        case lamar::OpCode::PATTSEXP:
        case lamar::OpCode::PATTREF:
        case lamar::OpCode::PATTVAL:
        case lamar::OpCode::PATTFUN:
        case lamar::OpCode::CALL_LREAD:
        case lamar::OpCode::CALL_LWRITE:
        case lamar::OpCode::CALL_LLENGTH:
        case lamar::OpCode::CALL_LSTRING:
        case lamar::OpCode::CALL_BARRAY:
        case lamar::OpCode::EOF_:
            return true;
        default:
            return false;
    }
}

bool lamar::is_valid_closure_arg_type(uint8_t сlosure_arg_type) {
    switch (сlosure_arg_type) {
        case lamar::ClosureArgType::Global:
        case lamar::ClosureArgType::Local:
        case lamar::ClosureArgType::Arg:
        case lamar::ClosureArgType::Capture:
            return true;
        default:
            return false;
    }
    return false;
}
