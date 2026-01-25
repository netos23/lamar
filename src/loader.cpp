//
// Created by Nikita Morozov on 06.01.2026.
//

#include "loader.hpp"


int32_t lamar::Loader::read_int32_t() {
    int32_t val = 0;

    auto expected_size = static_cast<std::streamsize>(sizeof(val));
    read_bytes(reinterpret_cast<char *>(&val), expected_size);

    return val;
}

void lamar::Loader::read_bytes(char *buf, std::streamsize size) {
    input_stream_.read(buf, size);

    if (input_stream_.gcount() < size) {
        throw std::range_error("Malformed bytefile");
    }
}

uint32_t lamar::Loader::read_uint32_t() {
    return static_cast<uint32_t>(read_int32_t());
}


lamar::ByteFile lamar::Loader::read_byte_file() {
    auto string_table_size = read_uint32_t();
    auto global_area_size = read_uint32_t();
    auto public_symbols_number = read_uint32_t();

    std::vector<PublicSymbol> symbol_table;
    symbol_table.reserve(public_symbols_number);

    for (uint32_t i = 0; i < public_symbols_number; i++) {
        symbol_table.emplace_back(read_uint32_t(), read_uint32_t());
    }

    std::unique_ptr<char> string_table(new char[string_table_size]);
    read_bytes(string_table.get(), string_table_size);

    auto program_code = read_bytecode();
    return {
            .string_table_size = string_table_size,
            .global_area_size = global_area_size,
            .public_symbols_number = public_symbols_number,
            .string_table = std::move(string_table),
            .public_symbol_table = std::move(symbol_table),
            .program_code =  std::move(program_code),
    };
}

std::vector<lamar::OpCode> lamar::Loader::read_bytecode() {
    auto pos = input_stream_.tellg();
    input_stream_.seekg(0, std::ios_base::end);
    auto end = input_stream_.tellg();
    auto len = end - pos + 1;
    input_stream_.seekg(pos);

    std::vector<lamar::OpCode> res;
    res.resize(len);

    input_stream_.read(reinterpret_cast<char*>(res.data()), len);

    return res;
}

lamar::Loader::Loader( std::istream &inputStream) : input_stream_(inputStream) {}
 