//
// Created by Nikita Morozov on 24.02.2026.
//

#ifndef LAMAR_UTILS_HPP
#define LAMAR_UTILS_HPP

#include <algorithm>
#include <cstdint>
#include <functional>

namespace lamar::util {

    template<typename E, typename Compare = std::less<E>>
    class PriorityQueue {
    public:
        PriorityQueue(void *data, uint32_t capacity, Compare comp = Compare())
                : data_(reinterpret_cast<E *>(data)), size_(0), capacity_(capacity), comp_(comp) {
            std::make_heap(data_, data_ + size_, comp_);
        }

        void push(const E &element) {
            if (size_ >= capacity_) {
                return;
            }
            data_[size_++] = element;
            std::push_heap(data_, data_ + size_, comp_);
        }

        E pop() {
            if (empty()) {
                return E{};
            }
            std::pop_heap(data_, data_ + size_, comp_);
            return data_[--size_];
        }

        E peek() {
            return data_[0];
        }

        [[nodiscard]] uint32_t size() const {
            return size_;
        }

        [[nodiscard]] bool empty() const {
            return size_ == 0;
        }

    private:
        E *data_;
        uint32_t size_;
        uint32_t capacity_;
        Compare comp_;
    };

    template<typename E>
    class Stack {
    public:
        explicit Stack(void *data) : data_(reinterpret_cast<E *>(data)) {}

        void push(const E &element) {
            *end_++ = element;
        }

        E pop() {
            return *--end_;
        }

        E peek(uint32_t offset = 0) const {
            return *(end_ - 1 - offset);
        }

        [[nodiscard]] uint32_t size() const {
            return end_ - data_;
        }

        [[nodiscard]] bool empty() const {
            return bool(size());
        }

    private:
        E *data_;
        E *end_ = data_;
    };
}

#endif //LAMAR_UTILS_HPP
