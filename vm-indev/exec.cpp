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

#include "runtime.h"
#include <cstring>
#include <limits>
#include <cstdlib>

FlagsReg flags;

template<typename T> inline
bool CanAdd(const T& a, const T& b)
{
    return b <= (std::numeric_limits<T>::max() - a);
}

template<typename T> inline
bool CanSub(const T& a, const T& b)
{
    return b <= (std::numeric_limits<T>::min() + a);
}

template<typename T> inline
bool CanMul(const T& a, const T& b)
{
    return (b <= (std::numeric_limits<T>::max() / a)) &&
           (b >= (std::numeric_limits<T>::min() / a));
}

template<typename T> inline
bool CanDiv(const T& a, const T& b)
{
    return (a <= (std::numeric_limits<T>::max() * b)) &&
           (a >= (std::numeric_limits<T>::min() * b));
}

void Module::exec_add() {
    PODTypes type1, type2, res;
    auto arg1 = getArg(type1);
    auto arg2 = getArg(type2);
    ++vip.uword;
    res = (unsigned char)type1 > (unsigned char)type2 ? type1 : type2;
    vra.reg = 0;
    switch (res)
    {
    case PODTypes::BOOL:
        {vra.b = arg1.b + arg2.b;
            vra.bytes[8] == (unsigned char)PODTypes::BOOL;}
        break;
    case PODTypes::BYTE:
        {
            if(!CanAdd(arg1.byte, arg2.byte)) flags.OF = 1;
            else flags.OF = 0;
            vra.byte = arg1.byte + arg2.byte;
            vra.bytes[8] = (unsigned char)PODTypes::BYTE;
            flags.CF = 0;
            flags.SF = 0;
        }
        break;
    case PODTypes::HWORD:
        {
            if(!CanAdd(arg1.hword, arg2.hword)) flags.OF = 1;
            else flags.OF = 0;
            vra.hword = arg1.hword + arg2.hword;
            vra.bytes[8] = (unsigned char)PODTypes::HWORD;
            if(vra.hword < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::WORD:
        {
            if(!CanAdd(arg1.word, arg2.word)) flags.OF = 1;
            else flags.OF = 0;
            vra.word = arg1.word + arg2.word;
            vra.bytes[8] = (unsigned char)PODTypes::WORD;
            if(vra.word < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::W64:
        {
            if(!CanAdd(arg1.reg, arg2.reg)) flags.OF = 1;
            else flags.OF = 0;
            vra.reg = arg1.reg + arg2.reg;
            vra.bytes[8] = (unsigned char)PODTypes::W64;
            if(vra.reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::DWORD:
        {
            if(type1 == PODTypes::DWORD && type2 == PODTypes::DWORD)
            {
                if(!CanAdd(arg1.dword, arg2.dword)) flags.OF = 1;
                else flags.OF = 0;
                vra.dword = arg1.dword + arg2.dword;
            }
            else if((unsigned char)type1 < (unsigned char)type2)
            {
                if(!CanAdd((double)arg1.reg, arg2.dword)) flags.OF = 1;
                else flags.OF = 0;
                vra.dword = arg1.reg + arg2.dword;
            }
            else
            {
                if(!CanAdd(arg1.dword, (double)arg2.reg)) flags.OF = 1;
                else flags.OF = 0;
                vra.dword = arg1.dword + arg2.reg;
            }
            vra.bytes[8] = (unsigned char)PODTypes::DWORD;
            if(vra.reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    default:
        break;
    }
}

void Module::exec_alloc() {
    PODTypes t = (PODTypes)commands[++vip.uword], t1;
    byte factor;
    switch (t) {
    case PODTypes::BOOL:
    case PODTypes::BYTE:
        factor = 1;
        break;
    case PODTypes::HWORD:
        factor = sizeof(short);
        break;
    case PODTypes::WORD:
    case PODTypes::STRING:
        factor = sizeof(int);
        break;
    case PODTypes::DWORD:
    case PODTypes::W64:
        factor = sizeof(__int64);
        break;
    default:
        rt->ThrowException("Невидомій тип!");
        break;
    }
    auto size = getArg(t1);
    //++vip.uword;
    if(t1 == PODTypes::DWORD || t1 == PODTypes::STRING || t1 == PODTypes::BOOL)
        rt->ThrowException("Розмір масиву визначається цілим числом!", RuntimeException::Warning);
    // FIXME!
    Array a;
    a.factor = factor;
    a.items.assign(size.uword*factor, 0);
    a.type = t;
    vra.uword = array_map.size();
    vra.bytes[8] = (byte)PODTypes::POINTER;
    //if(array_map.find(vra.uword) != array_map.end()) {
    //    cout << "ERROR IN ARRAY MAP: this key is already inserter! " << vra.uword << endl;
    //    cout << array_map[vra.uword].items.size() << endl;
    //}
    //cout << "ADDING ARRAY TO MAP WITH KEY " << array_map.size() << endl;
    array_map[vra.uword] = a;
}

void Module::exec_and()
{
    PODTypes type1, type2, res;
    auto arg1 = getArg(type1);
    auto arg2 = getArg(type2);
    ++vip.uword;
    res = (unsigned char)type1 > (unsigned char)type2 ? type1 : type2;
    vra.reg = 0;
    switch (res)
    {
    case PODTypes::BOOL:
        {vra.b = arg1.b & arg2.b;
            vra.bytes[8] = (unsigned char)PODTypes::BOOL;}
        break;
    case PODTypes::BYTE:
        {vra.byte = arg1.byte & arg2.byte;
            vra.bytes[8] = (unsigned char)PODTypes::BYTE;}
        break;
    case PODTypes::HWORD:
        {vra.hword = arg1.hword & arg2.hword;
            vra.bytes[8] = (unsigned char)PODTypes::HWORD;}
        break;
    case PODTypes::WORD:
        {vra.word = arg1.word & arg2.word;
            vra.bytes[8] = (unsigned char)PODTypes::WORD;}
        break;
    case PODTypes::W64:
        {vra.reg = arg1.reg & arg2.reg;
            vra.bytes[8] = (unsigned char)PODTypes::W64;}
        break;
    case PODTypes::DWORD:
        {
            //ERROR
        }
        break;
    default:
        break;
    }
}

void Module::exec_call()
{
    /************************************************************\
     * Должен находиить адрес функции по ее смещению в таблице! *
     *          Пока вызов идет по прямому адресу               *
    \************************************************************/
    memory_unit addr;
    addr.reg = 0;
    uint m;
    m = commands[++vip.uword];
    addr.bytes[0] = commands[++vip.uword];
    addr.bytes[1] = commands[++vip.uword];
    addr.bytes[2] = commands[++vip.uword];
    addr.bytes[3] = commands[++vip.uword];
    if(m == 0) {
        call_stack.push(vip.uword);
        vip.uword = addr.uword;
    }
    else {
        --m;
        //cout << "calling module " << imports[(int)m].name << endl;
        uint ip = vip.uword;
        //rt->GetModule(this, m)->Call(addr.uword);
        imports[m].Execute(addr.uword);
        vip.uword = ip;
    }
}

void Module::exec_calle()
{
    if (flags.ZF == 1) {
        memory_unit addr;
        addr.reg = 0;
        byte m;
        m = commands[++vip.uword];
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        if(m == 0) {
            call_stack.push(vip.uword);
            vip.uword = addr.uword;
        }
        else {
            uint ip = vip.uword;
            imports[m-1].Execute(addr.uword);
            //rt->GetModule(this, m)->Call(addr.uword);
            vip.uword = ip;
        }
    }
    else vip.uword += 6;
}

void Module::exec_callne()
{
    if (flags.ZF == 0) {
        memory_unit addr;
        addr.reg = 0;
        byte m;
        m = commands[++vip.uword];
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        if(m == 0) {
            call_stack.push(vip.uword);
            vip.uword = addr.uword;
        }
        else {
            uint ip = vip.uword;
            imports[m].Execute(addr.uword);
            //rt->GetModule(this, m)->Call(addr.uword);
            vip.uword = ip;
        }
    }
    else vip.uword += 6;
}

void Module::exec_cmp()
{
    PODTypes type1, type2, res;
    auto arg1 = getArg(type1);
    auto arg2 = getArg(type2);
    ++vip.uword;
    memory_unit u;
    if(type1 == PODTypes::DWORD && (unsigned char)type1 > (unsigned char)type2) {
        if(arg1.dword == arg2.reg) {
            flags.CF = 0;
            flags.ZF = 1;
            flags.OF = 0;
            flags.SF = 0;
        }
        else if(arg1.dword < arg2.reg) {
            flags.CF = 1;
            flags.ZF = 0;
            flags.OF = 1;
            flags.SF = 1;
        }
        else if(arg1.dword > arg2.reg) {
            flags.CF = 0;
            flags.ZF = 0;
            flags.OF = 0;
            flags.SF = 0;
        }
        return;
    }
    else if(type2 == PODTypes::DWORD && (unsigned char)type1 < (unsigned char)type2) {
        if(arg1.reg == arg2.dword) {
            flags.CF = 0;
            flags.ZF = 1;
            flags.OF = 0;
            flags.SF = 0;
        }
        else if(arg1.reg < arg2.dword) {
            flags.CF = 1;
            flags.ZF = 0;
            flags.OF = 1;
            flags.SF = 1;
        }
        else if(arg1.reg > arg2.dword) {
            flags.CF = 0;
            flags.ZF = 0;
            flags.OF = 0;
            flags.SF = 0;
        }
        return;
    }
    else if(type1 == PODTypes::DWORD && type2 == PODTypes::DWORD)
    {
        if(arg1.dword == arg2.dword) {
            flags.CF = 0;
            flags.ZF = 1;
            flags.OF = 0;
            flags.SF = 0;
        }
        else if(arg1.dword < arg2.dword) {
            flags.CF = 1;
            flags.ZF = 0;
            flags.OF = 1;
            flags.SF = 1;
        }
        else if(arg1.dword > arg2.dword) {
            flags.CF = 0;
            flags.ZF = 0;
            flags.OF = 0;
            flags.SF = 0;
        }
        return;
    }
    else if(type1 == PODTypes::STRING && type2 == PODTypes::STRING)
    {
        string s1 = str_stack[arg1.uword];
        string s2 = str_stack[arg2.uword];
        if(s1 == s2) {
            flags.CF = 0;
            flags.ZF = 1;
            flags.OF = 0;
            flags.SF = 0;
        }
        else if(s1.length() < s2.length()) {
            flags.CF = 1;
            flags.ZF = 0;
            flags.OF = 1;
            flags.SF = 1;
        }
        else if(s1.length() > s2.length()) {
            flags.CF = 0;
            flags.ZF = 0;
            flags.OF = 0;
            flags.SF = 0;
        }
        return;
    }
    else if(type1 == PODTypes::BOOL && type2 == PODTypes::BOOL)
    {
        if(arg1.b == arg2.b) {
            flags.CF = 0;
            flags.ZF = 1;
            flags.OF = 0;
            flags.SF = 0;
        }
        else if(!arg1.b && arg2.b) {
            flags.CF = 1;
            flags.ZF = 0;
            flags.OF = 1;
            flags.SF = 1;
        }
        else if(arg1.b && !arg2.b) {
            flags.CF = 0;
            flags.ZF = 0;
            flags.OF = 0;
            flags.SF = 0;
        }
        return;
    }
    else
    {
        if(arg1.reg == arg2.reg) {
            flags.CF = 0;
            flags.ZF = 1;
            flags.OF = 0;
            flags.SF = 0;
        }
        else if(arg1.reg < arg2.reg) {
            flags.CF = 1;
            flags.ZF = 0;
            flags.OF = 1;
            flags.SF = 1;
        }
        else if(arg1.reg > arg2.reg) {
            flags.CF = 0;
            flags.ZF = 0;
            flags.OF = 0;
            flags.SF = 0;
        }
        return;
    }
    res = (unsigned char)type1 > (unsigned char)type2 ? type1 : type2;
    switch (res)
    {
    case PODTypes::BOOL:
        {
            if(arg1.b == arg2.b) {
                flags.CF = 0;
                flags.ZF = 1;
                flags.OF = 0;
                flags.SF = 0;
            }
            else if(!arg1.b && arg2.b) {
                flags.CF = 1;
                flags.ZF = 0;
                flags.OF = 1;
                flags.SF = 1;
            }
            else if(arg1.b && !arg2.b) {
                flags.CF = 0;
                flags.ZF = 0;
                flags.OF = 0;
                flags.SF = 0;
            }
            // ___
            //|1|1|
            //|1|0|
            //|0|1|
            //|0|0|
        }
        break;
    case PODTypes::BYTE:
        {
            if(arg1.byte == arg2.byte) {
                flags.CF = 0;
                flags.ZF = 1;
                flags.OF = 0;
                flags.SF = 0;
            }
            else if(arg1.byte < arg2.byte) {
                flags.CF = 1;
                flags.ZF = 0;
                flags.OF = 1;
                flags.SF = 1;
            }
            else if(arg1.byte > arg2.byte) {
                flags.CF = 0;
                flags.ZF = 0;
                flags.OF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::HWORD:
        {
            if(arg1.hword == arg2.hword) {
                flags.CF = 0;
                flags.ZF = 1;
                flags.OF = 0;
                flags.SF = 0;
            }
            else if(arg1.hword < arg2.hword) {
                flags.CF = 1;
                flags.ZF = 0;
                flags.OF = 1;
                flags.SF = 1;
            }
            else if(arg1.hword > arg2.hword) {
                flags.CF = 0;
                flags.ZF = 0;
                flags.OF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::WORD:
        {
            if(arg1.word == arg2.word) {
                flags.CF = 0;
                flags.ZF = 1;
                flags.OF = 0;
                flags.SF = 0;
            }
            else if(arg1.word < arg2.word) {
                flags.CF = 1;
                flags.ZF = 0;
                flags.OF = 1;
                flags.SF = 1;
            }
            else if(arg1.word > arg2.word) {
                flags.CF = 0;
                flags.ZF = 0;
                flags.OF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::W64:
        {
            if(arg1.reg == arg2.reg) {
                flags.CF = 0;
                flags.ZF = 1;
                flags.OF = 0;
                flags.SF = 0;
            }
            else if(arg1.reg < arg2.reg) {
                flags.CF = 1;
                flags.ZF = 0;
                flags.OF = 1;
                flags.SF = 1;
            }
            else if(arg1.reg > arg2.reg) {
                flags.CF = 0;
                flags.ZF = 0;
                flags.OF = 0;
                flags.SF = 0;
            }
        }
    case PODTypes::STRING:
        {
            string s1 = str_stack[arg1.uword];
            string s2 = str_stack[arg2.uword];
            if(s1 == s2) {
                flags.CF = 0;
                flags.ZF = 1;
                flags.OF = 0;
                flags.SF = 0;
            }
            else if(s1.length() < s2.length()) {
                flags.CF = 1;
                flags.ZF = 0;
                flags.OF = 1;
                flags.SF = 1;
            }
            else if(s1.length() > s2.length()) {
                flags.CF = 0;
                flags.ZF = 0;
                flags.OF = 0;
                flags.SF = 0;
            }
        }
        break;
    default:
        break;
    }
}

void Module::exec_dec()
{
    PODTypes type1;
    if(commands[vip.uword+1] == (byte)PODTypes::POINTER)  {
        array_ptr ptr;
        //++vip.uword;
        ptr = getArrayPointer(type1);
        uint t = (byte)type1;
        if(type1 == PODTypes::DWORD)
            --(*ptr).dword;
        else if(t > (byte)PODTypes::BOOL && t < (byte)PODTypes::DWORD)
            --(*ptr).reg;
        else rt->ThrowException("Нечисловий тип!");
        applyArrayPointer(ptr);
        //*ptr = getArg(type2);
        //applyArrayPointer(ptr);
    }
    else {
        memory_unit* ptr;// = getPointer(type1);
        ptr = getPointer(type1);
        uint t = (byte)type1;
        if(type1 == PODTypes::DWORD)
            --(*ptr).dword;
        else if(t > (byte)PODTypes::BOOL && t < (byte)PODTypes::DWORD)
            --(*ptr).reg;
        else rt->ThrowException("Нечисловий тип!");
        //*arg1 = getArg(type2);
        ++vip.uword;
    }
    return;
}

void Module::exec_div() {
    PODTypes type1, type2, res;
    auto arg1 = getArg(type1);
    auto arg2 = getArg(type2);
    ++vip.uword;
    res = (unsigned char)type1 > (unsigned char)type2 ? type1 : type2;
    vra.reg = 0;
    vra.bytes[8] = (unsigned char)res;
    switch (res)
    {
    case PODTypes::BOOL:
        {vra.b = (bool)((unsigned char)arg1.b / (unsigned char)arg2.b);}
        break;
    case PODTypes::BYTE:
        {
            if(!CanDiv(arg1.byte, arg2.byte)) flags.OF = 1;
            else flags.OF = 0;
            vra.byte = arg1.byte / arg2.byte;
            flags.CF = 0;
            flags.SF = 0;
        }
        break;
    case PODTypes::HWORD:
        {
            if(!CanDiv(arg1.hword, arg2.hword)) flags.OF = 1;
            else flags.OF = 0;
            vra.hword = arg1.hword / arg2.hword;
            if(vra.hword < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::WORD:
        {
            if(!CanDiv(arg1.word, arg2.word)) flags.OF = 1;
            else flags.OF = 0;
            vra.word = arg1.word / arg2.word;
            if(vra.word < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::W64:
        {
            if(!CanDiv(arg1.reg, arg2.reg)) flags.OF = 1;
            else flags.OF = 0;
            vra.reg = arg1.reg / arg2.reg;
            if(vra.reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::DWORD:
        {
            if(type1 == PODTypes::DWORD && type2 == PODTypes::DWORD)
            {
                if(!CanDiv(arg1.dword, arg2.dword)) flags.OF = 1;
                else flags.OF = 0;
                vra.dword = arg1.dword / arg2.dword;
            }
            else if((unsigned char)type1 < (unsigned char)type2)
            {
                if(!CanDiv((double)arg1.reg, arg2.dword)) flags.OF = 1;
                else flags.OF = 0;
                vra.dword = arg1.reg / arg2.dword;
            }
            else
            {
                if(!CanDiv(arg1.dword, (double)arg2.reg)) flags.OF = 1;
                else flags.OF = 0;
                vra.dword = arg1.dword / arg2.reg;
            }
            if(vra.reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    default:
        break;
    }
}

void Module::exec_free() {
    PODTypes t1;
    memory_unit* a = getPointer(t1);
    if(t1 != PODTypes::POINTER)
        rt->ThrowException("Спроба звільнити змінну відмінну від масива!", RuntimeException::CriticalFault);
    array_map.erase(a->uword);
    a->bytes[8] = (byte)PODTypes::_NULL;
    ++vip.uword;
}

void Module::exec_inc()
{
    PODTypes type1;
    if(commands[vip.uword+1] == (byte)PODTypes::POINTER)  {
        array_ptr ptr;
        //++vip.uword;
        ptr = getArrayPointer(type1);
        uint t = (byte)type1;
        if(type1 == PODTypes::DWORD)
            ++(*ptr).dword;
        else if(t > (byte)PODTypes::BOOL && t < (byte)PODTypes::DWORD)
            ++(*ptr).reg;
        else rt->ThrowException("Нечисловий тип!");
        applyArrayPointer(ptr);
        //*ptr = getArg(type2);
        //applyArrayPointer(ptr);
    }
    else {
        memory_unit* ptr;// = getPointer(type1);
        ptr = getPointer(type1);
        uint t = (byte)type1;
        if(type1 == PODTypes::DWORD)
            ++(*ptr).dword;
        else if(t > (byte)PODTypes::BOOL && t < (byte)PODTypes::DWORD)
            ++(*ptr).reg;
        else rt->ThrowException("Нечисловий тип!");
        //*arg1 = getArg(type2);
        ++vip.uword;
    }
    return;
    /*
    PODTypes type;
    ++vip.uword;
    unsigned char modifier = commands[vip.uword];
    //if(modifier == (unsigned char)OpCodes::CAST) {
    //	cast = (PODTypes)commands[++vip.uword];
    //	++vip.uword;
    //	modifier = commands[vip.uword];
    //}
    //type = (PODTypes)modifier;
    if(is_register(modifier)) {
        unsigned char offset = (unsigned char)OpCodes::VR1;

        flags.CF = 0;
        flags.SF = 0;

        memory_unit *reg = &regs[commands[vip.uword] - offset];

        if(reg->bytes[8] == (unsigned char)PODTypes::BOOL)
            reg->b = true;
        else if(reg->bytes[8] == (unsigned char)PODTypes::DWORD) {
            if(!CanAdd(reg->dword, (double)1)) flags.OF = 1;
            else flags.OF = 0;

            ++reg->dword;
            if(reg->dword < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
        }
        else {
            if(!CanAdd(reg->reg, (__int64)1)) flags.OF = 1;
            else flags.OF = 0;

            ++reg->reg;
            if(reg->reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
        }
    }
    /*
    else if(modifier == (unsigned char)PODTypes::IDENTIFIER) {
        //type = (PODTypes) commands[++vip.uword];
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        type = (PODTypes)globals[addr.word].bytes[8];
        if(type != PODTypes::BOOL && type != PODTypes::DWORD) {
            auto t = globals[addr.word].reg;
            if(!CanAdd(t, (__int64)1)) flags.OF = 1;
            else flags.OF = 0;
            ++globals[addr.word].reg;
            if(globals[addr.word].reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        else if(type == PODTypes::DWORD) {
            auto t = globals[addr.word].dword;
            if(!CanAdd(t, (double)1)) flags.OF = 1;
            else flags.OF = 0;
            ++globals[addr.word].dword;
            if(globals[addr.word].dword < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        else {
            globals[addr.word].b = true;
            flags.OF = 0;
            flags.CF = 0;
            flags.SF = 0;
        }
    }
    * /
    else if(modifier == (unsigned char)OpCodes::SQBRCK) {
        modifier = commands[++vip.uword];
        memory_unit tmp;
        tmp.reg = 0;
        if(modifier == (unsigned char)PODTypes::BYTE) {
            tmp.bytes[0] = commands[++vip.uword];
        }
        if(modifier == (unsigned char)PODTypes::HWORD) {
            tmp.bytes[0] = commands[++vip.uword];
            tmp.bytes[1] = commands[++vip.uword];
        }
        if(modifier == (unsigned char)PODTypes::WORD) {
            tmp.bytes[0] = commands[++vip.uword];
            tmp.bytes[1] = commands[++vip.uword];
            tmp.bytes[2] = commands[++vip.uword];
            tmp.bytes[3] = commands[++vip.uword];
        }
        /*
        else if(modifier == (unsigned char)PODTypes::IDENTIFIER) {
            memory_unit addr;
            addr.reg = 0;
            addr.bytes[0] = commands[++vip.uword];
            addr.bytes[1] = commands[++vip.uword];
            addr.bytes[2] = commands[++vip.uword];
            addr.bytes[3] = commands[++vip.uword];
            tmp = globals[addr.word];
            if(tmp.bytes[8] == (unsigned char)PODTypes::DWORD ||
                tmp.bytes[8] == (unsigned char)PODTypes::STRING) {
                cout << "Addres can't be double or string!" << endl;
                cout << dec << "\tat " << vip.uword << endl;
                return;
            }
        }
        * /
        else if(is_register(modifier)) {
            unsigned char offset = (unsigned char)OpCodes::VR1;
            tmp.word = regs[commands[vip.uword] - offset].word;
        }
        type = (PODTypes)prog_stack[tmp.word].bytes[8];
        if(type != PODTypes::BOOL && type != PODTypes::DWORD) {
            memory_unit *t = &prog_stack[tmp.word];
            if(!CanAdd(t->reg, (__int64)1)) flags.OF = 1;
            else flags.OF = 0;
            ++t->reg;
            if(t->reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        else if(type == PODTypes::DWORD) {
            memory_unit *t = &prog_stack[tmp.word];
            if(!CanAdd(t->dword, (double)1)) flags.OF = 1;
            else flags.OF = 0;
            ++t->dword;
            if(t->dword < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        else {
            prog_stack[tmp.word].b = !prog_stack[tmp.word].b;
            flags.OF = 0;
            flags.CF = 0;
            flags.SF = 0;
        }
    }
    ++vip.uword;*/
}

void Module::exec_jmp()
{
    memory_unit addr;
    addr.reg = 0;
    addr.bytes[0] = commands[++vip.uword];
    addr.bytes[1] = commands[++vip.uword];
    addr.bytes[2] = commands[++vip.uword];
    addr.bytes[3] = commands[++vip.uword];
    vip.uword = addr.uword;

}

void Module::exec_je()
{
    if(flags.ZF == 1) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_jne()
{
    if(flags.ZF == 0) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_jg()
{
    if(flags.ZF == 0 && flags.CF == 0 && flags.SF == 0) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_jl()
{
    if(flags.ZF == 0 && flags.CF == 1 && flags.SF == 1) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_jge()
{
    if(flags.ZF == 1 || (flags.CF == 0 && flags.SF == 0)) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_jle()
{
    if(flags.ZF == 0 || (flags.CF == 1 && flags.SF == 1)) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_jc()
{
    if(flags.CF == 1) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_jnc()
{
    if(flags.CF == 0) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_ji()
{
    if(flags.IF == 1) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_jo()
{
    if(flags.OF == 1) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_jno()
{
    if(flags.OF == 0) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_js()
{
    if(flags.SF == 1) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_jns()
{
    if(flags.SF == 0) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_jp()
{
    if(flags.PF == 1) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_jnp()
{
    if(flags.PF == 0) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        vip.uword = addr.uword;
    }
    else vip.uword += 4;
}

void Module::exec_mov()
{

    //cout << "Mov vip 1: " << vip.uword << endl;
    PODTypes type1, type2, res;
    memory_unit* arg1;// = getPointer(type1);
    if(commands[vip.uword+1] == (byte)PODTypes::POINTER)  {
        //++vip.uword;
        array_ptr ptr = getArrayPointer(type1);
        *ptr = getArg(type2);
        applyArrayPointer(ptr);
    }
    else {
        arg1 = getPointer(type1);
        *arg1 = getArg(type2);
        ++vip.uword;
    }
    return;
    /*
    memory_unit *dest;
    PODTypes type;
    //++vip.uword;
    unsigned char modifier = commands[++vip.uword];
    type = (PODTypes)modifier;
    if(is_register(modifier)) {
        unsigned char offset = (unsigned char)OpCodes::VR1;
        dest = &regs[modifier - offset];
        //cout << (int)modifier << endl;
    }
    /*
    else if(modifier == (unsigned char)PODTypes::IDENTIFIER) {
        //type = (PODTypes) commands[++vip.uword];
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        dest = &globals[addr.word];
        type = (PODTypes)dest->bytes[8];
    }
    * /
    else if(modifier == (unsigned char)OpCodes::SQBRCK) {
        modifier = commands[++vip.uword];
        memory_unit tmp;
        tmp.reg = 0;
        if(modifier == (unsigned char)PODTypes::BYTE) {
            tmp.bytes[0] = commands[++vip.uword];
        }
        if(modifier == (unsigned char)PODTypes::HWORD) {
            tmp.bytes[0] = commands[++vip.uword];
            tmp.bytes[1] = commands[++vip.uword];
        }
        if(modifier == (unsigned char)PODTypes::WORD) {
            tmp.bytes[0] = commands[++vip.uword];
            tmp.bytes[1] = commands[++vip.uword];
            tmp.bytes[2] = commands[++vip.uword];
            tmp.bytes[3] = commands[++vip.uword];
        }
        /*
        else if(modifier == (unsigned char)PODTypes::IDENTIFIER) {
            memory_unit addr;
            addr.reg = 0;
            addr.bytes[0] = commands[++vip.uword];
            addr.bytes[1] = commands[++vip.uword];
            addr.bytes[2] = commands[++vip.uword];
            addr.bytes[3] = commands[++vip.uword];
            tmp = globals[addr.word];
            if(tmp.bytes[8] == (unsigned char)PODTypes::DWORD ||
                tmp.bytes[8] == (unsigned char)PODTypes::STRING) {
                cout << "Addres can't be double or string!" << endl;
                cout << dec << "\tat " << vip.uword << endl;
                throw -1;
            }
        }
        * /
        else if(is_register(modifier)) {
            unsigned char offset = (unsigned char)OpCodes::VR1;
            tmp.word = regs[commands[vip.uword] - offset].word;
        }
        dest = &prog_stack[tmp.word];
    }
    //++vip.uword;
    PODTypes t;
    *dest = getArg(t);
    dest->bytes[8] = (unsigned char)t;
    ++vip.uword;
    */
    //cout << "Mov vip 2: " << vip.uword << endl;
}

void Module::exec_mul() {
    PODTypes type1, type2, res;
    auto arg1 = getArg(type1);
    auto arg2 = getArg(type2);
    ++vip.uword;
    res = (unsigned char)type1 > (unsigned char)type2 ? type1 : type2;
    vra.reg = 0;
    vra.bytes[8] = (unsigned char)res;
    switch (res)
    {
    case PODTypes::BOOL:
        {vra.b = arg1.b + arg2.b;}
        break;
    case PODTypes::BYTE:
        {
            if(!CanMul(arg1.byte, arg2.byte)) flags.OF = 1;
            else flags.OF = 0;
            vra.byte = arg1.byte * arg2.byte;
            flags.CF = 0;
            flags.SF = 0;
        }
        break;
    case PODTypes::HWORD:
        {
            if(!CanMul(arg1.hword, arg2.hword)) flags.OF = 1;
            else flags.OF = 0;
            vra.hword = arg1.hword * arg2.hword;
            if(vra.hword < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::WORD:
        {
            if(!CanMul(arg1.word, arg2.word)) flags.OF = 1;
            else flags.OF = 0;
            vra.word = arg1.word * arg2.word;
            if(vra.word < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::W64:
        {
            if(!CanMul(arg1.reg, arg2.reg)) flags.OF = 1;
            else flags.OF = 0;
            vra.reg = arg1.reg * arg2.reg;
            if(vra.reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::DWORD:
        {
            if(type1 == PODTypes::DWORD && type2 == PODTypes::DWORD)
            {
                if(!CanMul(arg1.dword, arg2.dword)) flags.OF = 1;
                else flags.OF = 0;
                vra.dword = arg1.dword * arg2.dword;
            }
            else if((unsigned char)type1 < (unsigned char)type2)
            {
                if(!CanMul((double)arg1.reg, arg2.dword)) flags.OF = 1;
                else flags.OF = 0;
                vra.dword = arg1.reg * arg2.dword;
            }
            else
            {
                if(!CanMul(arg1.dword, (double)arg2.reg)) flags.OF = 1;
                else flags.OF = 0;
                vra.dword = arg1.dword * arg2.reg;
            }
            if(vra.reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    default:
        break;
    }
}

void Module::exec_neg()
{
    PODTypes type1;
    bool isptr = false;
    memory_unit* arg1, *tmp;// = getPointer(type1);
    array_ptr ptr;
    if(commands[vip.uword+1] == (byte)PODTypes::POINTER)  {
        //++vip.uword;
        isptr = true;
        ptr = getArrayPointer(type1);
        tmp = &ptr.val;
        //*ptr = getArg(type2);
        //applyArrayPointer(ptr);
    }
    else {
        arg1 = getPointer(type1);
        tmp = arg1;
        //*arg1 = getArg(type2);
        ++vip.uword;
    }

    switch (type1)
    {
    case PODTypes::BOOL:
        tmp->b = !tmp->b;
        break;
    case PODTypes::BYTE:
        {
            if(!CanMul(tmp->byte, (unsigned char)-1)) flags.OF = 1;
            else flags.OF = 0;
            tmp->byte *= -1;
            flags.CF = 0;
            flags.SF = 0;
        }
        break;
    case PODTypes::HWORD:
        {
            if(!CanMul(tmp->hword, (short)-1)) flags.OF = 1;
            else flags.OF = 0;
            tmp->hword *= -1;
            if(tmp->hword < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::WORD:
        {
            if(!CanMul(tmp->word, -1)) flags.OF = 1;
            else flags.OF = 0;
            tmp->word *= -1;
            if(tmp->word < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::W64:
        {
            if(!CanMul(tmp->reg, (__int64)-1)) flags.OF = 1;
            else flags.OF = 0;
            tmp->reg *= -1;
            if(tmp->reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::DWORD:
        {
            if(!CanMul(tmp->dword, (double)-1)) flags.OF = 1;
            else flags.OF = 0;
            tmp->dword *= -1;
            if(tmp->dword < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    default:
        break;
    }
    if(isptr)
        applyArrayPointer(ptr);

/*
    memory_unit *dest;
    PODTypes type;
    //++vip.uword;
    unsigned char modifier = commands[++vip.uword];
    type = (PODTypes)modifier;
    if(is_register(modifier)) {
        unsigned char offset = (unsigned char)OpCodes::VR1;
        dest = &regs[modifier - offset];
        type = (PODTypes)dest->bytes[8];
    }

    else if(modifier == (unsigned char)PODTypes::IDENTIFIER) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        dest = &globals[addr.word];
        type = (PODTypes)dest->bytes[8];
    }

    else if(modifier == (unsigned char)OpCodes::SQBRCK) {
        modifier = commands[++vip.uword];
        memory_unit tmp;
        tmp.reg = 0;
        if(modifier == (unsigned char)PODTypes::BYTE) {
            tmp.bytes[0] = commands[++vip.uword];
        }
        if(modifier == (unsigned char)PODTypes::HWORD) {
            tmp.bytes[0] = commands[++vip.uword];
            tmp.bytes[1] = commands[++vip.uword];
        }
        if(modifier == (unsigned char)PODTypes::WORD) {
            tmp.bytes[0] = commands[++vip.uword];
            tmp.bytes[1] = commands[++vip.uword];
            tmp.bytes[2] = commands[++vip.uword];
            tmp.bytes[3] = commands[++vip.uword];
        }
        /*
        else if(modifier == (unsigned char)PODTypes::IDENTIFIER) {
            memory_unit addr;
            addr.reg = 0;
            addr.bytes[0] = commands[++vip.uword];
            addr.bytes[1] = commands[++vip.uword];
            addr.bytes[2] = commands[++vip.uword];
            addr.bytes[3] = commands[++vip.uword];
            tmp = globals[addr.word];
            if(tmp.bytes[8] == (unsigned char)PODTypes::DWORD ||
                tmp.bytes[8] == (unsigned char)PODTypes::STRING) {
                cout << "Addres can't be double or string!" << endl;
                cout << dec << "\tat " << vip.uword << endl;
                throw -1;
            }
        }
        else if(is_register(modifier)) {
            unsigned char offset = (unsigned char)OpCodes::VR1;
            tmp.word = regs[commands[vip.uword] - offset].word;
        }
        dest = &prog_stack[tmp.word];
        type = (PODTypes)dest->bytes[8];
    }
    ++vip.uword;
    switch (type)
    {
    case PODTypes::BOOL:
        dest->b = !dest->b;
        break;
    case PODTypes::BYTE:
        {
            if(!CanMul(dest->byte, (unsigned char)-1)) flags.OF = 1;
            else flags.OF = 0;
            dest->byte *= -1;
            flags.CF = 0;
            flags.SF = 0;
        }
        break;
    case PODTypes::HWORD:
        {
            if(!CanMul(dest->hword, (short)-1)) flags.OF = 1;
            else flags.OF = 0;
            dest->hword *= -1;
            if(dest->hword < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::WORD:
        {
            if(!CanMul(dest->word, -1)) flags.OF = 1;
            else flags.OF = 0;
            dest->word *= -1;
            if(dest->word < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::W64:
        {
            if(!CanMul(dest->reg, (__int64)-1)) flags.OF = 1;
            else flags.OF = 0;
            dest->reg *= -1;
            if(dest->reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::DWORD:
        {
            if(!CanMul(dest->dword, (double)-1)) flags.OF = 1;
            else flags.OF = 0;
            dest->dword *= -1;
            if(dest->reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    default:
        break;
    }
*/
}

void Module::exec_not() {
    PODTypes type1;
    bool isptr = false;
    memory_unit* arg1, *tmp;// = getPointer(type1);
    array_ptr ptr;
    if(commands[vip.uword+1] == (byte)PODTypes::POINTER)  {
        //++vip.uword;
        isptr = true;
        ptr = getArrayPointer(type1);
        tmp = &ptr.val;
        //*ptr = getArg(type2);
        //applyArrayPointer(ptr);
    }
    else {
        arg1 = getPointer(type1);
        tmp = arg1;
        //*arg1 = getArg(type2);
        ++vip.uword;
    }
    switch (type1)
    {
    case PODTypes::BOOL:
        tmp->b = !tmp->b;
        break;
    case PODTypes::BYTE:
        tmp->byte = ~tmp->byte;
        break;
    case PODTypes::HWORD:
        tmp->hword = ~tmp->hword;
        break;
    case PODTypes::WORD:
        tmp->word = ~tmp->word;
        break;
    case PODTypes::W64:
        tmp->reg = ~tmp->reg;
        break;
    default:
        throw "Illegal arg in 'not' command!";
        break;
    }
    if(isptr)
        applyArrayPointer(ptr);
    return;
}

void Module::exec_or()
{
    PODTypes type1, type2, res;
    auto arg1 = getArg(type1);
    auto arg2 = getArg(type2);
    ++vip.uword;
    res = (unsigned char)type1 > (unsigned char)type2 ? type1 : type2;
    vra.reg = 0;
    vra.bytes[8] = (unsigned char)res;
    switch (res)
    {
    case PODTypes::BOOL:
        {vra.b = arg1.b | arg2.b;}
        break;
    case PODTypes::BYTE:
        {vra.byte = arg1.byte | arg2.byte;}
        break;
    case PODTypes::HWORD:
        {vra.hword = arg1.hword | arg2.hword;}
        break;
    case PODTypes::WORD:
        {vra.word = arg1.word | arg2.word;}
        break;
    case PODTypes::W64:
        {vra.reg = arg1.reg | arg2.reg;}
        break;
    case PODTypes::DWORD:
        {
            //ERROR
        }
        break;
    default:
        break;
    }
}

void Module::exec_pop() {
    PODTypes t;
    auto c = getArg(t);
    //auto i = commands[++vip.uword];
    //++vip.uword;
    for (int t = 0; t < c.uword; t++)
        prog_stack.pop_back();
    //if(prog_stack.size() == 0)
    //    vsp.word = 0;
    //else
        vsp.uword = prog_stack.size() == 0 ? 0 : prog_stack.size()-1;
    ++vip.uword;
}

void Module::exec_push()
{
    PODTypes t;
    vsp.uword = prog_stack.size();
    prog_stack.push_back(getArg(t));
    ++vip.uword;
    //if(prog_stack.size() == 0)
    //    vsp.word = 0;
    //else
}

//#ifdef WIN32

void Module::exec_rol()
{
    PODTypes type1, type2, res;
    auto arg1 = getArg(type1);
    auto arg2 = getArg(type2);
    ++vip.uword;
    uint target = arg1.uword;

    asm volatile ("roll %1,%0" : "+r" (target) : "c" (arg2.bytes[0]));

    vra.reg = 0;
    vra.uword = target;
    vra.bytes[8] = (byte)PODTypes::WORD;
    //cout << "Mov vip 1: " << vip.uword << endl;
    /*PODTypes type1, type2, res;
    memory_unit* arg1;// = getPointer(type1);
    if(commands[vip.uword+1] == (byte)PODTypes::POINTER)  {
        //++vip.uword;
        array_ptr ptr = getArrayPointer(type1);
        //*ptr =
        auto sh = getArg(type2);

        applyArrayPointer(ptr);
    }
    else {
        arg1 = getPointer(type1);
        *arg1 = getArg(type2);
        ++vip.uword;
    }*/
    return;
}

void Module::exec_ror() {
    PODTypes type1, type2, res;
    auto arg1 = getArg(type1);
    auto arg2 = getArg(type2);
    ++vip.uword;
    uint target = arg1.uword;

    asm volatile ("rorl %1,%0" : "+r" (target) : "c" (arg2.bytes[0]));

    vra.reg = 0;
    vra.uword = target;
    vra.bytes[8] = (byte)PODTypes::WORD;
    return;
}
//void exec_set();

void Module::exec_shl() {
    PODTypes type1, type2, res;
    auto arg1 = getArg(type1);
    auto arg2 = getArg(type2);
    ++vip.uword;

    vra.reg = 0;
    vra.uword = arg1.uword << arg2.bytes[0];
    vra.bytes[8] = (byte)PODTypes::WORD;
    return;
}

void Module::exec_shr() {
    PODTypes type1, type2, res;
    auto arg1 = getArg(type1);
    auto arg2 = getArg(type2);
    ++vip.uword;

    vra.reg = 0;
    vra.uword = arg1.uword >> arg2.bytes[0];
    vra.bytes[8] = (byte)PODTypes::WORD;
    return;
}

//#else
#warning ROL, ROR, SHL, SHR unsupported on ARM architecture
//#endif

void Module::exec_ret() {
    if(call_stack.size() == 0 && rt->GetMainModule() == this) exec_exit(vr1.word);
    else if(call_stack.size() != 0) {
        vip.uword = call_stack.top();
        call_stack.pop();
    }
}

void Module::exec_exit() {
    rt->log << "\033[0m";
    exit(0);
}

void Module::exec_exit(int res) {
    rt->log << "\033[0m";
    exit(res);
}

void Module::exec_sub() {
    PODTypes type1, type2, res;
    auto arg1 = getArg(type1);
    auto arg2 = getArg(type2);
    ++vip.uword;
    res = (unsigned char)type1 > (unsigned char)type2 ? type1 : type2;
    vra.reg = 0;
    vra.bytes[8] = (unsigned char)res;
    switch (res)
    {
    case PODTypes::BOOL:
        {vra.b = arg1.b + arg2.b;}
        break;
    case PODTypes::BYTE:
        {
            if(!CanSub(arg1.byte, arg2.byte)) flags.OF = 1;
            else flags.OF = 0;
            vra.byte = arg1.byte - arg2.byte;
            flags.CF = 0;
            flags.SF = 0;
        }
        break;
    case PODTypes::HWORD:
        {
            if(!CanSub(arg1.hword, arg2.hword)) flags.OF = 1;
            else flags.OF = 0;
            vra.hword = arg1.hword - arg2.hword;
            if(vra.hword < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::WORD:
        {
            if(!CanSub(arg1.word, arg2.word)) flags.OF = 1;
            else flags.OF = 0;
            vra.word = arg1.word - arg2.word;
            if(vra.word < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::W64:
        {
            if(!CanSub(arg1.reg, arg2.reg)) flags.OF = 1;
            else flags.OF = 0;
            vra.reg = arg1.reg - arg2.reg;
            if(vra.reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    case PODTypes::DWORD:
        {
            if(type1 == PODTypes::DWORD && type2 == PODTypes::DWORD)
            {
                if(!CanSub(arg1.dword, arg2.dword)) flags.OF = 1;
                else flags.OF = 0;
                vra.dword = arg1.dword - arg2.dword;
            }
            else if((unsigned char)type1 < (unsigned char)type2)
            {
                if(!CanSub((double)arg1.reg, arg2.dword)) flags.OF = 1;
                else flags.OF = 0;
                vra.dword = arg1.reg - arg2.dword;
            }
            else
            {
                if(!CanSub(arg1.dword, (double)arg2.reg)) flags.OF = 1;
                else flags.OF = 0;
                vra.dword = arg1.dword - arg2.reg;
            }
            if(vra.reg < 0) {
                flags.CF = 1;
                flags.SF = 1;
            }
            else {
                flags.CF = 0;
                flags.SF = 0;
            }
        }
        break;
    default:
        break;
    }
}

//void exec_xch();

void Module::exec_xor()
{
    PODTypes type1, type2, res;
    auto arg1 = getArg(type1);
    auto arg2 = getArg(type2);
    ++vip.uword;
    res = (unsigned char)type1 > (unsigned char)type2 ? type1 : type2;
    vra.reg = 0;
    vra.bytes[8] = (unsigned char)res;
    switch (res)
    {
    case PODTypes::BOOL:
        {vra.b = arg1.b | arg2.b;}
        break;
    case PODTypes::BYTE:
        {vra.byte = arg1.byte ^ arg2.byte;}
        break;
    case PODTypes::HWORD:
        {vra.hword = arg1.hword ^ arg2.hword;}
        break;
    case PODTypes::WORD:
        {vra.word = arg1.word ^ arg2.word;}
        break;
    case PODTypes::W64:
        {vra.reg = arg1.reg ^ arg2.reg;}
        break;
    case PODTypes::DWORD:
        {
            //ERROR
        }
        break;
    default:
        break;
    }
}

void Module::exec_stdout()  {
    //rt->PrintStringsTrace();
    cout << "\033[0m";
    PODTypes cast = PODTypes::_NULL, t1;
    auto arg = getArg(t1);
    switch (t1)
    {
    case PODTypes::BOOL:
        cout << arg.b;
        break;
    case PODTypes::BYTE:
        cout << arg.bytes[0];
        break;
    case PODTypes::HWORD:
        cout << arg.hword;
        break;
    case PODTypes::WORD:
        cout << arg.word;
        break;
    case PODTypes::W64:
        cout << arg.reg;
        break;
    case PODTypes::DWORD:
        cout << arg.dword;
        break;
    case PODTypes::STRING:
        //rt->PrintStringsTrace();
        cout << str_stack[arg.word];
        //cout << "arg.word: " << dec << arg.word << endl;
        //rt->PrintStringsTrace();
        break;
    default:
        break;
    }

    ++vip.uword;
    return;

    /*memory_unit first;
    first.reg = 0;
    ++vip.uword;
    unsigned char modifier = commands[vip.uword];
    if(modifier == (unsigned char)OpCodes::CAST) {
        cast = (PODTypes)commands[++vip.uword];
        ++vip.uword;
        modifier = commands[vip.uword];
    }
    if(modifier == (unsigned char)PODTypes::BOOL) {
        first.bytes[0] = commands[++vip.uword];
        t1 = PODTypes::BOOL;
    }
    else if(modifier == (unsigned char)PODTypes::BYTE) {
        first.bytes[0] = commands[++vip.uword];
        t1 = PODTypes::BYTE;
    }
    else if(modifier == (unsigned char)PODTypes::HWORD) {
        first.bytes[0] = commands[++vip.uword];
        first.bytes[1] = commands[++vip.uword];
        t1 = PODTypes::HWORD;
    }
    else if(modifier == (unsigned char)PODTypes::WORD) {
        first.bytes[0] = commands[++vip.uword];
        first.bytes[1] = commands[++vip.uword];
        first.bytes[2] = commands[++vip.uword];
        first.bytes[3] = commands[++vip.uword];
        t1 = PODTypes::WORD;
    }
    else if(modifier == (unsigned char)PODTypes::DWORD) {
        first.bytes[0] = commands[++vip.uword];
        first.bytes[1] = commands[++vip.uword];
        first.bytes[2] = commands[++vip.uword];
        first.bytes[3] = commands[++vip.uword];
        first.bytes[4] = commands[++vip.uword];
        first.bytes[5] = commands[++vip.uword];
        first.bytes[6] = commands[++vip.uword];
        first.bytes[7] = commands[++vip.uword];
        t1 = PODTypes::DWORD;
    }
    else if (modifier == (unsigned char)PODTypes::STRING)
    {
        memory_unit temp;
        temp.bytes[0] = commands[++vip.uword];
        temp.bytes[1] = commands[++vip.uword];
        temp.bytes[2] = commands[++vip.uword];
        temp.bytes[3] = commands[++vip.uword];
        cout << "TRACE: .size() = " << str_stack.size() << " idx = " << temp.uword << endl;
        cout << str_stack[temp.word];
        ++vip.uword;
        return;
    }
    else if(is_register(modifier)) {
        unsigned char offset = (unsigned char)OpCodes::VR1;
        first = regs[commands[vip.uword] - offset];
        t1 = (PODTypes)first.bytes[8];
    }
    /*
    else if(modifier == (unsigned char)PODTypes::IDENTIFIER) {
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        first = globals[addr.word];
        t1 = (PODTypes)first.bytes[8];
    }
    else if(modifier == (unsigned char)OpCodes::SQBRCK) {
        modifier = commands[++vip.uword];
        memory_unit tmp;
        tmp.reg = 0;
        if(modifier == (unsigned char)PODTypes::BYTE) {
            tmp.bytes[0] = commands[++vip.uword];
        }
        if(modifier == (unsigned char)PODTypes::HWORD) {
            tmp.bytes[0] = commands[++vip.uword];
            tmp.bytes[1] = commands[++vip.uword];
        }
        if(modifier == (unsigned char)PODTypes::WORD) {
            tmp.bytes[0] = commands[++vip.uword];
            tmp.bytes[1] = commands[++vip.uword];
            tmp.bytes[2] = commands[++vip.uword];
            tmp.bytes[3] = commands[++vip.uword];
        }

        else if(modifier == (unsigned char)PODTypes::IDENTIFIER) {
            memory_unit addr;
            addr.reg = 0;
            addr.bytes[0] = commands[++vip.uword];
            addr.bytes[1] = commands[++vip.uword];
            addr.bytes[2] = commands[++vip.uword];
            addr.bytes[3] = commands[++vip.uword];
            tmp = globals[addr.word];
            if(tmp.bytes[8] == (unsigned char)PODTypes::DWORD ||
                tmp.bytes[8] == (unsigned char)PODTypes::STRING) {
                cout << "Addres can't be double or string!" << endl;
                cout << dec << "\tat " << vip.uword << endl;
                throw -1;
            }
        }
        else if(is_register(modifier)) {
            unsigned char offset = (unsigned char)OpCodes::VR1;
            tmp.word = regs[commands[vip.uword] - offset].word;
        }
        first = prog_stack[tmp.word];
        t1 = (PODTypes)first.bytes[8];
    }
    if(cast != PODTypes::_NULL) {
        if(t1 < PODTypes::DWORD && cast == PODTypes::DWORD) {
            if(t1 != PODTypes::BOOL) cout << (double)first.reg;
            else cout << (double)first.b;
        }
        else if(t1 == PODTypes::DWORD && cast < PODTypes::DWORD) {
            cout << (__int64)first.dword;
        }
    }
    else {
        if(t1 == PODTypes::DWORD) cout << first.dword;
        else if(t1 == PODTypes::BOOL) cout << first.b;
        else if(t1 == PODTypes::STRING) cout << str_stack[first.word];
        else cout << first.reg;
    }
    ++vip.uword;*/
}

void Module::exec_stdin()
{
    PODTypes type1;
    //cout << "\nBEFORE STDIN" << endl;
    if(commands[vip.uword+1] == (byte)PODTypes::POINTER)  {
        array_ptr ptr;
        //++vip.uword;
        ptr = getArrayPointer(type1);
        //*ptr = getArg(type2);
        //applyArrayPointer(ptr);
        if (type1 != PODTypes::STRING)
            (*ptr).reg = 0;
        if(type1 == PODTypes::DWORD) cin >> (*ptr).dword;
        else if (type1 == PODTypes::BOOL) cin >> (*ptr).b;
        else if (type1 == PODTypes::STRING)
            cin >> str_stack[(*ptr).word];
        else cin >> (*ptr).reg;
        applyArrayPointer(ptr);
    }
    else if(commands[vip.uword+1] == (byte)PODTypes::_NULL) {
        ++vip.uword;
        type1 = (PODTypes)commands[++vip.uword];

        if(type1 == PODTypes::_NULL) {
            cin.get();
            ++vip.uword;
            return;
        }

        memory_unit val;// = getPointer(type1);
        val.reg = 0;
        if(type1 == PODTypes::DWORD) cin >> val.dword;
        else if (type1 == PODTypes::BOOL) cin >> val.b;
        else if (type1 == PODTypes::STRING) {
            string str;
            cin >> str;
            val.uword = str_stack.size();
            str_stack.push_back(str);
            //++vip.uword;
            //return;
        }
        else cin >> val.reg;
        val.bytes[8] = (byte)type1;
        vsp.uword = prog_stack.size();
        prog_stack.push_back(val);
        //*arg1 = getArg(type2);
        ++vip.uword;
    }
    else {
        memory_unit* ptr;// = getPointer(type1);
        ptr = getPointer(type1);
        if (type1 != PODTypes::STRING)
            ptr->reg = 0;
        if(type1 == PODTypes::DWORD) cin >> ptr->dword;
        else if (type1 == PODTypes::BOOL) cin >> ptr->b;
        else if (type1 == PODTypes::STRING)
            cin >> str_stack[ptr->word];
        else cin >> ptr->reg;
        //*arg1 = getArg(type2);
        ++vip.uword;
    }
    //cout << "\nAFTER STDIN" << endl;
    return;
    /*memory_unit *dest;
    PODTypes cast = PODTypes::_NULL, type;
    //++vip.uword;
    unsigned char modifier = commands[++vip.uword];
    //type = (PODTypes)modifier;
    if(modifier == (unsigned char)OpCodes::CAST) {
        cast = (PODTypes)commands[++vip.uword];
        ++vip.uword;
        modifier = commands[vip.uword];
        type = cast;
    }
    if(is_register(modifier)) {
        unsigned char offset = (unsigned char)OpCodes::VR1;
        dest = &regs[modifier - offset];
        if(cast != PODTypes::_NULL)
            dest->bytes[8] = (unsigned char)cast;
        type = (PODTypes)dest->bytes[8];
    }

    else if(modifier == (unsigned char)PODTypes::IDENTIFIER) {
        //type = (PODTypes) commands[++vip.uword];
        memory_unit addr;
        addr.reg = 0;
        addr.bytes[0] = commands[++vip.uword];
        addr.bytes[1] = commands[++vip.uword];
        addr.bytes[2] = commands[++vip.uword];
        addr.bytes[3] = commands[++vip.uword];
        dest = &globals[addr.word];
        type = (PODTypes)dest->bytes[8];
    }
    * /
    else if(modifier == (unsigned char)OpCodes::SQBRCK) {
        modifier = commands[++vip.uword];
        memory_unit tmp;
        tmp.reg = 0;
        if(modifier == (unsigned char)PODTypes::BYTE) {
            tmp.bytes[0] = commands[++vip.uword];
        }
        if(modifier == (unsigned char)PODTypes::HWORD) {
            tmp.bytes[0] = commands[++vip.uword];
            tmp.bytes[1] = commands[++vip.uword];
        }
        if(modifier == (unsigned char)PODTypes::WORD) {
            tmp.bytes[0] = commands[++vip.uword];
            tmp.bytes[1] = commands[++vip.uword];
            tmp.bytes[2] = commands[++vip.uword];
            tmp.bytes[3] = commands[++vip.uword];
        }
        /*
        else if(modifier == (unsigned char)PODTypes::IDENTIFIER) {
            memory_unit addr;
            addr.reg = 0;
            addr.bytes[0] = commands[++vip.uword];
            addr.bytes[1] = commands[++vip.uword];
            addr.bytes[2] = commands[++vip.uword];
            addr.bytes[3] = commands[++vip.uword];
            tmp = globals[addr.word];
            if(tmp.bytes[8] == (unsigned char)PODTypes::DWORD ||
                tmp.bytes[8] == (unsigned char)PODTypes::STRING) {
                cout << "Addres can't be double or string!" << endl;
                cout << dec << "\tat " << vip.uword << endl;
                throw -1;
            }
        }
        * /
        else if(is_register(modifier)) {
            unsigned char offset = (unsigned char)OpCodes::VR1;
            tmp.word = regs[commands[vip.uword] - offset].word;
        }
        dest = &prog_stack[tmp.word];
        if(cast != PODTypes::_NULL)
            dest->bytes[8] = (unsigned char)cast;
        type = (PODTypes)dest->bytes[8];
    }
    ++vip.uword;
    if (type != PODTypes::STRING)
        dest->reg = 0;
    if(type == PODTypes::DWORD) cin >> dest->dword;
    else if (type == PODTypes::BOOL) cin >> dest->b;
    else if (type == PODTypes::STRING)
        cin >> str_stack[dest->word];
    else cin >> dest->reg;*/
}


