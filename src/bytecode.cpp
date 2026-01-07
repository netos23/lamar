//
// Created by Nikita Morozov on 07.01.2026.
//

#include "bytecode.hpp"


lamar::PublicSymbol::PublicSymbol(uint32_t nameOffset, uint32_t offset) : name_offset(nameOffset), offset(offset) {}
