//
// Created by Nikita Morozov on 04.02.2026.
//

#ifndef LAMAR_VALUE_HPP
#define LAMAR_VALUE_HPP

#include "runtime.hpp"

namespace lamar {

    class Value {
    public:
        Value();

        explicit Value(auint x);

        explicit Value(aint v);

        explicit Value(void *p);

        Value(const Value &x) = default;

        Value &operator=(const Value &other) = default;

        [[nodiscard]] auint as_repr() const;

        [[nodiscard]] aint as_int() const;

        [[nodiscard]] void *as_ptr() const;

        [[nodiscard]] lama_type type() const;

        [[nodiscard]] size_t size() const;

        [[nodiscard]] bool is_int() const;

        [[nodiscard]] bool is_boxed() const;

        [[nodiscard]] bool is_str() const;

        [[nodiscard]] bool is_arr() const;

        [[nodiscard]] bool is_s_expr() const;

        [[nodiscard]] bool is_closure() const;

        [[nodiscard]] bool is_aggregate() const;

    private:
        auint repr_;
    };
}

#endif //LAMAR_VALUE_HPP
