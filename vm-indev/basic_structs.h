/*
    uavm -- virtual machine. A part of bilexical programming language gramework
    Copyright (C) 2013-2014 Andrew Senko.

    This file is part of uavm.

    Uavm is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Uavm is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with uavm.  If not, see <http://www.gnu.org/licenses/>.
*/

/*  Written by Andrew Senko <andrewsen@yandex.ru>. */

#ifndef BASIC_STRUCTS_H
#define BASIC_STRUCTS_H

#include <vector>
#include <string>
#include "../compiler-indev/asm_keywords.h"

#define __interface struct

typedef long long int __int64;
typedef unsigned int uint;

typedef unsigned char byte;

union memory_unit {
    double dword;
    __int64 reg;
    int word;
    unsigned int uword;
    short int hword;
    bool b;
    unsigned char byte;
    unsigned char bytes [9];
};

struct Array
{
    std::vector<byte> items;
    unsigned char factor;
    PODTypes type;
};

class TraceData {
public:
    std::string modname;
    std::vector<byte> * commands;
    std::vector<Array> * arrays;
    std::vector<memory_unit> * stack;
    std::vector<std::string> * strings;
    memory_unit * regs;
};

#endif // BASIC_STRUCTS_H
