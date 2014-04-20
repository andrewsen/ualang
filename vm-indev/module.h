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

#ifndef MODULE_H
#define MODULE_H

#include <vector>
#include <stack>
#include <iomanip>
#include <fstream>
#include <iostream>
//#include <ctime>
#include "basic_structs.h"
//#include "String.h"
using namespace std;
//#include "limits.cpp"

extern istream *in;
extern ostream *out;
extern ostream *err;
extern ostream *log;

class Runtime;

using namespace std;

struct FlagsReg {
    unsigned CF : 1; // ���� ��������
    unsigned PF : 1; // ���� �������
    unsigned AF : 1;
    unsigned ZF : 1; // ���� ����
    unsigned SF : 1; // ���� �����
    unsigned TF : 1; // ���� �����������
    unsigned OF : 1; // ���� ������������
    unsigned IF : 1; // Illegal Command Flag
};
extern FlagsReg flags;

class Module
{
public:
    class Function {
        friend class Module;
        string signature;
        uint addr;
        PODTypes returnType;
    public:
        Function(PODTypes ret, const string& sign, int addr) {
            returnType = ret;
            this->signature = sign;
            this->addr = addr;
        }

        //void Call();
        uint GetAddr() {
            return addr;
        }
    };
    union Flags
    {
        struct
        {
            unsigned canExecute : 1;
            unsigned isKernelModule : 1;
            unsigned canBeLinked : 1;
            unsigned useOsAPI : 1;
            unsigned isBinding : 1;
            unsigned reserved : 3;
        } ;
        byte value;
    } modFlags;


private:

    Runtime * rt;
    //SegmentedModule * segmod = nullptr;
    struct Table {
        memory_unit real_addr_ptr,real_array_ptr, real_string_ptr, stack_ptr, array_ptr, com_ptr, fun_ptr, str_ptr;
    } ptable;

    struct array_ptr {
        uint aid;
        uint pos;
        memory_unit val;

        memory_unit& operator* () {
            return val;
        }
    };


    string name;

    uint moduleOffset = 0;

    uint entry_addr;
    uint stackOffset = 0, arrayOffset = 0, stringOffset = 0;
    uint rdp = 0, rrp = 0, rsp = 0;

    //vector<Module> modules;
    //vector<memory_unit> globals;		//COMPILABLE
    //vector<memory_unit> prog_stack;
    vector<unsigned char> commands;			//COMPILABLE
    stack<int> call_stack;
    vector<unsigned int> real_addr_table;
    vector<unsigned int> real_array_table;
    vector<unsigned int> real_string_table;
    vector<Function> funcs;

    string entry;

    //int array[42];

    int compiler_ver;
    int target_ver;
    int min_ver;
    int max_ver;

    vector<Module> imports;

    friend class Runtime;
    friend void memUnitReader(ifstream &ifs, uint end, vector<memory_unit>& v);
    friend void stringReader(ifstream &ifs, uint end, vector<string>& v);
    friend void arrayReader(ifstream &ifs, uint end, vector<Array>& v);
public:

    static const int NEW_SIGN = 0xDEADBABE;

    Module(const string& file, Runtime *rt, bool _main=false);
    Module() {}

    ~Module() {
        //if(segmod != nullptr)
        //    delete segmod;
    }

    void Load();

    void Run(string args);

    void Call(uint addr);
    void Execute();
    void Execute(unsigned int addr);
    void Execute(const string &func, memory_unit args[]);
    bool IsExecutable();

private:
    inline bool is_register (unsigned char reg) {
        if(reg < (unsigned char)OpCodes::VR1 || reg > (unsigned char)OpCodes::VRA) return false;
        return true;
    }

    void loadOld(ifstream &ifs);
    void loadNew(ifstream &ifs);

    memory_unit getArg (PODTypes &t1);
    memory_unit* getPointer (PODTypes &t1);
    array_ptr getArrayPointer(PODTypes &t1);
    void applyArrayPointer(array_ptr ptr);
    int readInt(ifstream &ifs);
    memory_unit readMemUnit(ifstream &ifs);
    void read_bytecode(ifstream &ifs);
    void read_stack(ifstream &ifs);
    void read_arrays(ifstream &ifs);
    void read_functions(ifstream &ifs);
    void read_strpool(ifstream &ifs);
    void read_imports(ifstream &ifs);

    //virtual bool loadBinaryNM(pheader h, istream &reader);
    //virtual bool loadMetaNM(pheader h, istream &reader);
    //virtual bool loadMemUnit(pheader h, istream &reader);
    //virtual bool loadStrings(pheader h, istream &reader);
    //virtual bool loadArrays(pheader h, istream &reader);
    //virtual bool loadTyped(pheader h, istream &reader);

    void exec_add();

    void exec_alloc();

    void exec_and();

    void exec_call();

    void exec_calle();

    void exec_callne();

    void exec_cmp();

    void exec_dec();

    void exec_div();

    void exec_free();

    void exec_inc();

    void exec_jmp();

    void exec_je();

    void exec_jne();

    void exec_jg();

    void exec_jl();

    void exec_jge();

    void exec_jle();

    void exec_jc();

    void exec_jnc();

    void exec_ji();

    void exec_jo();

    void exec_jno();

    void exec_js();

    void exec_jns();

    void exec_jp();

    void exec_jnp();

    void exec_mov();

    void exec_mul();

    void exec_neg();

    void exec_not();

    void exec_or();

    void exec_pop();

    void exec_push();

    //#ifdef WIN32
    void exec_rol();

    void exec_ror();

    void exec_shl();

    void exec_shr();

    //#else
    //#warning ROL, ROR, SHL, SHR unsupported on ARM architecture
    //#endif

    void exec_ret();

    void exec_exit();

    void exec_exit(int res);

    void exec_sub();

    //void exec_xch();

    void exec_xor();

    void exec_stdout();

    void exec_stdin();
};

#endif // MODULE_H
