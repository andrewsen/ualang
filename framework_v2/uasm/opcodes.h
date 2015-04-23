#ifndef OPCODES_H
#define OPCODES_H

//#include "common.h"

typedef unsigned char byte;

struct ModuleFlags
{
    unsigned executable_bit : 1;
    unsigned no_globals_bit : 1;
    unsigned no_internal_bit : 1;
    unsigned reserved : 5;
};

enum class Type : byte {
    VOID,
    UI8,
    I16,
    UI32,
    I32,
    UI64,
    I64,
    BOOL,
    DOUBLE,
    UTF8
};

extern unsigned int sizes [];

enum class OpCode : byte {
    NOP,
    TOP,
    BAND,
    BOR,
    ADD,
    ADDF,
    SUB,
    SUBF,
    MUL,
    MULF,
    DIV,
    DIVF,
    REM,
    REMF,
    CONV_UI8,
    CONV_I16,
    CONV_I32,
    CONV_UI32,
    CONV_I64,
    CONV_UI64,
    CONV_F,
    JMP,
    JZ,
    JT,
    JNZ,
    JF,
    JNULL,
    JNNULL,
    CALL,
    NEWLOC,
    FREELOC,
    LDLOC,
    LDLOC_0,
    LDLOC_1,
    LDLOC_2,
    STLOC,
    STLOC_0,
    STLOC_1,
    STLOC_2,
    LDARG,
    LDARG_0,
    LDARG_1,
    LDARG_2,
    STARG,
    STARG_0,
    STARG_1,
    STARG_2,
    LDFLD,
    LDFLD_0,
    LDFLD_1,
    LDFLD_2,
    STFLD,
    STFLD_0,
    STFLD_1,
    STFLD_2,
    LD_0,
    LD_1,
    LD_2,
    LD_STR,
    LD_UI8,
    LD_I16,
    LD_I32,
    LD_UI32,
    LD_I64,
    LD_UI64,
    LD_F,
    LD_TRUE,
    LD_FALSE,
    LD_NULL,
    AND,
    OR,
    EQ,
    NEQ,
    NOT,
    INV,
    XOR,
    NEG,
    POS,
    INC,
    DEC,
    SHL,
    SHR,
    POP,
    GT,
    GTE,
    LT,
    LTE,
    SIZEOF,
    TYPEOF,
    RET, //88

    CALL_INTERNAL,
    UNDEFINED
};

#endif // OPCODES_H
