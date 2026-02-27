//
// Created by Nikita Morozov on 29.01.2026.
//

#include "interpreter.hpp"
#include "diagnostics.hpp"
#include <iostream>


void lamar::Interpreter::interpret() {
    __init();
    init_stack();

    auto &code = byte_file_.program_code;
    while (ip < code.size()) {
        auto instr = code.at(ip++);

        switch (instr) {
            case ADD:
                interpret_add();
                break;
            case SUB:
                interpret_sub();
                break;
            case MUL:
                interpret_mul();
                break;
            case DIV:
                interpret_div();
                break;
            case MOD:
                interpret_mod();
                break;
            case LT:
                interpret_lt();
                break;
            case LE:
                interpret_le();
                break;
            case GT:
                interpret_gt();
                break;
            case GE:
                interpret_ge();
                break;
            case EQ:
                interpret_eq();
                break;
            case NE:
                interpret_ne();
                break;
            case AND:
                interpret_and();
                break;
            case OR:
                interpret_or();
                break;
            case CONST:
                interpret_const();
                break;
            case STRING:
                interpret_string();
                break;
            case SEXP:
                interpret_s_exp();
                break;
            case STI:
                interpret_sti();
                break;
            case STA:
                interpret_sta();
                break;
            case JMP:
                interpret_jmp();
                break;
            case END:
            case RET:
                interpret_end();
                break;
            case DROP:
                interpret_drop();
                break;
            case DUP:
                interpret_dup();
                break;
            case SWAP:
                interpret_swap();
                break;
            case ELEM:
                interpret_elem();
                break;
            case LD_G:
                interpret_ld_g();
                break;
            case LD_L:
                interpret_ld_l();
                break;
            case LD_A:
                interpret_ld_a();
                break;
            case LD_C:
                interpret_ld_c();
                break;
            case LDA_G:
                interpret_lda_g();
                break;
            case LDA_L:
                interpret_lda_l();
                break;
            case LDA_A:
                interpret_lda_a();
                break;
            case LDA_C:
                interpret_lda_c();
                break;
            case ST_G:
                interpret_st_g();
                break;
            case ST_L:
                interpret_st_l();
                break;
            case ST_A:
                interpret_st_a();
                break;
            case ST_C:
                interpret_st_c();
                break;
            case CJMPZ:
                interpret_cjmpz();
                break;
            case CJMPNZ:
                interpret_cjmpnz();
                break;
            case BEGIN:
                interpret_begin();
                break;
            case CBEGIN:
                interpret_cbegin();
                break;
            case CLOSURE:
                interpret_closure();
                break;
            case CALLC:
                interpret_callc();
                break;
            case CALL:
                interpret_call();
                break;
            case TAG:
                interpret_tag();
                break;
            case ARRAY:
                interpret_array();
                break;
            case FAIL:
                interpret_fail();
                break;
            case LINE:
                interpret_line();
                break;
            case PATTEQSTR:
                interpret_patteqstr();
                break;
            case PATTSTRING:
                interpret_pattstring();
                break;
            case PATTARRAY:
                interpret_pattarray();
                break;
            case PATTSEXP:
                interpret_pattsexp();
                break;
            case PATTREF:
                interpret_pattref();
                break;
            case PATTVAL:
                interpret_pattval();
                break;
            case PATTFUN:
                interpret_pattfun();
                break;
            case CALL_LREAD:
                interpret_call_lread();
                break;
            case CALL_LWRITE:
                interpret_call_lwrite();
                break;
            case CALL_LLENGTH:
                interpret_call_llength();
                break;
            case CALL_LSTRING:
                interpret_call_lstring();
                break;
            case CALL_BARRAY:
                interpret_call_barray();
                break;
            case EOF_:
                ip = Frame::ninit;
                break;
        }
    }

    __shutdown();
}

lamar::Interpreter::Interpreter(lamar::ByteFile &&byte_file, auint *stack, lamar::Frame *call_stack)
        : stack_(stack),
          call_stack_(call_stack),
          byte_file_(std::move(byte_file)) {}





