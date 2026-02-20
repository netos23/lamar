//
// Created by Nikita Morozov on 07.02.2026.
//

#ifndef LAMAR_FRAME_HPP
#define LAMAR_FRAME_HPP

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "value.hpp"

namespace lamar {

    class Frame {
    public:
        static constexpr uint32_t ninit = uint32_t(-1);

        Frame() = default;

        Frame(
                uint32_t return_address,
                uint32_t sp,
                uint32_t proc_address,
                uint32_t args_count,
                bool closure = false
        );

#ifndef DISABLE_RUNTIME_CHECKS

        [[nodiscard]] uint32_t get_locals_count() const;

        void set_locals_count(uint32_t locals_count);

#endif


        void set_line(size_t line);

        [[nodiscard]] uint32_t get_args_count() const;

        [[nodiscard]] uint32_t get_sp() const;

        uint32_t get_return_addres();

        bool is_closure() const;

    private:
        uint32_t return_address_ = ninit;
        uint32_t sp_ = ninit;
        uint32_t proc_address_ = ninit;
        uint32_t args_count_ = ninit;
#ifndef DISABLE_RUNTIME_CHECKS
        uint32_t locals_count_ = ninit;
#endif
        uint32_t line_ = ninit;
        bool closure_ = false;
    };
}

#endif //LAMAR_FRAME_HPP
