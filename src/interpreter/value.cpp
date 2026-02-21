//
// Created by Nikita Morozov on 04.02.2026.
//

#include "value.hpp"

lamar::Value::Value() : repr_(0) {}

lamar::Value::Value(auint x) : repr_(x) {}

lamar::Value::Value(aint v) : repr_(BOX(v)) {}

lamar::Value::Value(void *p) : repr_(reinterpret_cast<auint>(p)) {}

auint lamar::Value::as_repr() const {
    return repr_;
}

aint lamar::Value::as_int() const {
    return UNBOX(repr_);
}

void *lamar::Value::as_ptr() const {
    return reinterpret_cast<void *>(repr_);
}

lama_type lamar::Value::type() const {
    return get_type_header_ptr(get_obj_header_ptr(as_ptr()));
}

bool lamar::Value::is_int() const {
    return UNBOXED(repr_);
}

bool lamar::Value::is_boxed() const {
    return !is_int();
}

bool lamar::Value::is_str() const {
    return is_boxed() && type() == ::STRING;
}

bool lamar::Value::is_arr() const {
    return is_boxed() && type() == ::ARRAY;
}

bool lamar::Value::is_s_expr() const {
    return is_boxed() && type() == ::SEXP;
}

bool lamar::Value::is_closure() const {
    return is_boxed() && type() == ::CLOSURE;
}

bool lamar::Value::is_aggregate() const {
    return is_arr() || is_s_expr() || is_str();
}

size_t lamar::Value::size() const {
    return is_int() ? 0 : Llength(as_ptr());
}