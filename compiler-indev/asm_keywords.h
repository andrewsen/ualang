/*
    uac -- bilexical compiler. A part of bilexical programming language gramework
    Copyright (C) 2013-2014 Andrew Senko.

    This file is part of uac.

    uac is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    uac is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with uac.  If not, see <http://www.gnu.org/licenses/>.
*/

/*  Written by Andrew Senko <andrewsen@yandex.ru>. */

#ifndef ASM_KEYWORDS_H
#define ASM_KEYWORDS_H

#undef TRUE
#undef FALSE
enum class OpCodes : unsigned char {_NULL_ = 0x0, TRUE = 0x1, ADD = 0x2, AND, BAND, BTC, BOR, BTS, BXOR, CALL, CMP, DEC, DIV, INC,
	JMP, JE, JNE, JG, JL, JGE, JLE,
	MOV, MUL, NEG, NOP, NOT, OR, POP, PUSH, ROL, ROR, SET, SHL, SHR, SUB, XCH, XOR,
	DEF,
VR1 = 0x70,
VR2 = 0x71,
VR3 = 0x72,
VR4 = 0x73,
VR5 = 0x74,
VRI = 0x75,
VIP = 0x76,
VSP = 0x77,
VRA = 0x78,
FLAGS = 0x7F,
FUN = 42,
BT, // <- be-be-be
JI,
JS,
JNS,
JC,
JNC,
JO,
JNO,
JP,
JNP,
ALLOC,
FREE,
PROT,
RET,
END,
HALT,
STDOUT,
STDIN,
SCAN,
EXEC,
CALLE,
CALLNE,
CALL_NATIVE,
EXC, 
ADDR,
SIZEOF,
UNKNOWN,
CAST,
NAMESPACE,
EXTEND,
BRCK = 0xED,
SQBRCK = 0xEE,
DIGIT = 0xFF};

enum class PODTypes : unsigned char {
BOOL,
BYTE,
HWORD,
WORD,
W64,
DWORD,
STRING,
IDENTIFIER,
FUNC,
LABEL,
POINTER,
_NULL};

#endif
