//
// Created by Nikita Morozov on 29.01.2026.
//

#ifndef LAMAR_INTERPRETER_HPP
#define LAMAR_INTERPRETER_HPP

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"

#include "runtime.hpp"
#include "bytecode.hpp"
#include "value.hpp"
#include "frame.hpp"

#define FRAME_STACK_GROWTH 3
#define NEW_FRAME_STEP 40
#define ZERO BOX(0)

namespace lamar {

    class Interpreter {
    public:
        explicit Interpreter(ByteFile &&byteFile);

        void interpret();

    private:

        inline void push_error_diagnostic(std::string_view message);

        inline void init_stack();

        inline void push(Value v);

        inline Value pop();

        inline Value peek(uint32_t offset = 0);

        ptrdiff_t stack_size();

        inline uint32_t read_uint();

        inline uint8_t read_uint8();

        Value get_local(int32_t address);

        Value get_arg(int32_t address);

        Value get_capture(int32_t address);

        Value get_local_address(int32_t address);

        Value get_arg_address(int32_t address);

        Value get_capture_address(int32_t address);

        void set_local(int32_t address, Value& value);

        void set_arg(int32_t address, Value& value);

        void set_capture(int32_t address, Value& value);

        inline void interpret_add();

        inline void interpret_sub();

        inline void interpret_mul();

        inline void interpret_div();

        inline void interpret_mod();

        inline void interpret_lt();

        inline void interpret_le();

        inline void interpret_gt();

        inline void interpret_ge();

        inline void interpret_eq();

        inline void interpret_ne();

        inline void interpret_and();

        inline void interpret_or();

        inline void interpret_const();

        inline void interpret_string();

        inline void interpret_s_exp();

        inline void interpret_sti();

        inline void interpret_sta();

        inline void interpret_jmp();

        inline void interpret_end();

        inline void interpret_drop();

        inline void interpret_dup();

        inline void interpret_swap();

        inline void interpret_elem();

        inline void interpret_ld_g();

        inline void interpret_ld_l();

        inline void interpret_ld_a();

        inline void interpret_ld_c();

        inline void interpret_lda_g();

        inline void interpret_lda_l();

        inline void interpret_lda_a();

        inline void interpret_lda_c();

        inline void interpret_st_g();

        inline void interpret_st_l();

        inline void interpret_st_a();

        inline void interpret_st_c();

        inline void interpret_cjmpz();

        inline void interpret_cjmpnz();

        inline void interpret_begin();

        inline void interpret_cbegin();

        inline void interpret_closure();

        inline void interpret_callc();

        inline void interpret_call();

        inline void interpret_tag();

        inline void interpret_array();

        inline void interpret_fail();

        inline void interpret_line();

        inline void interpret_patteqstr();

        inline void interpret_pattstring();

        inline void interpret_pattarray();

        inline void interpret_pattsexp();

        inline void interpret_pattref();

        inline void interpret_pattval();

        inline void interpret_pattfun();

        inline void interpret_call_lread();

        inline void interpret_call_lwrite();

        inline void interpret_call_llength();

        inline void interpret_call_lstring();

        inline void interpret_call_barray();

        inline void interpret_eof();

        uint32_t ip = 0;
        std::vector<auint> stack_{};
        std::vector<Frame> call_stack_{};
        ByteFile byte_file_;
    };
}


#pragma clang diagnostic pop
#endif //LAMAR_INTERPRETER_HPP

