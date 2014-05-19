#ifndef OPCODES_H
#define OPCODES_H

enum class OpCodes: unsigned char {
    ADD,
    ADDF,
    AND,

    BAND,
    BOR,

    CONV_UI8,
    CONV_I16,
    CONV_I32,
    CONV_UI32,
    CONV_I64,
    CONV_UI64,
    CONV_F,
    CALL,

    DEC,
    DIV,
    DIVF,

    EQ,

    FREELOC,

    INC,

    JMP,
    JZ,
    JT,
    JNZ,
    JF,
    JNULL,
    JNNULL,


    LDLOC,
    LDLOC_0,
    LDLOC_1,
    LDLOC_2,
    LDLOC_3,
    LDARG,
    LDARG_0,
    LDARG_1,
    LDARG_2,
    LDARG_3,
    LDFLD,
    LDFLD_0,
    LDFLD_1,
    LDFLD_2,
    LDFLD_3,
    LD_0,
    LD_1,
    LD_STR,
    LD_UI8,
    LD_I16,
    LD_I32,
    LD_UI32,
    LD_I64,
    LD_UI64,
    LD_F,
    LD_BOOL,
    LD_NULL,
    LDELEM,

    MUL,
    MULF,

    NEWARR,
    NEWLOC,
    NEQ,
    NEG,
    NOT,
    NOP,

    OR,

    POP,

    REM,
    REMF,
    RET,
    POS,

    STLOC,
    STLOC_0,
    STLOC_1,
    STLOC_2,
    STLOC_3,
    STARG,
    STARG_0,
    STARG_1,
    STARG_2,
    STARG_3,
    STFLD,
    STFLD_0,
    STFLD_1,
    STFLD_2,
    STFLD_3,
    SUB,
    SUBF,
    STELEM,
    SHL,
    SHR,
    SIZEOF,

    TOP,
    TYPEOF,

    XOR,
};

#endif // OPCODES_H
