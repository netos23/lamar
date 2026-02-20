//
// Created by Nikita Morozov on 29.01.2026.
//

#include "interpreter.hpp"


lamar::Interpreter::Interpreter(lamar::ByteFile &&byteFile) : byte_file_(std::move(byteFile)) {}

void lamar::Interpreter::interpret() {
    __init();
    init_stack();

    std::vector<OpCode> &code = byte_file_.program_code;
    auto eof_marker = false;
    while (ip < code.size() && ! eof_marker) {
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
                interpret_end();
                break;
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
                eof_marker = true;
                break;
        }
    }

    __shutdown();
}


void lamar::Interpreter::push_error_diagnostic(std::string_view message) {
    std::string formated_message = std::string(message) + "At: %#08X\\n";

    char *formated_message_data = const_cast<char *>(formated_message.c_str());
    failure(formated_message_data, ip);
}

ptrdiff_t lamar::Interpreter::stack_size() {
    return __gc_stack_bottom - __gc_stack_top;
}

void lamar::Interpreter::init_stack() {
    size_t stack_size = byte_file_.global_area_size + 2;
    stack_.resize(stack_size, ZERO);
    __gc_stack_top = stack_.data();
    __gc_stack_bottom = stack_.data() + stack_size;

    call_stack_.reserve(100);
    // main frame
    call_stack_.emplace_back(
            Frame::ninit,
            uint32_t(Interpreter::stack_size()),
            0,
            2,
            false
    );

}

void lamar::Interpreter::push(lamar::Value v) {
    if (stack_size() + 1 >= stack_.size()) {
        stack_.resize(stack_.size() * 2, ZERO);
    }
    //                    push_error_diagnostic()
    *(__gc_stack_bottom++) = v.as_repr();
}

lamar::Value lamar::Interpreter::pop() {
#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_size() <= 0) {
        push_error_diagnostic("No such element on stack to pop");
    }
#endif

    return lamar::Value(*(--__gc_stack_bottom));
}

lamar::Value lamar::Interpreter::peek(uint32_t offset) {
#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_size() <= 0) {
        push_error_diagnostic("No such element on stack to pop");
    }
#endif

    return lamar::Value(*(__gc_stack_bottom - 1 - offset));
}

uint32_t lamar::Interpreter::read_uint() {
#ifndef DISABLE_RUNTIME_CHECKS
    if (ip + sizeof(uint32_t) > byte_file_.program_code.size()) {
        push_error_diagnostic("Index out of bounds, while read constant from code");
    }
#endif

    uint32_t res = 0;
    std::memcpy(&res, byte_file_.program_code.data() + ip, sizeof(uint32_t));
    ip += sizeof(uint32_t);

    return res;
}

uint32_t lamar::Interpreter::read_uint8() {
#ifndef DISABLE_RUNTIME_CHECKS
    if (ip + sizeof(uint8_t) > byte_file_.program_code.size()) {
        push_error_diagnostic("Index out of bounds, while read constant from code");
    }
#endif

    uint8_t res = byte_file_.program_code.at(ip++);
    return res;
}

void lamar::Interpreter::interpret_add() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    push(Value(lhs.as_int() + rhs.as_int()));
}

void lamar::Interpreter::interpret_sub() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    push(Value(lhs.as_int() - rhs.as_int()));
}

void lamar::Interpreter::interpret_mul() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    push(Value(lhs.as_int() * rhs.as_int()));
}

void lamar::Interpreter::interpret_div() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    auto divisor = rhs.as_int();

#ifndef DISABLE_RUNTIME_CHECKS
    if (divisor == 0) {
        push_error_diagnostic("Division by zero");
    }
#endif

    push(Value(lhs.as_int() / divisor));
}

void lamar::Interpreter::interpret_mod() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    auto modulo = rhs.as_int();

#ifndef DISABLE_RUNTIME_CHECKS
    if (modulo == 0) {
        push_error_diagnostic("Modulo by zero");
    }
#endif

    push(Value(lhs.as_int() % modulo));
}

void lamar::Interpreter::interpret_lt() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    aint res = lhs.as_int() < rhs.as_int();
    push(Value(res));
}

void lamar::Interpreter::interpret_le() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    aint res = lhs.as_int() <= rhs.as_int();
    push(Value(res));
}

void lamar::Interpreter::interpret_gt() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    aint res = lhs.as_int() > rhs.as_int();
    push(Value(res));
}

void lamar::Interpreter::interpret_ge() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    aint res = lhs.as_int() >= rhs.as_int();
    push(Value(res));
}

void lamar::Interpreter::interpret_eq() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    aint res = lhs.as_int() == rhs.as_int();
    push(Value(res));
}

void lamar::Interpreter::interpret_ne() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    aint res = lhs.as_int() != rhs.as_int();
    push(Value(res));
}

void lamar::Interpreter::interpret_and() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    aint res = lhs.as_int() && rhs.as_int();
    push(Value(res));
}

void lamar::Interpreter::interpret_or() {
    auto rhs = pop();
    auto lhs = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!lhs.is_int() || !rhs.is_int()) {
        push_error_diagnostic("One or more operands are not integers");
    }
#endif

    aint res = lhs.as_int() || rhs.as_int();
    push(Value(res));
}

void lamar::Interpreter::interpret_const() {
    auto c = static_cast<aint>(read_uint());
    push(Value(c));
}

void lamar::Interpreter::interpret_string() {
    auto index = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
    if (index <= 0 || index >= byte_file_.string_table_size) {
        push_error_diagnostic("String index out of bounds");
    }
#endif

    auto ptr = &byte_file_.string_table.get()[index];
    auto str = Bstring(reinterpret_cast<aint *>(ptr));
    push(Value(str));
}

void lamar::Interpreter::interpret_s_exp() {
    auto tag = read_uint();
    auto size = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
    if (size < 0) {
        push_error_diagnostic("One or more operands are not integers");
    }

    if (tag <= 0 || tag >= byte_file_.string_table_size) {
        push_error_diagnostic("String index out of bounds");
    }
#endif

    auto ptr = &byte_file_.string_table.get()[tag];
    auto tag_hash = LtagHash(ptr);

    std::vector<aint> args;
    args.reserve(size + 1);
    for (int i = static_cast<int>(size - 1); i >= 0; --i) {
        auto arg = pop();
        args[i] = static_cast<aint>(arg.as_repr());
    }

    args[size] = tag_hash;
    auto sexp = Bsexp(args.data(), BOX(size + 1));
    push(Value(sexp));
}

void lamar::Interpreter::interpret_sti() {
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

void lamar::Interpreter::interpret_sta() {
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

        Bsta(x.as_ptr(), i, val.as_ptr());
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


inline void lamar::Interpreter::interpret_jmp() {
    auto location = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
    if (byte_file_.program_code.size() <= location || location <= 0) {
        push_error_diagnostic("Jump out of bounds");
    }
#endif

    ip = location;
}

inline void lamar::Interpreter::interpret_end() {
    auto value = pop();
    decltype(auto) frame = call_stack_.back();

    auto next_sp = frame.is_closure()
                   ? frame.get_sp() - frame.get_args_count() - 1
                   : frame.get_sp() - frame.get_args_count();


    __gc_stack_bottom = static_cast<auint *>(__gc_stack_top + next_sp);

    auto prev_ip = frame.get_return_addres();
    if(prev_ip != Frame::ninit){
        ip = prev_ip;
    }
    call_stack_.pop_back();

    push(value);
}


inline void lamar::Interpreter::interpret_drop() {
    pop();
}

inline void lamar::Interpreter::interpret_dup() {
    auto value = peek();
    push(value);
}

inline void lamar::Interpreter::interpret_swap() {
    auto a = pop();
    auto b = pop();

    push(b);
    push(a);
}

inline void lamar::Interpreter::interpret_elem() {
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

    auto value = Belem(x.as_ptr(), index.as_int());

    push(Value(value));
}

inline void lamar::Interpreter::interpret_ld_g() {
    auto address = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_.size() <= address || address < 0) {
        push_error_diagnostic("Global index out of bounds");
    }
#endif

    auto global = stack_[address];
    push(Value(global));
}

inline void lamar::Interpreter::interpret_ld_l() {
    auto address = read_uint();
#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_.size() <= address || address < 0) {
        push_error_diagnostic("Local index out of bounds");
    }
#endif

    auto local = get_local(address);
    push(local);
}

inline void lamar::Interpreter::interpret_ld_a() {
    auto address = read_uint();
#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_.size() <= address || address < 0) {
        push_error_diagnostic("Arg index out of bounds");
    }
#endif

    auto arg = get_arg(address);
    push(arg);
}

inline void lamar::Interpreter::interpret_ld_c() {
    auto address = read_uint();
#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_.size() <= address || address < 0) {
        push_error_diagnostic("Capture index out of bounds");
    }
#endif

    auto capture = get_capture(address);
    push(capture);
}

inline void lamar::Interpreter::interpret_lda_g() {
    auto address = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_.size() <= address || address < 0) {
        push_error_diagnostic("Global index out of bounds");
    }
#endif

    auto global = &stack_[address];
    push(Value(global));
}

inline void lamar::Interpreter::interpret_lda_l() {
    auto address = read_uint();
#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_.size() <= address || address < 0) {
        push_error_diagnostic("Local index out of bounds");
    }
#endif

    auto local = get_local_address(address);
    push(local);
}

inline void lamar::Interpreter::interpret_lda_a() {
    auto address = read_uint();
#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_.size() <= address || address < 0) {
        push_error_diagnostic("Arg index out of bounds");
    }
#endif

    auto arg = get_arg_address(address);
    push(arg);
}

inline void lamar::Interpreter::interpret_lda_c() {
    auto address = read_uint();
#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_.size() <= address || address < 0) {
        push_error_diagnostic("Capture index out of bounds");
    }
#endif

    auto capture = get_capture_address(address);
    push(capture);
}

inline void lamar::Interpreter::interpret_st_g() {
    auto value = pop();
    auto address = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_.size() <= address || address < 0) {
        push_error_diagnostic("Global index out of bounds");
    }
#endif

    stack_[address] = value.as_repr();
    push(value);
}

inline void lamar::Interpreter::interpret_st_l() {
    auto value = pop();
    auto address = read_uint();
#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_.size() <= address || address < 0) {
        push_error_diagnostic("Local index out of bounds");
    }
#endif

    set_local(address, value);
    push(value);
}

inline void lamar::Interpreter::interpret_st_a() {
    auto value = pop();
    auto address = read_uint();
#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_.size() <= address || address < 0) {
        push_error_diagnostic("Arg index out of bounds");
    }
#endif

    set_arg(address, value);
    push(value);
}

inline void lamar::Interpreter::interpret_st_c() {
    auto value = pop();
    auto address = read_uint();
#ifndef DISABLE_RUNTIME_CHECKS
    if (stack_.size() <= address || address < 0) {
        push_error_diagnostic("Capture index out of bounds");
    }
#endif

    set_capture(address, value);
    push(value);
}

inline void lamar::Interpreter::interpret_cjmpz() {
    auto location = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
    if (byte_file_.program_code.size() <= location || location <= 0) {
        push_error_diagnostic("Jump out of bounds");
    }
#endif

    Value condition = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!condition.is_int()) {
        push_error_diagnostic("Condition should be integer");
    }
#endif

    if (!condition.as_int()) {
        ip = location;
    }
}

inline void lamar::Interpreter::interpret_cjmpnz() {
    auto location = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
    if (byte_file_.program_code.size() <= location || location <= 0) {
        push_error_diagnostic("Jump out of bounds");
    }
#endif

    Value condition = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!condition.is_int()) {
        push_error_diagnostic("Condition should be integer");
    }
#endif

    if (condition.as_int()) {
        ip = location;
    }
}

inline void lamar::Interpreter::interpret_begin() {
    auto args_count = read_uint();
    auto locals_count = read_uint();

    decltype(auto) frame = call_stack_.back();

#ifndef DISABLE_RUNTIME_CHECKS
    if (args_count != frame.get_args_count()) {
        push_error_diagnostic("Incorrect arguments count");
    }

    frame.set_locals_count(locals_count);
#endif

    for (uint32_t i = 0; i < locals_count; i++) {
        push(Value(auint(0)));
    }
}

inline void lamar::Interpreter::interpret_cbegin() {
    auto args_count = read_uint();
    auto locals_count = read_uint();

    decltype(auto) frame = call_stack_.back();

#ifndef DISABLE_RUNTIME_CHECKS
    if (args_count != frame.get_args_count()) {
        push_error_diagnostic("Incorrect arguments count");
    }

    frame.set_locals_count(locals_count);
#endif

    for (uint32_t i = 0; i < locals_count; i++) {
        push(Value(auint(0)));
    }
}

inline void lamar::Interpreter::interpret_closure() {
    auto address = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
    if (byte_file_.program_code.size() <= address || address <= 0) {
        push_error_diagnostic("Closure address out of bounds");
    }
#endif

    auto count = read_uint();


    std::vector<aint> args;
    args.reserve(count + 1);
    args[0] = static_cast<aint>(address);
    decltype(auto) frame = call_stack_.back();

    for (int i = 0; i < count; i++) {
        auto arg = pop();
        args[i] = static_cast<aint>(arg.as_repr());

        auto type = static_cast<ClosureArgType>(byte_file_.program_code.at(ip++));

        auto arg_address = read_uint8();

        switch (type) {
            case Global:
                args[i + 1] = static_cast<aint>(stack_[arg_address]);
                break;
            case Local:
                args[i + 1] = static_cast<aint>(get_local(arg_address).as_repr());
                break;
            case Arg:
                args[i + 1] = static_cast<aint>(get_arg(arg_address).as_repr());
                break;
            case Capture:
                args[i + 1] = static_cast<aint>(get_capture(arg_address).as_repr());
                break;
        }
    }

    auto closure = Bclosure(args.data(), BOX(count + 1));
    push(Value(closure));
}

inline void lamar::Interpreter::interpret_callc() {
    auto args_count = read_uint();

    auto closure = peek(args_count);
#ifndef DISABLE_RUNTIME_CHECKS
    if (!closure.is_closure()) {
        push_error_diagnostic("Target code isn`t closure");
    }
#endif
    auto closure_data = TO_DATA(closure.as_ptr());
    auto captured = reinterpret_cast<auint *>(closure_data->contents);

    auto closure_address = static_cast<int32_t>(captured[0]);

#ifndef DISABLE_RUNTIME_CHECKS
    if (byte_file_.program_code.size() <= closure_address || closure_address <= 0) {
        push_error_diagnostic("Closure address out of bounds");
    }
#endif


    decltype(auto) frame = call_stack_.emplace_back(
            ip,
            uint32_t(stack_size()),
            closure_address,
            args_count,
            true
    );

    // todo check begin
    ip = closure_address;
}

inline void lamar::Interpreter::interpret_call() {
    auto proc_address = read_uint();
#ifndef DISABLE_RUNTIME_CHECKS
    if (byte_file_.program_code.size() <= proc_address || proc_address <= 0) {
        push_error_diagnostic("Closure address out of bounds");
    }
#endif

    auto args_count = read_uint();

    // todo check begin

    decltype(auto) frame = call_stack_.emplace_back(
            ip,
            uint32_t(stack_size()),
            proc_address,
            args_count,
            false
    );

    ip = proc_address;
}

inline void lamar::Interpreter::interpret_tag() {
    auto tag = read_uint();
    auto size = read_uint();

#ifndef DISABLE_RUNTIME_CHECKS
    if (size < 0) {
        push_error_diagnostic("Negative size not allowed");
    }

    if (tag <= 0 || tag >= byte_file_.string_table_size) {
        push_error_diagnostic("String index out of bounds");
    }
#endif

    auto probe = pop();
    if (!probe.is_s_expr()) {
        push(Value());
        return;
    }

    auto ptr = &byte_file_.string_table.get()[tag];
    auto tag_hash = LtagHash(ptr);
    auto result = Btag(probe.as_ptr(), tag_hash, BOX(size));

    push(Value(static_cast<auint>(result)));
}

inline void lamar::Interpreter::interpret_array() {
    auto n = read_uint();

    auto value = pop();
    auto result = Barray_patt(value.as_ptr(), BOX(n));
    push(Value(static_cast<auint>(result)));
}

inline void lamar::Interpreter::interpret_fail() {
    auto line = read_uint();
    auto column = read_uint();
    auto value = pop();

    Bmatch_failure(value.as_ptr(), byte_file_.file_name.data(), line, column);
}

inline void lamar::Interpreter::interpret_line() {
    // todo
    read_uint();
}

inline void lamar::Interpreter::interpret_patteqstr() {
    auto b = pop();
    auto a = pop();
    auto result = Bstring_patt(a.as_ptr(), b.as_ptr());

    push(Value(static_cast<auint>(result)));
}

inline void lamar::Interpreter::interpret_pattstring() {
    auto value = pop();
    auto result = Bstring_tag_patt(value.as_ptr());

    push(Value(static_cast<auint>(result)));
}

inline void lamar::Interpreter::interpret_pattarray() {
    auto value = pop();
    auto result = Barray_tag_patt(value.as_ptr());

    push(Value(static_cast<auint>(result)));
}

inline void lamar::Interpreter::interpret_pattsexp() {
    auto value = pop();
    auto result = Bsexp_tag_patt(value.as_ptr());

    push(Value(static_cast<auint>(result)));
}

inline void lamar::Interpreter::interpret_pattref() {
    auto value = pop();
    auto result = Bboxed_patt(value.as_ptr());

    push(Value(static_cast<auint>(result)));
}

inline void lamar::Interpreter::interpret_pattval() {
    auto value = pop();
    auto result = Bunboxed_patt(value.as_ptr());

    push(Value(static_cast<auint>(result)));
}

inline void lamar::Interpreter::interpret_pattfun() {
    auto value = pop();
    auto result = Bclosure_tag_patt(value.as_ptr());

    push(Value(static_cast<auint>(result)));
}

inline void lamar::Interpreter::interpret_call_lread() {
    auto value = Lread();
    push(Value(static_cast<auint>(value)));
}

inline void lamar::Interpreter::interpret_call_lwrite() {
    auto value = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!value.is_int()) {
        push_error_diagnostic("Only integer write supported");
    }
#endif

    auto result = Lwrite(static_cast<aint>(value.as_repr()));

    push(Value(static_cast<auint>(result)));
}

inline void lamar::Interpreter::interpret_call_llength() {
    auto x = pop();

#ifndef DISABLE_RUNTIME_CHECKS
    if (!x.is_arr() && !x.is_str() && !x.is_s_expr()) {
        push_error_diagnostic("Try to get length not in array, string, or sexp");
    }
#endif

    auto len = Llength(x.as_ptr());
    push(Value(static_cast<auint>(len)));
}

inline void lamar::Interpreter::interpret_call_lstring() {
    auto value = pop();
    // todo validate
    auto str = Lstring(static_cast<aint *>(value.as_ptr()));
    push(Value(str));
}

inline void lamar::Interpreter::interpret_call_barray() {
    auto len = read_uint();
#ifndef DISABLE_RUNTIME_CHECKS
    if (len <= 0) {
        push_error_diagnostic("Try allocate zero or negative length array");
    }
#endif

    std::vector<aint> args;
    args.reserve(len);
    for (int i = static_cast<int>(len - 1); i >= 0; --i) {
        auto arg = pop();
        args[i] = static_cast<aint>(arg.as_repr());
    }

    void *v = Barray(args.data(), BOX(len));
    push(Value(v));
}

inline void lamar::Interpreter::interpret_eof() {
    // do nothing
}

lamar::Value lamar::Interpreter::get_local(int32_t address) {
    // todo validate
    decltype(auto) frame = call_stack_.back();
    auto val = static_cast<auint>(stack_.at(frame.get_sp() + address));
    return Value(val);
}

lamar::Value lamar::Interpreter::get_arg(int32_t address) {
    // todo validate
    decltype(auto) frame = call_stack_.back();
    auto val = static_cast<auint>(stack_.at(frame.get_sp() - frame.get_args_count() + address));
    return Value(val);
}

lamar::Value lamar::Interpreter::get_capture(int32_t address) {
    // todo validate
    auto closure = get_arg(-1);
    auto closure_data = TO_DATA(closure.as_ptr());
    auto captures = reinterpret_cast<auint *>(closure_data->contents);
    return Value(captures[address + 1]);
}

lamar::Value lamar::Interpreter::get_local_address(int32_t address) {
    // todo validate
    decltype(auto) frame = call_stack_.back();
    auto val = &stack_.at(frame.get_sp() + address);
    return Value(val);
}

lamar::Value lamar::Interpreter::get_arg_address(int32_t address) {
    // todo validate
    decltype(auto) frame = call_stack_.back();
    auto val = &stack_.at(frame.get_sp() - frame.get_args_count() + address);
    return Value(val);
}

lamar::Value lamar::Interpreter::get_capture_address(int32_t address) {
    // todo validate
    auto closure = get_arg(-1);
    auto closure_data = TO_DATA(closure.as_ptr());
    auto captures = reinterpret_cast<auint *>(closure_data->contents);
    return Value(&captures[address + 1]);
}

void lamar::Interpreter::set_local(int32_t address, Value &value) {
    // todo validate
    decltype(auto) frame = call_stack_.back();
    stack_.at(frame.get_sp() + address) = value.as_repr();
}

void lamar::Interpreter::set_arg(int32_t address, Value &value) {
    // todo validate
    decltype(auto) frame = call_stack_.back();
    stack_.at(frame.get_sp() - frame.get_args_count() + address) = value.as_repr();
}

void lamar::Interpreter::set_capture(int32_t address, Value &value) {
    // todo validate
    auto closure = get_arg(-1);
    auto closure_data = TO_DATA(closure.as_ptr());
    auto captures = reinterpret_cast<auint *>(closure_data->contents);
    captures[address + 1] = value.as_repr();
}

