//
// Created by Nikita Morozov on 07.01.2026.
//

#include "bytecode.hpp"


lamar::PublicSymbol::PublicSymbol(uint32_t nameOffset, uint32_t offset) : name_offset(nameOffset), offset(offset) {}

std::string_view lamar::read_string(const ByteFile &file, int32_t offset) {
    if (offset < 0 || static_cast<uint32_t>(offset) >= file.string_table_size) {
        throw std::runtime_error("Invalid string offset in bytecode");
    }

    const char *start = file.string_table.get() + offset;
    const auto max_len = file.string_table_size - static_cast<uint32_t>(offset);
    size_t len = 0;
    while (len < max_len && start[len] != '\0') {
        ++len;
    }
    return {start, len};
}
