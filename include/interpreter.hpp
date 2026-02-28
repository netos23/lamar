//
// Created by Nikita Morozov on 29.01.2026.
//

#ifndef LAMAR_INTERPRETER_HPP
#define LAMAR_INTERPRETER_HPP

#include <iostream>
#include <cstring>
#include "diagnostics.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"

#include "runtime.hpp"
#include "bytecode.hpp"
#include "value.hpp"
#include "frame.hpp"
#include "utils.hpp"

namespace lamar {

    class Interpreter {
    public:
        Interpreter(ByteFile &&byte_file, auint *stack, Frame *call_stack);

        void interpret();

    private:


        void push_error_diagnostic(std::string_view message) const {
            diagnostics::push_error_diagnostic(message, ip);
        }

        size_t stack_size() { // NOLINT(*-convert-member-functions-to-static)
            return __gc_stack_bottom - __gc_stack_top;
        }

        void init_stack() {
            auto size = byte_file_.global_area_size + 2;
            __gc_stack_top = stack_;
            __gc_stack_bottom = stack_ + size;

            std::memset(stack_, 0, size * sizeof(auint));

            // main frame
            call_stack_[++fp] = Frame(
                    Frame::ninit,
                    uint32_t(stack_size()),
                    0,
                    2,
                    false
            );

        }

        void push(Value v) {
#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (stack_size() + 1 >= MAX_STACK_SIZE) {
                push_error_diagnostic("Stack overflow");

            }
#endif

            *(__gc_stack_bottom++) = v.as_repr();
        }

        Value pop() {
#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (stack_size() <= 0) {
                push_error_diagnostic("No such element on stack to pop");
            }
#endif

            return Value(*(--__gc_stack_bottom));
        }

        Value peek(uint32_t offset = 0) {
#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (stack_size() <= 0) {
                push_error_diagnostic("No such element on stack to peek");
            }
#endif

            return Value(stack_[stack_size() - 1 - offset]);
        }

        uint32_t read_uint() {
#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (ip + sizeof(uint32_t) > byte_file_.program_code.size()) {
                push_error_diagnostic("Index out of bounds, while read constant from code");
            }
#endif

            uint32_t res = 0;
            std::memcpy(&res, byte_file_.program_code.data() + ip, sizeof(uint32_t));
            ip += sizeof(uint32_t);

            return res;
        }

        uint8_t read_uint8() {
#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (ip + sizeof(uint8_t) > byte_file_.program_code.size()) {
                push_error_diagnostic("Index out of bounds, while read constant from code");
            }
#endif

            return byte_file_.program_code.at(ip++);
        }

        Value get_local(int32_t address) {
            auto &frame = call_stack_[fp];
#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (static_cast<uint32_t>(address) >= frame.get_locals_count()) {
                push_error_diagnostic("Local index out of bounds");
            }
#endif
            auto val = static_cast<auint>(stack_[frame.get_sp() + address]);
            return Value(val);
        }

        Value get_arg(int32_t address) {
            auto &frame = call_stack_[fp];

            auto val = static_cast<auint>(stack_[frame.get_sp() - frame.get_args_count() + address]);
            return Value(val);
        }

        Value get_capture(int32_t address) {
            auto closure = get_arg(-1);
#ifndef DISABLE_RUNTIME_CHECKS
            if (!closure.is_closure()) {
                push_error_diagnostic("Expected closure for capture access");
            }
#endif
            auto closure_data = TO_DATA(closure.as_ptr());
            auto captures = reinterpret_cast<auint *>(closure_data->contents);
#ifndef DISABLE_RUNTIME_CHECKS
            auto captures_count = LEN(closure.as_ptr()) - 1;
            if (static_cast<uint32_t>(address) >= captures_count) {
                push_error_diagnostic("Capture index out of bounds");
            }
#endif
            return Value(captures[address + 1]);
        }

        Value get_local_address(int32_t address) {
            auto &frame = call_stack_[fp];
#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (static_cast<uint32_t>(address) >= frame.get_locals_count()) {
                push_error_diagnostic("Local index out of bounds");
            }
#endif
            auto val = &stack_[frame.get_sp() + address];
            return Value(val);
        }

        Value get_arg_address(int32_t address) {
            auto &frame = call_stack_[fp];
#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (address >= frame.get_args_count()) {
                push_error_diagnostic("Arg index out of bounds");
            }
#endif
            auto val = &stack_[frame.get_sp() - frame.get_args_count() + address];
            return Value(val);
        }

        Value get_capture_address(int32_t address) {
            auto closure = get_arg(-1);
#ifndef DISABLE_RUNTIME_CHECKS
            if (!closure.is_closure()) {
                push_error_diagnostic("Expected closure for capture address access");
            }
#endif
            auto closure_data = TO_DATA(closure.as_ptr());
            auto captures = reinterpret_cast<auint *>(closure_data->contents);
#ifndef DISABLE_RUNTIME_CHECKS
            auto captures_count = LEN(closure.as_ptr()) - 1;
            if (static_cast<uint32_t>(address) >= captures_count) {
                push_error_diagnostic("Capture index out of bounds");
            }
#endif
            return Value(&captures[address + 1]);
        }

        void set_local(int32_t address, Value &value) {
            auto &frame = call_stack_[fp];
#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (static_cast<uint32_t>(address) >= frame.get_locals_count()) {
                push_error_diagnostic("Local index out of bounds");
            }
#endif
            stack_[frame.get_sp() + address] = value.as_repr();
        }

        void set_arg(int32_t address, Value &value) {
            auto &frame = call_stack_[fp];
#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (address >= frame.get_args_count()) {
                push_error_diagnostic("Arg index out of bounds");
            }
#endif

            stack_[frame.get_sp() - frame.get_args_count() + address] = value.as_repr();
        }

        void set_capture(int32_t address, Value &value) {
            auto closure = get_arg(-1);
#ifndef DISABLE_RUNTIME_CHECKS
            if (!closure.is_closure()) {
                push_error_diagnostic("Expected closure for capture address access");
            }
#endif
            auto closure_data = TO_DATA(closure.as_ptr());
#ifndef DISABLE_RUNTIME_CHECKS
            auto captures_count = LEN(closure.as_ptr()) - 1;
            if (static_cast<uint32_t>(address) >= captures_count) {
                push_error_diagnostic("Capture index out of bounds");
            }
#endif
            auto captures = reinterpret_cast<auint *>(closure_data->contents);
            captures[address + 1] = value.as_repr();
        }

        void interpret_add() {
            interpret_binop([](aint lhs, aint rhs) { return lhs + rhs; });
        }

        void interpret_sub() {
            interpret_binop([](aint lhs, aint rhs) { return lhs - rhs; });
        }

        void interpret_mul() {
            interpret_binop([](aint lhs, aint rhs) { return lhs * rhs; });
        }

        void interpret_div() {
            interpret_binop(
                    [](aint lhs, aint rhs) { return lhs / rhs; },
#ifndef DISABLE_RUNTIME_CHECKS
                    [this](aint, aint rhs) {
                        if (rhs == 0) {
                            push_error_diagnostic("Division by zero");
                        }
                    }
#endif
            );
        }

        void interpret_mod() {
            interpret_binop(
                    [](aint lhs, aint rhs) { return lhs % rhs; },
#ifndef DISABLE_RUNTIME_CHECKS
                    [this](aint, aint rhs) {
                        if (rhs == 0) {
                            push_error_diagnostic("Modulo by zero");
                        }
                    }
#endif
            );
        }

        void interpret_lt() {
            interpret_binop([](aint lhs, aint rhs) -> aint { return lhs < rhs; });
        }

        void interpret_le() {
            interpret_binop([](aint lhs, aint rhs) -> aint { return lhs <= rhs; });
        }

        void interpret_gt() {
            interpret_binop([](aint lhs, aint rhs) -> aint { return lhs > rhs; });
        }

        void interpret_ge() {
            interpret_binop([](aint lhs, aint rhs) -> aint { return lhs >= rhs; });
        }

        void interpret_eq() {
            auto rhs = pop();
            auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
            if (!lhs.is_int() && !rhs.is_int()) {
                push_error_diagnostic("One or more operands are not integers");
            }
#endif

            aint res = 0;

            if (lhs.is_int() && rhs.is_int()) {
                res = lhs.as_int() == rhs.as_int();
            }
            push(Value(res));
        }

        void interpret_ne() {
            interpret_binop([](aint lhs, aint rhs) -> aint { return lhs != rhs; });
        }

        void interpret_and() {
            interpret_binop([](aint lhs, aint rhs) -> aint { return lhs && rhs; });
        }

        void interpret_or() {
            interpret_binop([](aint lhs, aint rhs) -> aint { return lhs || rhs; });
        }

        void interpret_const() {
            auto c = static_cast<aint>(read_uint());
            push(Value(c));
        }

        void interpret_string() {
            auto index = read_uint();

#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (index >= byte_file_.string_table_size) {
                push_error_diagnostic("String index out of bounds");
            }
#endif

            auto ptr = &byte_file_.string_table.get()[index];
            auto str = Bstring(reinterpret_cast<aint *>(&ptr));
            push(Value(str));
        }

        void interpret_s_exp() {
            auto tag = read_uint();
            auto size = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
            if (size > stack_size()) {
                push_error_diagnostic("Not enough elements on stack to create s-expression");
            }
#endif

#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (tag >= byte_file_.string_table_size) {
                push_error_diagnostic("String index out of bounds");
            }
#endif

            auto ptr = &byte_file_.string_table.get()[tag];
            auto tag_hash = LtagHash(ptr);

            push(Value(auint(tag_hash)));
            auto sp = __gc_stack_bottom - size - 1;
            auto sexp = Bsexp(reinterpret_cast<aint *>(sp), BOX(size + 1));
            __gc_stack_bottom = sp;

            push(Value(sexp));
        }

        void interpret_sti() {
            auto value = pop();
            auto reference = pop();

#ifndef DISABLE_RUNTIME_CHECKS
            if (!reference.is_boxed()) {
                push_error_diagnostic("Reference expected for sti");
            }
#endif

            auto ptr = reinterpret_cast<auint *>(reference.as_ptr());
            *ptr = value.as_repr();
            push(value);
        }

        void interpret_sta() {
            Value val = pop();
            Value index = pop();

            if (index.is_int()) {
                Value x = pop();
                auto i = index.as_int();

#ifndef DISABLE_RUNTIME_CHECKS
                if (!x.is_arr() && !x.is_str() && !x.is_s_expr()) {
                    push_error_diagnostic("Try to store not in array, string, or sexp");
                }

                if (x.size() <= i || i < 0) {
                    push_error_diagnostic("Index out of bounds");
                }
#endif

                Bsta(x.as_ptr(), aint(index.as_repr()), val.as_ptr());
            } else {

#ifndef DISABLE_RUNTIME_CHECKS
                if (!index.is_boxed()) {
                    push_error_diagnostic("Reference expected for sti");
                }
#endif
                auto ptr = reinterpret_cast<auint *>(index.as_ptr());
                *ptr = val.as_repr();
            }

            push(val);
        }

        void interpret_jmp() {
            auto location = read_uint();

#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (byte_file_.program_code.size() <= location) {
                push_error_diagnostic("Jump out of bounds");
            }
#endif

            ip = location;
        }

        void interpret_end() {
            auto value = pop();
            auto &frame = call_stack_[fp];

            auto next_sp = frame.is_closure()
                           ? frame.get_sp() - frame.get_args_count() - 1
                           : frame.get_sp() - frame.get_args_count();

            __gc_stack_bottom = __gc_stack_top + next_sp;

            auto prev_ip = frame.get_return_address();
            ip = prev_ip;

            fp--;

            push(value);
        }

        void interpret_drop() {
            pop();
        }

        void interpret_dup() {
            auto value = peek();
            push(value);
        }

        void interpret_swap() {
            auto a = pop();
            auto b = pop();

            push(b);
            push(a);
        }

        void interpret_elem() {
            auto index = pop();

#ifndef DISABLE_RUNTIME_CHECKS
            if (!index.is_int()) {
                push_error_diagnostic("Index must be an integer");
            }
#endif

            auto x = pop();

#ifndef DISABLE_RUNTIME_CHECKS
            if (!x.is_arr() && !x.is_str() && !x.is_s_expr()) {
                push_error_diagnostic("Try to get element not from array, string, or sexp");
            }
#endif

            auto value = Belem(x.as_ptr(), aint(index.as_repr()));

            push(Value(value));
        }

        void interpret_ld_g() {
            interpret_load("Global index out of stack bounds", [this](uint32_t address) {
                return Value(stack_[address]);
            });
        }

        void interpret_ld_l() {
            interpret_load("Local index out of stack bounds", [this](uint32_t address) {
                return get_local(static_cast<int32_t>(address));
            });
        }

        void interpret_ld_a() {
            interpret_load("Arg index out of stack bounds", [this](uint32_t address) {
                return get_arg(static_cast<int32_t>(address));
            });
        }

        void interpret_ld_c() {
            interpret_load("Capture index out of stack bounds", [this](uint32_t address) {
                return get_capture(static_cast<int32_t>(address));
            });
        }

        void interpret_lda_g() {
            interpret_load_address("Global index out of bounds", [this](uint32_t address) {
                return Value(&stack_[address]);
            });
        }

        void interpret_lda_l() {
            interpret_load_address("Local index out of stack bounds", [this](uint32_t address) {
                return get_local_address(static_cast<int32_t>(address));
            });
        }

        void interpret_lda_a() {
            interpret_load_address("Arg index out of stack bounds", [this](uint32_t address) {
                return get_arg_address(static_cast<int32_t>(address));
            });
        }

        void interpret_lda_c() {
            interpret_load_address("Capture index out of stack bounds", [this](uint32_t address) {
                return get_capture_address(static_cast<int32_t>(address));
            });
        }

        void interpret_st_g() {
            interpret_store("Global index out of stack bounds", [this](uint32_t address, Value &value) {
                stack_[address] = value.as_repr();
            });
        }

        void interpret_st_l() {
            interpret_store("Local index out of stack bounds", [this](uint32_t address, Value &value) {
                set_local(static_cast<int32_t>(address), value);
            });
        }

        void interpret_st_a() {
            interpret_store("Arg index out of stack bounds", [this](uint32_t address, Value &value) {
                set_arg(static_cast<int32_t>(address), value);
            });
        }

        void interpret_st_c() {
            interpret_store("Capture index out of stack bounds", [this](uint32_t address, Value &value) {
                set_capture(static_cast<int32_t>(address), value);
            });
        }

        void interpret_conditional_jump(bool jump_on_true) {
            auto location = read_uint();

#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (byte_file_.program_code.size() <= location) {
                push_error_diagnostic("Jump out of bounds");
            }
#endif

            Value condition = pop();

#ifndef DISABLE_RUNTIME_CHECKS
            if (!condition.is_int()) {
                push_error_diagnostic("Condition should be integer");
            }
#endif

            if ((condition.as_int() != 0) == jump_on_true) {
                ip = location;
            }
        }

        void interpret_cjmpz() {
            interpret_conditional_jump(false);
        }

        void interpret_cjmpnz() {
            interpret_conditional_jump(true);
        }

        void interpret_begin() {
            auto args_count = read_uint();
            auto locals_count = read_uint();

            auto &frame = call_stack_[fp];

#ifndef DISABLE_RUNTIME_CHECKS
            if (args_count != frame.get_args_count()) {
                push_error_diagnostic("Incorrect arguments count");
            }

            frame.set_locals_count(locals_count);
#endif

#ifdef ENABLE_VERIFICATION
            auto required_size = ((args_count >> 16) & 0xFFFFu);

            if (stack_size() + required_size >= MAX_STACK_SIZE) {
                push_error_diagnostic("Not enough stack space for procedure execution");
            }
#endif

            for (uint32_t i = 0; i < locals_count; i++) {
                push(Value(auint(0)));
            }
        }

        void interpret_cbegin() {
            auto args_count = read_uint();
            auto locals_count = read_uint();

            auto &frame = call_stack_[fp];

#ifndef DISABLE_RUNTIME_CHECKS
            if (args_count != frame.get_args_count()) {
                push_error_diagnostic("Incorrect arguments count");
            }

            frame.set_locals_count(locals_count);
#endif

#ifdef ENABLE_VERIFICATION
            auto required_size = ((args_count >> 16) & 0xFFFFu);

            if (stack_size() + required_size >= MAX_STACK_SIZE) {
                push_error_diagnostic("Not enough stack space for procedure execution");
            }
#endif

            for (uint32_t i = 0; i < locals_count; i++) {
                push(Value(auint(0)));
            }
        }

        void interpret_closure() {
            auto address = read_uint();

#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (byte_file_.program_code.size() <= address) {
                push_error_diagnostic("Closure address out of bounds");
            }
#endif

            auto count = read_uint();

            auto sp = __gc_stack_bottom;
            push(Value(auint(address)));

            for (int i = 0; i < count; i++) {
                auto type = static_cast<ClosureArgType>(read_uint8());

                auto arg_address = int32_t(read_uint());

                switch (type) {
                    case Global:
                        push(Value(auint(stack_[arg_address])));
                        break;
                    case Local:
                        push(Value(auint(get_local(arg_address).as_repr())));
                        break;
                    case Arg:
                        push(Value(auint(get_arg(arg_address).as_repr())));
                        break;
                    case Capture:
                        push(Value(auint(get_capture(arg_address).as_repr())));
                        break;
                }
            }

            auto closure = Bclosure(reinterpret_cast<aint *>(sp), BOX(count));
            __gc_stack_bottom = sp;
            push(Value(closure));
        }

        void interpret_callc() {
            auto args_count = read_uint();

            auto closure = peek(args_count);
#ifndef DISABLE_RUNTIME_CHECKS
            if (!closure.is_closure()) {
                push_error_diagnostic("Target code isn't closure");
            }
#endif
            auto closure_data = TO_DATA(closure.as_ptr());
            auto captured = reinterpret_cast<auint *>(closure_data->contents);

            auto closure_address = static_cast<int32_t>(captured[0]);

#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (byte_file_.program_code.size() <= closure_address) {
                push_error_diagnostic("Closure address out of bounds");
            }
#endif

#ifndef DISABLE_RUNTIME_CHECKS
            auto instr = byte_file_.program_code.at(closure_address);
            if (instr != BEGIN && instr != CBEGIN) {
                push_error_diagnostic("Closure entry must be begin or cbegin");
            }

            if (fp + 1 >= MAX_CALL_STACK_SIZE) {
                push_error_diagnostic("Call stack overflow");
            }
#endif


            call_stack_[++fp] = Frame(
                    ip,
                    uint32_t(stack_size()),
                    closure_address,
                    args_count,
                    true
            );

            ip = closure_address;
        }

        void interpret_call() {
            auto proc_address = read_uint();

#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (byte_file_.program_code.size() <= proc_address) {
                push_error_diagnostic("Closure address out of bounds");
            }

            auto instr = byte_file_.program_code.at(proc_address);
            if (instr != BEGIN && instr != CBEGIN) {
                push_error_diagnostic("Procedure entry must be begin or cbegin");
            }
#endif

#ifndef DISABLE_RUNTIME_CHECKS
            if (fp + 1 >= MAX_CALL_STACK_SIZE) {
                push_error_diagnostic("Call stack overflow");
            }
#endif

            auto args_count = read_uint();

            call_stack_[++fp] = Frame(
                    ip,
                    uint32_t(stack_size()),
                    proc_address,
                    args_count,
                    false
            );

            ip = proc_address;
        }

        void interpret_tag() {
            auto tag = read_uint();
            auto size = read_uint();

#if !defined(DISABLE_RUNTIME_CHECKS) && !defined(ENABLE_VERIFICATION)
            if (tag >= byte_file_.string_table_size) {
                push_error_diagnostic("String index out of bounds");
            }
#endif

            auto probe = pop();
            if (!probe.is_s_expr()) {
                push(Value(aint(0)));
                return;
            }

            auto ptr = &byte_file_.string_table.get()[tag];
            auto tag_hash = LtagHash(ptr);
            auto result = Btag(probe.as_ptr(), tag_hash, BOX(size));

            push(Value(static_cast<auint>(result)));
        }

        void interpret_array() {
            auto n = read_uint();

            auto value = pop();
            auto result = Barray_patt(value.as_ptr(), BOX(n));
            push(Value(static_cast<auint>(result)));
        }

        void interpret_fail() {
            auto line = read_uint();
            auto column = read_uint();
            auto value = pop();

            Bmatch_failure(value.as_ptr(), &byte_file_.file_name[0], line, column);
        }

        void interpret_line() {
            auto line = read_uint();
            auto &frame = call_stack_[fp];
            frame.set_line(line);
        }

        void interpret_patteqstr() {
            auto b = pop();
            auto a = pop();
            auto result = Bstring_patt(a.as_ptr(), b.as_ptr());

            push(Value(static_cast<auint>(result)));
        }

        void interpret_pattstring() {
            auto value = pop();
            auto result = Bstring_tag_patt(value.as_ptr());

            push(Value(static_cast<auint>(result)));
        }

        void interpret_pattarray() {
            auto value = pop();
            auto result = Barray_tag_patt(value.as_ptr());

            push(Value(static_cast<auint>(result)));
        }

        void interpret_pattsexp() {
            auto value = pop();
            auto result = Bsexp_tag_patt(value.as_ptr());

            push(Value(static_cast<auint>(result)));
        }

        void interpret_pattref() {
            auto value = pop();
            auto result = Bboxed_patt(value.as_ptr());

            push(Value(static_cast<auint>(result)));
        }

        void interpret_pattval() {
            auto value = pop();
            auto result = Bunboxed_patt(value.as_ptr());

            push(Value(static_cast<auint>(result)));
        }

        void interpret_pattfun() {
            auto value = pop();
            auto result = Bclosure_tag_patt(value.as_ptr());

            push(Value(static_cast<auint>(result)));
        }

        void interpret_call_lread() {
            auto value = Lread();
            push(Value(static_cast<auint>(value)));
        }

        void interpret_call_lwrite() {
            auto value = pop();

#ifndef DISABLE_RUNTIME_CHECKS
            if (!value.is_int()) {
                push_error_diagnostic("Only integer write supported");
            }
#endif

            auto result = Lwrite(static_cast<aint>(value.as_repr()));

            push(Value(static_cast<auint>(result)));
        }

        void interpret_call_llength() {
            auto x = pop();

#ifndef DISABLE_RUNTIME_CHECKS
            if (!x.is_arr() && !x.is_str() && !x.is_s_expr()) {
                push_error_diagnostic("Try to get length not in array, string, or sexp");
            }
#endif

            auto len = Llength(x.as_ptr());
            push(Value(static_cast<auint>(len)));
        }

        void interpret_call_lstring() {
            auto value = pop();
            void *ptr = value.as_ptr();
            auto str = Lstring(reinterpret_cast<aint *>(&ptr));
            push(Value(str));
        }

        void interpret_call_barray() {
            auto len = read_uint();
#ifndef DISABLE_RUNTIME_CHECKS
            if (len > stack_size()) {
                push_error_diagnostic("Not enough elements on stack for array creation");
            }
#endif

            auto sp = __gc_stack_bottom - len;
            void *v = Barray(reinterpret_cast<aint *>(sp), BOX(len));
            __gc_stack_bottom = sp;
            push(Value(v));
        }


        template<typename Operation, typename ExtraCheck>
        void interpret_binop(Operation op, ExtraCheck extra_check) {
            auto rhs = pop();
            auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
            if (!lhs.is_int() || !rhs.is_int()) {
                push_error_diagnostic("One or more operands are not integers");
            }
#endif

            const auto l = lhs.as_int();
            const auto r = rhs.as_int();

#ifndef DISABLE_RUNTIME_CHECKS
            extra_check(l, r);
#endif

            push(Value(op(l, r)));
        }

        template<typename Operation>
        void interpret_binop(Operation op) {
            interpret_binop(op, [](aint, aint) {});
        }

        template<typename Getter>
        void interpret_load(std::string_view bounds_error, Getter getter) {
            auto address = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
            if (stack_size() <= address) {
                push_error_diagnostic(bounds_error);
            }
#endif

            push(getter(address));
        }

        template<typename Getter>
        void interpret_load_address(std::string_view bounds_error, Getter getter) {
            auto address = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
            if (stack_size() <= address) {
                push_error_diagnostic(bounds_error);
            }
#endif

            push(getter(address));
        }

        template<typename Setter>
        void interpret_store(std::string_view bounds_error, Setter setter) {
            auto value = pop();
            auto address = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
            if (stack_size() <= address) {
                push_error_diagnostic(bounds_error);
            }
#endif

            setter(address, value);
            push(value);
        }


        uint32_t ip = 0;
        uint32_t fp = -1;
        auint *stack_;
        Frame *call_stack_;
        ByteFile byte_file_;
    };

}


#pragma clang diagnostic pop
#endif //LAMAR_INTERPRETER_HPP

