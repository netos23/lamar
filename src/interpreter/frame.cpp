//
// Created by Nikita Morozov on 07.02.2026.
//

#include "frame.hpp"


void lamar::Frame::set_line(size_t line) {
    line_ = line;
}

lamar::Frame::Frame(
        uint32_t return_address,
        uint32_t sp,
        uint32_t proc_address,
        uint32_t args_count,
        bool closure
) : return_address_(return_address),
    sp_(sp),
    proc_address_(proc_address),
    args_count_(args_count),
    closure_(closure) {}


#ifndef DISABLE_RUNTIME_CHECKS

uint32_t lamar::Frame::get_locals_count() const {
    return locals_count_;
}

void lamar::Frame::set_locals_count(uint32_t locals_count) {
    locals_count_ = locals_count;
}

#endif

uint32_t lamar::Frame::get_args_count() const {
    return args_count_;
}

uint32_t lamar::Frame::get_sp() const {
    return sp_;
}

uint32_t lamar::Frame::get_return_address() const {
    return return_address_;
}

bool lamar::Frame::is_closure() const {
    return closure_;
}
