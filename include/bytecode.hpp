//
// Created by Nikita Morozov on 29.12.2025.
//

#ifndef LAMAR_BYTECODE_HPP
#define LAMAR_BYTECODE_HPP

#include <vector>
#include <cstdint>
#include <memory>
#include <string>

namespace lamar {
    // todo: more docs for opcodes
    enum OpCode : uint8_t {
        /// Binary operations
        /// +
        ADD = 0X01,
        /// -
        SUB = 0X02,
        /// *
        MUL = 0X03,
        /// /
        DIV = 0X04,
        /// %
        MOD = 0X05,
        /// <
        LT = 0X06,
        /// <=
        LE = 0X07,
        /// >
        GT = 0X08,
        /// >=
        GE = 0X09,
        /// ==
        EQ = 0X0A,
        /// !=
        NE = 0X0B,
        /// &&
        AND = 0X0C,
        /// ||
        OR = 0X0D,

        /// `const K`
        CONST = 0X10,
        /// `string S`
        STRING = 0X11,
        /// `sexp S N`
        SEXP = 0X12,
        /// `sti`
        STI = 0X13,
        /// `sta`
        STA = 0X14,
        /// `jmp L`
        JMP = 0X15,
        /// `end`
        END = 0X16,
        /// `ret`
        RET = 0X17,
        /// `DROP`
        DROP = 0X18,
        /// `DUP`
        DUP = 0X19,
        /// `SWAP`
        SWAP = 0X1A,
        /// `ELEM`
        ELEM = 0X1B,

        /// `LD G(M)`
        LD_G = 0X20,
        /// `LD L(M)`
        LD_L = 0X21,
        /// `LD A(M)`
        LD_A = 0X22,
        /// `LD C(M)`
        LD_C = 0X23,
        /// `LDA G(M)`
        LDA_G = 0X30,
        /// `LDA L(M)`
        LDA_L = 0X31,
        /// `LDA A(M)`
        LDA_A = 0X32,
        /// `LDA C(M)`
        LDA_C = 0X33,
        /// `ST G(M)`
        ST_G = 0X40,
        /// `ST L(M)`
        ST_L = 0X41,
        /// `ST A(M)`
        ST_A = 0X42,
        /// `ST C(M)`
        ST_C = 0X43,

        /// `CJMPZ L`
        CJMPZ = 0X50,
        /// `CJMPNZ`
        CJMPNZ = 0X51,
        /// `BEGIN A N`
        BEGIN = 0X52,
        /// `CBEGIN A N`
        CBEGIN = 0X53,
        /// `CLOSURE L N V(M)...`
        CLOSURE = 0X54,
        /// `CALLC N`.
        CALLC = 0X55,
        /// `CALL L N`.
        CALL = 0X56,
        /// `TAG S N`
        TAG = 0X57,
        /// `ARRAY N`
        ARRAY = 0X58,
        /// `FAIL LN COL`
        FAIL = 0X59,
        /// `LINE LN`
        LINE = 0X5A,

        /// `PATT =STR`
        PATTEQSTR = 0X60,
        /// `PATT #STRING`
        PATTSTRING = 0X61,
        /// `PATT #ARRAY`
        PATTARRAY = 0X62,
        /// `PATT #SEXP`
        PATTSEXP = 0X63,
        /// `PATT #REF`
        PATTREF = 0X64,
        /// `PATT #VAL`
        PATTVAL = 0X65,
        /// `PATT #FUN`
        PATTFUN = 0X66,

        /// `CALL LREAD`
        CALL_LREAD = 0X70,
        /// `CALL LWRITE`
        CALL_LWRITE = 0X71,
        /// `CALL LLENGTH`
        CALL_LLENGTH = 0X72,
        /// `CALL LSTRING`
        CALL_LSTRING = 0X73,
        /// `CALL BARRAY`
        CALL_BARRAY = 0X74,

        /// END-OF-FILE
        EOF_ = 0XFF,
    };


    enum ClosureArgType : uint8_t {
        Global = 0,
        Local = 1,
        Arg = 2,
        Capture = 3,
    };

    struct PublicSymbol {
        uint32_t name_offset;
        uint32_t offset;

        PublicSymbol(uint32_t nameOffset, uint32_t offset);
    };


    struct ByteFile {
        uint32_t string_table_size;
        uint32_t global_area_size;
        uint32_t public_symbols_number;
        std::unique_ptr<char> string_table;
        std::vector<PublicSymbol> public_symbol_table;
        std::vector<OpCode> program_code;
        std::string file_name;
    };

    bool is_valid_opcode(uint8_t opcode);
    bool is_valid_closure_arg_type(uint8_t closure_arg_type);
}

#endif //LAMAR_BYTECODE_HPP
