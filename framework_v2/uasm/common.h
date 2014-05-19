#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <fstream>
#include "OpCodes.h"

typedef unsigned char u8;
typedef unsigned char byte;
typedef short i16;
typedef int i32;
typedef unsigned int ui32;
typedef long long i64;
typedef unsigned long long ui64;

using namespace std;

enum class Type : byte {
    UI8,
    I16,
    UI16,
    I32,
    UI32,
    I64,
    UI64,

    DOUBLE,
    UTF8,
    BOOL,

    OBJ,

    NUL
};
#endif // COMMON_H
