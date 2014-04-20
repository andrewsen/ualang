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
#include <sys/stat.h>

using namespace std;

void memUnitReader(ifstream &ifs, uint end, vector<memory_unit>& v);

bool exists (const char* file) {
    struct stat fileAtt; //the type stat and function stat have exactly the same names, so to refer the type, we put struct before it to indicate it is an structure.

    //Use the stat function to get the information
    if (stat(file, &fileAtt) != 0) //start will be 0 when it succeeds
        return false; //So on non-zero, throw an exception

    //S_ISREG is a macro to check if the filepath referers to a file. If you don't know what a macro is, it's ok, you can use S_ISREG as any other function, it 'returns' a bool.
    return S_ISREG(fileAtt.st_mode);
}

Module::Module(const string &file, Runtime * rt, bool _main)
{
    this->rt = rt;
    if(file[0] == '/' || _main) {                            /// FIXME!!!

        name = file;
        rt->log << "LOG: головний модуль" << file << endl;
        return;
    }

    string env = getenv("SVM_PATH");
    auto pathes = split(env, ':', true);

    name = "";
    //cout << strlen("/home/senko/qt/Framework/build-vm-indev-Desktop-Debug/test.sky.slm") << endl;
    //if(exists(string("/home/senko/qt/Framework/build-vm-indev-Desktop-Debug/") + string("test.sky") + string(".slm")))
    //   cout << "EXISTS!" << endl;
    for (int i = 0; i < pathes.size(); ++i) {
        if(pathes[i] == "") break;
        //rt->log << "Looking for module " << file << " in path " << pathes[i] << endl;
        //ifstream test;
        //test.open(pathes[i] + file + ".slm");
        rt->log << "Пошук модулю" << pathes[i] + file + ".slm\n";
        //rt->log << "Or module " << pathes[i] + file << endl;
        const char * temp = (pathes[i] + file + ".slm").c_str();
        //rt->log << strlen(temp) << endl << (pathes[i] + file + ".slm").length() << endl;
        if(exists(temp)) {
            name = pathes[i] + file + ".slm";
            //rt->log << "LOG: module '" << name << "' exists!" << endl;
            break;
        }
        else if(exists((pathes[i] + file).c_str())) {
            name = pathes[i] + file;
            //rt->log << "LOG: module '" << name << "' exists!" << endl;
            break;
        }
    }
    if(name == "") throw RuntimeException("Модуль " + file + " Не знайдено!");

    //name = file;
}

void Module::Load() {
    //rt->log << "LOG: " << "loading module " << name << endl;
    ifstream ifs(name, ios::binary);
    if(ifs.bad()) {
        cout << "Very, very bad file! :'(";
        //cin.get();
    }

    //while (!ifs.eof()) {
    //    rt->log << hex << (unsigned)(byte) ifs.get() << " ";
    //}
    //rt->log << dec << "\n";
    //ifs.seekg(0, ios::beg);

    string vasm;
    for (int i = 0; i < 4; i++)
        vasm += (char)ifs.get();

    //rt->log << "'\n";

    if(vasm == "VASM") {
        loadOld(ifs);
    }
    //else if(atoi(vasm.c_str()) == Module::NEW_SIGN) {
    //    ifs.get((char&)modFlags.value);
    //    loadNew(ifs);
    //}
    else throw "unknown magic " + vasm + " in module '" + this->name + "'";

    return;

}

void Module::loadOld(ifstream &ifs) {
    compiler_ver = readInt(ifs);
    target_ver = readInt(ifs);
    min_ver = readInt(ifs);
    max_ver = readInt(ifs);

    ifs.get((char&)modFlags.value);

    //canExecute = modFlags.canExecute;

    //int pp = readInt(ifs), t = 0;

    if(modFlags.canExecute)
        entry_addr = readInt(ifs);

    //entry = "";

    //while ((t = ifs.tellg()) != pp) {
    //    char ch;
    //    ifs.get(ch);
    //    entry = entry.Append(ch);
    //}

    if(min_ver > Runtime::VERSION){
        throw "Version of your run-time is too old!";
        return;
    }

    ptable.real_addr_ptr.word = readInt(ifs);
    ptable.real_array_ptr.word = readInt(ifs);
    ptable.real_string_ptr.word = readInt(ifs);
    ptable.stack_ptr.word = readInt(ifs);
    ptable.array_ptr.word = readInt(ifs);
    ptable.com_ptr.word = readInt(ifs);
    //ptable.fun_tab_ptr.word = readInt(ifs);
    ptable.fun_ptr.word = readInt(ifs);
    //ptable.str_tab_ptr.word = readInt(ifs);
    ptable.str_ptr.word = readInt(ifs);
    rt->log << "Таблиці:\n---" << hex << endl
            << "\tRDP " << ptable.real_addr_ptr.word << endl
            << "\tRRP " << ptable.real_array_ptr.word << endl
            << "\tRSP " << ptable.real_string_ptr.word << endl
            << "\tSP " << ptable.stack_ptr.word << endl
            << "\tAP " << ptable.array_ptr.word << endl
            << "\tCOM " << ptable.com_ptr.word << endl
            << "\tFUN " << ptable.fun_ptr.word << endl
            << "\tSTR " << ptable.str_ptr.word << endl
            << "---" << endl
            << dec
               ;
    //rt->log << "Offset tables loaded" << endl;
    read_imports(ifs);
    //rt->log << "Import modules loaded" << endl;
    //if(this == NULL) throw "This is NULL!!!!";
    //cout << "In module " << endl;
    ifs.seekg(ptable.real_addr_ptr.word, ios::beg);
    int temp;
    while ((temp = ifs.tellg()) < ptable.real_array_ptr.word)
        real_addr_table.push_back(readInt(ifs));

    ifs.seekg(ptable.real_array_ptr.word, ios::beg);
    while ((temp = ifs.tellg()) < ptable.real_string_ptr.word)
        real_array_table.push_back(readInt(ifs));

    ifs.seekg(ptable.real_string_ptr.word, ios::beg);
    while ((temp = ifs.tellg()) < ptable.stack_ptr.word)
        real_string_table.push_back(readInt(ifs));

    rt->log << "\nІніциалізація стеку... ";
    read_stack(ifs);
    rt->log << "ОК\nІніціалізація масивів... ";
    read_arrays(ifs);
    rt->log << "ОК\nЗавантаження байт-коду... ";
    read_bytecode(ifs);
    rt->log << "ОК\nЗавантаження сигнатур функцій... ";
    read_functions(ifs);
    rt->log << "ОК\nІніціалізація стеку рядків... ";
    read_strpool(ifs);
    rt->log << "ОК\n";
    ifs.close();
}

/*void Module::loadNew(ifstream &ifs) {
    segmod = new SegmentedModule(ifs);
    SegmentedModule &s = *segmod;

    Segment* seg = &s["code"];
    if(!seg->IsRaw() && !seg->IsUninited()) {
        seg->Move(commands);
    }

    seg = &s.GetRaw("stack");
    seg->ReadRawData(memUnitReader, prog_stack);

    //Segment& st = s["stack"];
}*/

void Module::Run(string args)
{
    rt->log << "Виклик вхідної функції за адресою 0x" << hex << entry_addr << dec << endl;
    if(!this->modFlags.canExecute) throw "Бібліотечний модуль не може бути виконаний";
    //vip.uword = 0;
    //prog_stack.clear();

    //rt->PrintStringsTrace();
    //cout << "\tEntry: " << entry_addr << endl;
    vip.uword = entry_addr;
    Execute();
}

int Module::readInt(ifstream &ifs) {
    memory_unit u;
    ifs.get((char &)u.bytes[0]);
    ifs.get((char &)u.bytes[1]);
    ifs.get((char &)u.bytes[2]);
    ifs.get((char &)u.bytes[3]);
    return u.word;
}

memory_unit Module::readMemUnit(ifstream &ifs) {
    memory_unit u;
    ifs.get((char &)u.bytes[0]);
    ifs.get((char &)u.bytes[1]);
    ifs.get((char &)u.bytes[2]);
    ifs.get((char &)u.bytes[3]);
    ifs.get((char &)u.bytes[4]);
    ifs.get((char &)u.bytes[5]);
    ifs.get((char &)u.bytes[6]);
    ifs.get((char &)u.bytes[7]);
    ifs.get((char &)u.bytes[8]);
    return u;
}

void Module::read_stack(ifstream &ifs)
{
    ifs.seekg(ptable.stack_ptr.word, ios::beg);
    int temp;
    while ((temp = ifs.tellg()) < ptable.array_ptr.word) {
        memory_unit u = readMemUnit(ifs);
        if(u.bytes[8] == (byte)PODTypes::POINTER)
            u.uword = u.uword + rt->GetArrayOffset();
        else if(u.bytes[8] == (byte)PODTypes::STRING)
            u.uword = u.uword + rt->GetStringOffset();
        prog_stack.push_back(u);
        ++this->stackOffset;
    }
    if(prog_stack.size() != 0)
        vsp.uword = prog_stack.size()-1;
    vsp.bytes[8] = (byte)PODTypes::WORD;
}

void Module::read_arrays(ifstream &ifs) {
    ifs.seekg(ptable.array_ptr.word, ios::beg);
    int temp;
    while ((temp = ifs.tellg()) < ptable.com_ptr.word) {
        Array a;
        char type;
        ifs.get(type);
        //cout << "ARRAY type: " << (int)type << endl;
        a.type = (PODTypes)type;
        switch (a.type) {
        case PODTypes::BOOL:
        case PODTypes::BYTE:
            a.factor = 1;
            break;
        case PODTypes::HWORD:
            a.factor = sizeof(short); //2
            break;
        case PODTypes::WORD:
        case PODTypes::STRING:
            a.factor = sizeof(int); //4
            break;
        case PODTypes::DWORD:
        case PODTypes::W64:
            a.factor = sizeof(__int64); //8
        default:
            break;
        }
        uint size = readInt(ifs);
        a.items.assign(size, 0);
        uint s = readInt(ifs);
        if(a.type != PODTypes::STRING)
            for (uint i = 0; i < s; i++) {
                char b;
                ifs.get(b);
                a.items[i] = b;
            }
        else
            for (uint i = 0; i < s/sizeof(uint); i++) {
                memory_unit idx;
                idx.uword = readInt(ifs);
                idx.uword += rt->GetStringOffset();
                uint addr = i*sizeof(uint);
                a.items[ addr ] = idx.bytes[0];
                a.items[addr+1] = idx.bytes[1];
                a.items[addr+2] = idx.bytes[2];
                a.items[addr+3] = idx.bytes[3];
            }
        //cout << (int)a.type << " " << a.items.size()/a.factor << endl;
        //cout << "ADDING ARRAY TO MAP WITH KEY " << array_map.size() << endl;
        array_map[array_map.size()] = a;
        //array_map.insert(pointed_array(array_map.size(),a));
        //array_stack.push_back(a);
        ++this->arrayOffset;
    }
}

void Module::read_bytecode(ifstream &ifs)
{
    ifs.seekg(ptable.com_ptr.word, ios::beg);
    //Runtime::Log(Runtime::LOG_DEBUG, "");
    int temp;
    //cerr  << ptable.com_ptr.word << "\n";
    //cout << "After";
    while ((temp = ifs.tellg()) < ptable.fun_ptr.word) {
        char ch;
        //cout << hex << (int)(unsigned char)ch << " ";
        ifs.get(ch);
        commands.push_back((unsigned char)ch);
        if(real_addr_table.size() != 0 && commands.size()-4 == real_addr_table[rdp]) {
            memory_unit u;
            int ptr = commands.size()-1;
            u.bytes[3] = commands[ptr];
            u.bytes[2] = commands[ptr-1];
            u.bytes[1] = commands[ptr-2];
            u.bytes[0] = commands[ptr-3];
            u.word = u.word + rt->GetOffset();
            commands[ptr-3] = u.bytes[0];
            commands[ptr-2] = u.bytes[1];
            commands[ptr-1] = u.bytes[2];
            commands[ptr] = u.bytes[3];
            ++rdp;
        }
        else if(real_string_table.size() != 0 && commands.size()-4 == real_string_table[rsp]) {
            memory_unit u;
            uint ptr = commands.size()-1;
            u.bytes[3] = commands[ptr];
            u.bytes[2] = commands[ptr-1];
            u.bytes[1] = commands[ptr-2];
            u.bytes[0] = commands[ptr-3];
            //cout << "before move: " << u.uword << endl;
            u.uword = u.uword + rt->GetStringOffset();
            //cout << "after move: " << u.uword << endl;
            //cout << "global offset: " << rt->GetStringOffset() << endl;
            commands[ptr-3] = u.bytes[0];
            commands[ptr-2] = u.bytes[1];
            commands[ptr-1] = u.bytes[2];
            commands[ptr] = u.bytes[3];
            ++rsp;
        }
    }
    //cout << dec << endl;
}

void Module::read_functions(ifstream &ifs) {
    uint temp;
    ifs.seekg(ptable.fun_ptr.word, ios::beg);
    while ((temp = ifs.tellg()) < ptable.str_ptr.word) {
        memory_unit addr;
        for(uint b = 0; b < 4; ++b)
            addr.bytes[b] = ifs.get();
        string ret_str, sign;
        PODTypes ret;
        char ch;
        while((ch = ifs.get()) != ' ')
            ret_str += ch;
        // Why 'switch'? Because switch is faster then 'if' :)
        switch (ret_str[0]) {
        case 'v':
            ret = PODTypes::_NULL;
            break;
        case 'b':
            switch (ret_str[1]) {
            case 'y':
                ret = PODTypes::BYTE;
                break;
            default:
                ret = PODTypes::BOOL;
                break;
            }
            break;
        case 'i':
            switch (ret_str.size()) {
            case 3:
                ret = PODTypes::WORD;
                break;
            default:
                ret = PODTypes::W64;
                break;
            }
            break;
        case 's':
            switch (ret_str[1]) {
            case 't':
                ret = PODTypes::STRING;
                break;
            default:
                ret = PODTypes::HWORD;
                break;
            }
            break;
        case 'd':
            ret = PODTypes::DWORD;
            break;
        default:
            break;
        }
        while((ch = ifs.get()) != 0)
            sign += ch;
        //cout << "Funcltion: " << ret_str << " " << sign << " addr: " << addr.uword << endl;
        auto f = Function(ret, sign, addr.uword);
        funcs.push_back(f);
    }
}

void Module::read_strpool(ifstream &ifs)
{
    //ifs.seekg(ptable.str_ptr.word, ios::beg);
    //int temp;
    //while ((temp = ifs.tellg()) < ptable.str_ptr.word)
    //    str_tab.push_back(readInt(ifs));
    ifs.seekg(ptable.str_ptr.word, ios::beg);
    //int len = str_tab[0];
    //cout << "ptr: " << dec << ptable.str_ptr.word << endl;
    //int entry = 0;
    while (!ifs.eof())
    {
        string str = "";
        //cout << "len " << len << endl;
        for (;;)
        {
            char ch = 0;
            ifs.get(ch);
            if(ch == 0) break;
            str += ch;
            //cout << ch;
        }
        //cout << "STRING: " << str << endl;
        str_stack.push_back(str);

        ++this->stringOffset;
        //entry++;
        //if (entry == str_tab.size()) break;
        //cout << endl;
        //len = str_tab[entry];
    }
}

void Module::read_imports(ifstream &ifs)
{
    //ifs.seekg(ptable.str_ptr.word, ios::beg);
    //int temp;
    //while ((temp = ifs.tellg()) < ptable.str_ptr.word)
    //    str_tab.push_back(readInt(ifs));
    //ifs.seekg(ptable.str_ptr.word, ios::beg);
    //int len = str_tab[0];
    //cout << "size: " << dec << str_tab.size() << endl;
   // int entry = 0;
    rt->log << "LOG: завантаження імпортованих модулів" << endl;
    int temp;
    while ((temp = ifs.tellg()) < ptable.real_addr_ptr.word)
    {
        string str = "";
        //cout << "len " << len << endl;
        for (;;)
        {
            char ch;
            ifs.get(ch);
            if(ch == 0)
                break;
            str += ch;
            //cout << ch;
        }
        rt->log << "LOG: імпорт модуля '" << str.c_str() << "' у модуль " << this->name << endl;
        rt->LoadModule(this, str);
        //entry++;
        //if (entry == str_tab.size()) break;
        //cout << endl;
        //len = str_tab[entry];
    }
}

void Module::Execute(const string &func, memory_unit args[])
{

}

void Module::Call(uint addr) {
    this->Execute(this->funcs[addr].addr);
}

void Module::Execute(unsigned int addr)
{
    //vip.uword = 0;
    //cout << "Funcs size = " << this->name << endl;
    vip.uword = funcs[addr].GetAddr();
    Execute();
}

void Module::Execute() {
    unsigned int size = commands.size();
    while (vip.uword < size) {
        auto opcode = (OpCodes)commands[vip.uword];
        if(flags.TF) rt->Trace(this);
        switch (opcode)
        {
        case OpCodes::_NULL_:
            ++vip.uword;
            break;
        case OpCodes::ADD:
            exec_add();
            break;
        case OpCodes::ALLOC:
            exec_alloc();
            break;
        case OpCodes::AND:
            exec_and();
            break;
        //case OpCodes::BT:
        //	{
        //	}
        //	break;
        //case OpCodes::BTC:
        //	break;
        //case OpCodes::BTS:
        //	break;
        case OpCodes::CALL:
            exec_call();
            break;
        case OpCodes::CALLE:
            exec_calle();
            break;
        case OpCodes::CALLNE:
            exec_callne();
            break;
        case OpCodes::CMP:
            exec_cmp();
            break;
        case OpCodes::DEC:
            exec_dec();
            break;
        case OpCodes::DIV:
            exec_div();
            break;
        case OpCodes::FREE:
            exec_free();
            break;
        case OpCodes::INC:
            exec_inc();
            break;
        case OpCodes::JMP:
            exec_jmp();
            break;
        case OpCodes::JE:
            exec_je();
            break;
        case OpCodes::JNE:
            exec_jne();
            break;
        case OpCodes::JG:
            exec_jg();
            break;
        case OpCodes::JL:
            exec_jl();
            break;
        case OpCodes::JGE:
            exec_jge();
            break;
        case OpCodes::JLE:
            exec_jle();
            break;
        case OpCodes::JC:
            exec_jc();
            break;
        case OpCodes::JNC:
            exec_jnc();
            break;
        case OpCodes::JI:
            exec_ji();
            break;
        case OpCodes::JO:
            exec_jo();
            break;
        case OpCodes::JNO:
            exec_jno();
            break;
        case OpCodes::JS:
            exec_js();
            break;
        case OpCodes::JNS:
            exec_jns();
            break;
        case OpCodes::JP:
            exec_jp();
            break;
        case OpCodes::JNP:
            exec_jnp();
            break;
        case OpCodes::MOV:
            exec_mov();
            break;
        case OpCodes::MUL:
            exec_mul();
            break;
        case OpCodes::NEG:
            exec_neg();
            break;
        case OpCodes::NOP:
        #ifdef _WIN32
            __asm nop;
        #else
            //asm volatile ("nop");
            ++vip.uword;
        #endif
            break;
        case OpCodes::NOT:
            exec_not();
            break;
        case OpCodes::OR:
            exec_or();
            break;
        case OpCodes::POP:
            exec_pop();
            break;
        case OpCodes::PUSH:
            exec_push();
            break;
        case OpCodes::ROL:
        #ifdef __ARM
            cerr << "ROL command temporary unsupported on ARM architecture" << endl;
            cerr << "terminate application!" << endl;
            exec_exit();
            return;
        #else
            exec_rol();
        #endif
            break;
        case OpCodes::ROR:
        #ifdef __ARM
            cerr << "ROR command temporary unsupported on ARM architecture" << endl;
            cerr << "terminate application!" << endl;
            exec_exit();
            return;
        #else
            exec_ror();
        #endif
            break;
        case OpCodes::SET:
            break;
        case OpCodes::SHL:
        #ifdef __ARM
            cerr << "SHL command temporary unsupported on ARM architecture" << endl;
            cerr << "terminate application!" << endl;
            exec_exit();
            return;
        #else
            exec_shl();
        #endif
            break;
        case OpCodes::SHR:
        #ifdef __ARM
            cerr << "SHR command temporary unsupported on ARM architecture" << endl;
            cerr << "terminate application!" << endl;
            exec_exit();
            return;
        #else
            exec_shr();
        #endif
            break;
        case OpCodes::SUB:
            exec_sub();
            break;
        case OpCodes::XCH:
            break;
        case OpCodes::XOR:
            exec_xor();
            break;
        //case OpCodes::DEF:
        //	break;
        case OpCodes::RET:
            if(call_stack.size() == 0 && rt->GetMainModule() == this) exec_exit();
            else if(call_stack.size() != 0) {
                vip.uword = call_stack.top();
                call_stack.pop();
            }
            else if(call_stack.size() == 0 && rt->GetMainModule() != this) return;
            //exec_ret();
            break;
        //case OpCodes::END:
        //	break;
        case OpCodes::HALT:
            exec_exit();
            break;
        case OpCodes::STDOUT:
            exec_stdout();
            break;
        case OpCodes::STDIN:
            exec_stdin();
            break;
        //case OpCodes::SCAN:
        //	break;
        //case OpCodes::EXC:
        //	break;
        default:
            cout << "\e[31m\e[1m" << "Невідома команда: 0x" <<  hex << (int)commands[vip.uword]  << " (" << dec << (int)commands[vip.uword] << ")" << endl;

            rt->PrintVMInfo(this);
            rt->PrintRegisters();
            rt->PrintStackTrace();
            //cin.get();
            cout /*<< dec << "\t " << vip.uword*/ << "\033[0m" << endl;
            cout << "\e[31m\e[1mПродовжити виконання (не рекомендується) (так/ні) ";
            string cont;
            cin >> cont;
            if(cont != "yes" && cont != "так") {

                cout << "\e[34m\e[1mВиконання перервано\n\033[0m";
                return;
            }
            ++vip.uword;
        }
        if(vsp.uword != prog_stack.size()-1) {
            if(vsp.uword != 0 && prog_stack.size() != 0) {
                memory_unit u;
                u.reg = 0;
                prog_stack.resize(vsp.uword, u);
                vsp.uword = prog_stack.size()-1;
            }
        }
        //cout << "DEBUG: vip: " << vip.uword << endl;
    }
}

bool Module::IsExecutable()
{

}

memory_unit Module::getArg (PODTypes &t1) {
    bool is_pointer = false, _sizeof = false, _addr = false;
    PODTypes cast = PODTypes::_NULL;
    memory_unit first;
    uint addr;
    first.reg = 0;
    ++vip.uword;
    unsigned char modifier = commands[vip.uword];
    OpCodes op = (OpCodes)modifier;
    if(op == OpCodes::CAST) {
        cast = (PODTypes)commands[++vip.uword];
        ++vip.uword;
        modifier = commands[vip.uword];
    }
    if(op == OpCodes::SIZEOF) {
        modifier = commands[++vip.uword];
        if(modifier == (byte)OpCodes::VSP) {
            first.uword = prog_stack.size();
            first.bytes[8] = (byte)(t1 = PODTypes::WORD);
            modifier = 0xFF;
        }
        else
            _sizeof = true;
    }
    else if(op == OpCodes::ADDR) {
        _addr = true;
        modifier = commands[++vip.uword];
    }
    if(modifier == (byte)PODTypes::POINTER) {
        is_pointer = true;
        modifier = commands[++vip.uword];
    }
    if(modifier != 0xFF)
        t1 = (PODTypes)modifier;
    switch ((PODTypes)modifier) {
    case PODTypes::BOOL:
        first.bytes[0] = commands[++vip.uword];
        break;
    case PODTypes::BYTE:
        first.bytes[0] = commands[++vip.uword];
        break;
    case PODTypes::HWORD:
        first.bytes[0] = commands[++vip.uword];
        first.bytes[1] = commands[++vip.uword];
        break;
    case PODTypes::WORD:
        first.bytes[0] = commands[++vip.uword];
        first.bytes[1] = commands[++vip.uword];
        first.bytes[2] = commands[++vip.uword];
        first.bytes[3] = commands[++vip.uword];
        break;
    case PODTypes::STRING:
        first.bytes[0] = commands[++vip.uword];
        first.bytes[1] = commands[++vip.uword];
        first.bytes[2] = commands[++vip.uword];
        first.bytes[3] = commands[++vip.uword];
        break;
    case PODTypes::DWORD:
        first.bytes[0] = commands[++vip.uword];
        first.bytes[1] = commands[++vip.uword];
        first.bytes[2] = commands[++vip.uword];
        first.bytes[3] = commands[++vip.uword];
        first.bytes[4] = commands[++vip.uword];
        first.bytes[5] = commands[++vip.uword];
        first.bytes[6] = commands[++vip.uword];
        first.bytes[7] = commands[++vip.uword];
        break;
    default:    
        if(is_register(modifier)) {
            unsigned char offset = (unsigned char)OpCodes::VR1;
            if(is_pointer) {
                //cout << "ARRAYS SIZE: " << array_stack.size() << " IDX: " << tmp.uword << endl;
                //auto array = array_map
                auto array = array_map[regs[commands[vip.uword] - offset].uword];
                PODTypes t;
                //--vip.uword;
                auto idx = getArg(t);
                for(uint i = 0; i < array.factor; ++i) {
                    first.bytes[i] = array.items[idx.uword * array.factor + i];
                }
                first.bytes[8] = (byte)array.type;
                t1 = array.type;
            }
            else {
                first = regs[commands[vip.uword] - offset];
                if(commands[++vip.uword] == (byte)OpCodes::EXTEND) {
                    OpCodes op = (OpCodes)commands[++vip.uword];
                    byte t = commands[++vip.uword];
                    memory_unit a2;
                    if(t == (byte)OpCodes::DIGIT) {
                        a2.bytes[0] = commands[++vip.uword];
                        a2.bytes[1] = commands[++vip.uword];
                        a2.bytes[2] = commands[++vip.uword];
                        a2.bytes[3] = commands[++vip.uword];
                    }
                    else if(is_register(t)) {
                        a2 = regs[t - (byte)OpCodes::VR1];
                    }
                    switch (op) {
                    case OpCodes::ADD:
                        first.uword += a2.uword;
                        break;
                    case OpCodes::SUB:
                        first.uword -= a2.uword;
                        break;
                    case OpCodes::MUL:
                        first.uword *= a2.uword;
                        break;
                    case OpCodes::DIV:
                        first.uword /= a2.uword;
                        break;
                    default:
                        rt->ThrowException("Unknow extension");
                        break;
                    }
                }
                else --vip.uword;
            }
            t1 = (PODTypes)first.bytes[8];
        }
        else if(modifier == (unsigned char)OpCodes::SQBRCK) {
            modifier = commands[++vip.uword];
            memory_unit tmp;
            if(cast == PODTypes::_NULL)
                t1 = PODTypes::W64;
            else t1 = cast;
            tmp.reg = 0;
            if(modifier == (unsigned char)PODTypes::BYTE) {
                tmp.bytes[0] = commands[++vip.uword];
            }
            else if(modifier == (unsigned char)PODTypes::HWORD) {
                tmp.bytes[0] = commands[++vip.uword];
                tmp.bytes[1] = commands[++vip.uword];
            }
            else if(modifier == (unsigned char)PODTypes::WORD) {
                tmp.bytes[0] = commands[++vip.uword];
                tmp.bytes[1] = commands[++vip.uword];
                tmp.bytes[2] = commands[++vip.uword];
                tmp.bytes[3] = commands[++vip.uword];
            }
            else if(is_register(modifier)) {
                unsigned char offset = (unsigned char)OpCodes::VR1;
                tmp.word = regs[commands[vip.uword] - offset].word;
            }
            if(commands[++vip.uword] == (byte)OpCodes::EXTEND) {
                OpCodes op = (OpCodes)commands[++vip.uword];
                byte t = commands[++vip.uword];
                memory_unit a2;
                if(t == (byte)OpCodes::DIGIT) {
                    a2.bytes[0] = commands[++vip.uword];
                    a2.bytes[1] = commands[++vip.uword];
                    a2.bytes[2] = commands[++vip.uword];
                    a2.bytes[3] = commands[++vip.uword];
                }
                else if(is_register(t)) {
                    a2 = regs[t - (byte)OpCodes::VR1];
                }
                switch (op) {
                case OpCodes::ADD:
                    tmp.uword += a2.uword;
                    break;
                case OpCodes::SUB:
                    tmp.uword -= a2.uword;
                    break;
                case OpCodes::MUL:
                    tmp.uword *= a2.uword;
                    break;
                case OpCodes::DIV:
                    tmp.uword /= a2.uword;
                    break;
                default:
                    rt->ThrowException("Unknow extension");
                    break;
                }
            }
            else --vip.uword;
            if(is_pointer) {
                //cout << "ARRAYS SIZE: " << array_map.size() << " IDX: " << tmp.uword << endl;
                if((PODTypes)prog_stack[tmp.word].bytes[8]  == PODTypes::STRING) {
                    auto str = str_stack[prog_stack[tmp.word].uword];
                    PODTypes t;
                    auto idx = getArg(t);
                    string target = "";
                    target += str[idx.uword];
                    t1 = PODTypes::STRING;
                    first.uword = str_stack.size();
                    str_stack.push_back(target);
                }
                else {
                    auto array = array_map[prog_stack[tmp.word].uword];
                    //cout << "AMS: " << (int)array.type << endl;
                    PODTypes t;
                    //--vip.uword;
                    auto idx = getArg(t);
                    for(uint i = 0; i < array.factor; ++i) {
                        first.bytes[i] = array.items[idx.uword * array.factor + i];
                    }
                    first.bytes[8] = (byte)array.type;
                    t1 = array.type;
                }
            }
            else if (_addr) {
                first.uword = tmp.uword;
                t1 = PODTypes::WORD;
            }
            else {
                first = prog_stack[tmp.word];
                t1 = (PODTypes)first.bytes[8];
            }
        }   
        break;
    }
    /*else if (modifier == (byte) PODTypes::POINTER) {
        first.bytes[0] = commands[++vip.uword];
        first.bytes[1] = commands[++vip.uword];
        first.bytes[2] = commands[++vip.uword];
        first.bytes[3] = commands[++vip.uword];

    }*/
    if(_sizeof) {
        //rt->PrintStackTrace();
        uint size;
        switch (t1) {
        case PODTypes::BOOL:
        case PODTypes::BYTE:
            size = 1;
            break;
        case PODTypes::HWORD:
            size = 2;
            break;
        case PODTypes::WORD:
            size = 4;
            break;
        case PODTypes::STRING:
            size = str_stack[first.uword].size();
            break;
        case PODTypes::W64:
        case PODTypes::DWORD:
            size = 8;
            break;
        case PODTypes::POINTER:
        {
            if(array_map.count(first.uword) <= 0) {
                rt->ThrowException("Can't find element in array_map");
            }
            auto a = array_map[first.uword];
            size = a.items.size()/a.factor;
        }
            break;
        default:
            break;
        }
        first.uword = size;
        t1 = PODTypes::WORD;
    }
    if(cast != PODTypes::_NULL) {
        if(t1 < PODTypes::DWORD && cast == PODTypes::DWORD)
            if(t1 != PODTypes::BOOL) first.dword = (double)first.reg;
            else first.dword = 1;
        else if(t1 < PODTypes::DWORD && cast == PODTypes::BOOL)
            first.b = (bool)first.reg;
        else if(t1 == PODTypes::DWORD && cast == PODTypes::BOOL)
            first.b = (bool)first.dword;
        else if(t1 == PODTypes::DWORD && cast < PODTypes::DWORD)
            first.reg = (__int64)first.dword;
        t1 = cast;
    }
    //++vip.uword;
    first.bytes[8] = (unsigned char)t1;
    return first;
}

memory_unit* Module::getPointer (PODTypes &t1) {
    bool is_pointer = false;
    PODTypes cast = PODTypes::_NULL;
    memory_unit *first;
    //first.reg = 0;
    ++vip.uword;
    unsigned char modifier = commands[vip.uword];
    if(modifier == (byte)PODTypes::POINTER) {
        is_pointer = true;
        modifier = commands[++vip.uword];
    }
    if(modifier == (unsigned char)OpCodes::CAST) {
        cast = (PODTypes)commands[++vip.uword];
        ++vip.uword;
        modifier = commands[vip.uword];
    }
    if(is_register(modifier)) {
        //t1 = PODTypes::W64;
        unsigned char offset = (unsigned char)OpCodes::VR1;
            //cout << "Offet: " << commands[vip.uword] - offset << endl;
        first = &regs[commands[vip.uword] - offset];
        t1 = (PODTypes)first->bytes[8];
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
    */
    else if(modifier == (unsigned char)OpCodes::SQBRCK) {
        modifier = commands[++vip.uword];
        memory_unit tmp;
        if(cast == PODTypes::_NULL)
            t1 = PODTypes::W64;
        else t1 = cast;
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
        */
        else if(is_register(modifier)) {
            unsigned char offset = (unsigned char)OpCodes::VR1;
            tmp.word = regs[commands[vip.uword] - offset].word;
        }
        if(commands[++vip.uword] == (byte)OpCodes::EXTEND) {
            OpCodes op = (OpCodes)commands[++vip.uword];
            byte t = commands[++vip.uword];
            memory_unit a2;
            if(t == (byte)PODTypes::WORD) {
                a2.bytes[0] = commands[++vip.uword];
                a2.bytes[1] = commands[++vip.uword];
                a2.bytes[2] = commands[++vip.uword];
                a2.bytes[3] = commands[++vip.uword];
            }
            else if(is_register(t)) {
                a2 = regs[t - (byte)OpCodes::VR1];
            }
            switch (op) {
            case OpCodes::ADD:
                tmp.uword += a2.uword;
                break;
            case OpCodes::SUB:
                tmp.uword -= a2.uword;
                break;
            case OpCodes::MUL:
                tmp.uword *= a2.uword;
                break;
            case OpCodes::DIV:
                tmp.uword /= a2.uword;
                break;
            default:
                rt->ThrowException("Unknow extension");
                break;
            }
        }
        else --vip.uword;
        if(is_pointer) {
            rt->ThrowException("Can't create array element pointer!", RuntimeException::CriticalFault);
            /*
            cout << "ARRAYS SIZE: " << array_stack.size() << " IDX: " << tmp.uword << endl;
            auto array = array_stack[tmp.uword];
            PODTypes t;
            //--vip.uword;
            auto idx = getArg(t);
            for(uint i = 0; i < array.factor; ++i) {
                first.bytes[i] = array.items[idx.uword * array.factor + i];
            }
            first.bytes[8] = (byte)array.type;
            //Array::size
            */
        }
        else
            first = &prog_stack[tmp.word];
        t1 = (PODTypes)first->bytes[8];
    }
    if(cast != PODTypes::_NULL) {
        if(t1 < PODTypes::DWORD && cast == PODTypes::DWORD)
            if(t1 != PODTypes::BOOL) first->dword = (double)first->reg;
            else first->dword = 1;
        else if(t1 < PODTypes::DWORD && cast == PODTypes::BOOL)
            first->b = (bool)first->reg;
        else if(t1 == PODTypes::DWORD && cast == PODTypes::BOOL)
            first->b = (bool)first->dword;
        else if(t1 == PODTypes::DWORD && cast < PODTypes::DWORD)
            first->reg = (__int64)first->dword;
        t1 = cast;
    }
    //++vip.uword;
    //first->bytes[8] = (unsigned char)t1;
    return first;
}

Module::array_ptr Module::getArrayPointer(PODTypes &t1) {
    //PODTypes t1;
    //memory_unit arg = getArg(t1);
    array_ptr ptr;
    PODTypes cast = PODTypes::_NULL;
    //memory_unit first;
    ptr.val.reg = 0;
    ++vip.uword;
    unsigned char modifier = commands[vip.uword];
    if(modifier != (byte)PODTypes::POINTER) {
        rt->ThrowException("Аргумент не є масивом!");
    }
    modifier = commands[++vip.uword];
    if(modifier == (unsigned char)OpCodes::CAST) {
        cast = (PODTypes)commands[++vip.uword];
        ++vip.uword;
        modifier = commands[vip.uword];
    }

    if(is_register(modifier)) {
        unsigned char offset = (unsigned char)OpCodes::VR1;
        //cout << "ARRAYS SIZE: " << array_stack.size() << " IDX: " << tmp.uword << endl;
        ptr.aid = regs[commands[vip.uword] - offset].uword;
        auto array = array_map[ptr.aid];
        //ptr.fctr = array.factor;
        PODTypes t;
        //--vip.uword;
        ptr.pos = getArg(t).uword;

        for(uint i = 0; i < array.factor; ++i) {
            ptr.val.bytes[i] = array.items[ptr.pos * array.factor + i];
        }
        ptr.val.bytes[8] = (byte)array.type;
        t1 = (PODTypes)ptr.val.bytes[8];
    }
    else if(modifier == (unsigned char)OpCodes::SQBRCK) {
        modifier = commands[++vip.uword];
        memory_unit tmp;
        if(cast == PODTypes::_NULL)
            t1 = PODTypes::W64;
        else t1 = cast;
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
        else if(is_register(modifier)) {
            unsigned char offset = (unsigned char)OpCodes::VR1;
            tmp.word = regs[commands[vip.uword] - offset].word;
        }
        if(commands[++vip.uword] == (byte)OpCodes::EXTEND) {
            OpCodes op = (OpCodes)commands[++vip.uword];
            byte t = commands[++vip.uword];
            memory_unit a2;
            if(t == (byte)PODTypes::WORD) {
                a2.bytes[0] = commands[++vip.uword];
                a2.bytes[1] = commands[++vip.uword];
                a2.bytes[2] = commands[++vip.uword];
                a2.bytes[3] = commands[++vip.uword];
            }
            else if(is_register(t)) {
                a2 = regs[t - (byte)OpCodes::VR1];
            }
            switch (op) {
            case OpCodes::ADD:
                tmp.uword += a2.uword;
                break;
            case OpCodes::SUB:
                tmp.uword -= a2.uword;
                break;
            case OpCodes::MUL:
                tmp.uword *= a2.uword;
                break;
            case OpCodes::DIV:
                tmp.uword /= a2.uword;
                break;
            default:
                rt->ThrowException("Unknow extension");
                break;
            }
        }
        else --vip.uword;
        //cout << "ARRAYS SIZE: " << array_stack.size() << " IDX: " << tmp.uword << endl;
        ptr.aid = tmp.uword;
        auto array = array_map[prog_stack[ptr.aid].uword];
        //ptr.fctr = array.factor;
        PODTypes t;
        //--vip.uword;
        ptr.pos = getArg(t).uword;

        for(uint i = 0; i < array.factor; ++i) {
            ptr.val.bytes[i] = array.items[ptr.pos * array.factor + i];
        }
        ptr.val.bytes[8] = (byte)array.type;
        t1 = (PODTypes)ptr.val.bytes[8];
    }
    if(cast != PODTypes::_NULL) {
        if(t1 < PODTypes::DWORD && cast == PODTypes::DWORD)
            if(t1 != PODTypes::BOOL) ptr.val.dword = (double)ptr.val.reg;
            else ptr.val.dword = 1;
        else if(t1 < PODTypes::DWORD && cast == PODTypes::BOOL)
            ptr.val.b = (bool)ptr.val.reg;
        else if(t1 == PODTypes::DWORD && cast == PODTypes::BOOL)
            ptr.val.b = (bool)ptr.val.dword;
        else if(t1 == PODTypes::DWORD && cast < PODTypes::DWORD)
            ptr.val.reg = (__int64)ptr.val.dword;
        t1 = cast;
    }
    //++vip.uword;
    //first->bytes[8] = (unsigned char)t1;
    return ptr;
}

void Module::applyArrayPointer(array_ptr ptr) {
    Array* a = &array_map[ptr.aid];

    uint idx = ptr.pos*a->factor;
    for(uint i = 0; i < a->factor; ++i) {
        a->items[idx + i] = ptr.val.bytes[i];
    }
}

/*
void memUnitReader(ifstream &ifs, uint end, vector<memory_unit>& v) {
    int pos;
    while ((pos = ifs.tellg()) != end) {
        memory_unit u;
        ifs.get((char &)u.bytes[0]);
        ifs.get((char &)u.bytes[1]);
        ifs.get((char &)u.bytes[2]);
        ifs.get((char &)u.bytes[3]);
        ifs.get((char &)u.bytes[4]);
        ifs.get((char &)u.bytes[5]);
        ifs.get((char &)u.bytes[6]);
        ifs.get((char &)u.bytes[7]);
        ifs.get((char &)u.bytes[8]);
        v.push_back(u);
    }
}
/*
void readStack(ifstream &ifs, uint end, Module &m)
{
    int temp;
    while ((temp = ifs.tellg()) < end) {
        memory_unit u = readMemUnit(ifs);
        if(u.bytes[8] == (byte)PODTypes::POINTER)
            u.uword = u.uword + m.rt->GetArrayOffset();
        else if(u.bytes[8] == (byte)PODTypes::STRING)
            u.uword = u.uword + m.rt->GetStringOffset();
        v.push_back(u);
        ++m.stackOffset;
    }
    vsp.uword = prog_stack.size()-1;
    vsp.bytes[8] = (byte)PODTypes::WORD;
}

void readArrays(ifstream &ifs, uint end, Module &m)
{
    //ifs.seekg(ptable.array_ptr.word, ios::beg);
    int temp;
    while ((temp = ifs.tellg()) < end) {
        Array a;
        char type;
        ifs.get(type);
        //cout << "ARRAY type: " << (int)type << endl;
        a.type = (PODTypes)type;
        switch (a.type) {
        case PODTypes::BOOL:
        case PODTypes::BYTE:
            a.factor = 1;
            break;
        case PODTypes::HWORD:
            a.factor = sizeof(short); //2
            break;
        case PODTypes::WORD:
        case PODTypes::STRING:
            a.factor = sizeof(int); //4
            break;
        case PODTypes::DWORD:
        case PODTypes::W64:
            a.factor = sizeof(__int64); //8
        default:
            break;
        }
        uint size = readInt(ifs);
        a.items.assign(size, 0);
        uint s = readInt(ifs);
        if(a.type != PODTypes::STRING)
            for (uint i = 0; i < s; i++) {
                char b;
                ifs.get(b);
                a.items[i] = b;
            }
        else
            for (uint i = 0; i < s/sizeof(uint); i++) {
                memory_unit idx;
                idx.uword = readInt(ifs);
                idx.uword += m.rt->GetStringOffset();
                uint addr = i*sizeof(uint);
                a.items[ addr ] = idx.bytes[0];
                a.items[addr+1] = idx.bytes[1];
                a.items[addr+2] = idx.bytes[2];
                a.items[addr+3] = idx.bytes[3];
            }
        //cout << (int)a.type << " " << a.items.size()/a.factor << endl;
        //cout << "ADDING ARRAY TO MAP WITH KEY " << array_map.size() << endl;
        array_map[array_map.size()] = a;
        //array_map.insert(pointed_array(array_map.size(),a));
        //array_stack.push_back(a);
        ++m.arrayOffset;
    }
}

void readBytecode(ifstream &ifs, uint end, Module &m)
{
    //ifs.seekg(ptable.com_ptr.word, ios::beg);
    //Runtime::Log(Runtime::LOG_DEBUG, "");
    int temp;
    //cerr  << ptable.com_ptr.word << "\n";
    //cout << "After";
    while ((temp = ifs.tellg()) < end) {
        char ch;
        //cout << hex << (int)(unsigned char)ch << " ";
        ifs.get(ch);
        m.commands.push_back((unsigned char)ch);
        if(m.real_addr_table.size() != 0 && m.commands.size()-4 == m.real_addr_table[rdp]) {
            memory_unit u;
            int ptr = m.commands.size()-1;
            u.bytes[3] = m.commands[ptr];
            u.bytes[2] = m.commands[ptr-1];
            u.bytes[1] = m.commands[ptr-2];
            u.bytes[0] = m.commands[ptr-3];
            u.word = u.word + m.rt->GetOffset();
            m.commands[ptr-3] = u.bytes[0];
            m.commands[ptr-2] = u.bytes[1];
            m.commands[ptr-1] = u.bytes[2];
            m.commands[ptr] = u.bytes[3];
            ++rdp;
        }
        else if(m.real_string_table.size() != 0 && m.commands.size()-4 == m.real_string_table[rsp]) {
            memory_unit u;
            uint ptr = m.commands.size()-1;
            u.bytes[3] = m.commands[ptr];
            u.bytes[2] = m.commands[ptr-1];
            u.bytes[1] = m.commands[ptr-2];
            u.bytes[0] = m.commands[ptr-3];
            //cout << "before move: " << u.uword << endl;
            u.uword = u.uword + m.rt->GetStringOffset();
            //cout << "after move: " << u.uword << endl;
            //cout << "global offset: " << rt->GetStringOffset() << endl;
            m.commands[ptr-3] = u.bytes[0];
            m.commands[ptr-2] = u.bytes[1];
            m.commands[ptr-1] = u.bytes[2];
            m.commands[ptr] = u.bytes[3];
            ++rsp;
        }
    }
    //cout << dec << endl;
}

void read_functions(ifstream &ifs) {
    uint temp;
    ifs.seekg(ptable.fun_ptr.word, ios::beg);
    while ((temp = ifs.tellg()) < ptable.str_ptr.word) {
        memory_unit addr;
        for(uint b = 0; b < 4; ++b)
            addr.bytes[b] = ifs.get();
        string ret_str, sign;
        PODTypes ret;
        char ch;
        while((ch = ifs.get()) != ' ')
            ret_str += ch;
        // Why 'switch'? Because switch is faster then 'if' :)
        switch (ret_str[0]) {
        case 'v':
            ret = PODTypes::_NULL;
            break;
        case 'b':
            switch (ret_str[1]) {
            case 'y':
                ret = PODTypes::BYTE;
                break;
            default:
                ret = PODTypes::BOOL;
                break;
            }
            break;
        case 'i':
            switch (ret_str.size()) {
            case 3:
                ret = PODTypes::WORD;
                break;
            default:
                ret = PODTypes::W64;
                break;
            }
            break;
        case 's':
            switch (ret_str[1]) {
            case 't':
                ret = PODTypes::STRING;
                break;
            default:
                ret = PODTypes::HWORD;
                break;
            }
            break;
        case 'd':
            ret = PODTypes::DWORD;
            break;
        default:
            break;
        }
        while((ch = ifs.get()) != 0)
            sign += ch;
        //cout << "Funcltion: " << ret_str << " " << sign << " addr: " << addr.uword << endl;
        auto f = Function(ret, sign, addr.uword);
        funcs.push_back(f);
    }
}

void Module::read_strpool(ifstream &ifs)
{
    //ifs.seekg(ptable.str_ptr.word, ios::beg);
    //int temp;
    //while ((temp = ifs.tellg()) < ptable.str_ptr.word)
    //    str_tab.push_back(readInt(ifs));
    ifs.seekg(ptable.str_ptr.word, ios::beg);
    //int len = str_tab[0];
    //cout << "ptr: " << dec << ptable.str_ptr.word << endl;
    //int entry = 0;
    while (!ifs.eof())
    {
        string str = "";
        //cout << "len " << len << endl;
        for (;;)
        {
            char ch = 0;
            ifs.get(ch);
            if(ch == 0) break;
            str += ch;
            //cout << ch;
        }
        //cout << "STRING: " << str << endl;
        str_stack.push_back(str);

        ++this->stringOffset;
        //entry++;
        //if (entry == str_tab.size()) break;
        //cout << endl;
        //len = str_tab[entry];
    }
}

void Module::read_imports(ifstream &ifs)
{
    //ifs.seekg(ptable.str_ptr.word, ios::beg);
    //int temp;
    //while ((temp = ifs.tellg()) < ptable.str_ptr.word)
    //    str_tab.push_back(readInt(ifs));
    //ifs.seekg(ptable.str_ptr.word, ios::beg);
    //int len = str_tab[0];
    //cout << "size: " << dec << str_tab.size() << endl;
   // int entry = 0;
    rt->log << "LOG: завантаження імпортованих модулів" << endl;
    int temp;
    while ((temp = ifs.tellg()) < ptable.real_addr_ptr.word)
    {
        string str = "";
        //cout << "len " << len << endl;
        for (;;)
        {
            char ch;
            ifs.get(ch);
            if(ch == 0)
                break;
            str += ch;
            //cout << ch;
        }
        rt->log << "LOG: імпорт модуля '" << str.c_str() << "' у модуль " << this->name << endl;
        rt->LoadModule(this, str);
        //entry++;
        //if (entry == str_tab.size()) break;
        //cout << endl;
        //len = str_tab[entry];
    }
}
* /
bool Module::loadBinaryNM(pheader h, istream &reader){
    if(h.first != "code") return false;
    ifs.seekg(h.second.addr, ios::beg);
    //Runtime::Log(Runtime::LOG_DEBUG, "");
    int temp;
    //cerr  << ptable.com_ptr.word << "\n";
    //cout << "After";
    while ((temp = ifs.tellg()) < h.second.next->addr && !ifs.eof()) {
        char ch;
        //cout << hex << (int)(unsigned char)ch << " ";
        ifs.get(ch);
        commands.push_back((unsigned char)ch);
        if(real_addr_table.size() != 0 && commands.size()-4 == real_addr_table[rdp]) {
            memory_unit u;
            int ptr = commands.size()-1;
            u.bytes[3] = commands[ptr];
            u.bytes[2] = commands[ptr-1];
            u.bytes[1] = commands[ptr-2];
            u.bytes[0] = commands[ptr-3];
            u.word = u.word + rt->GetOffset();
            commands[ptr-3] = u.bytes[0];
            commands[ptr-2] = u.bytes[1];
            commands[ptr-1] = u.bytes[2];
            commands[ptr] = u.bytes[3];
            ++rdp;
        }
        else if(real_string_table.size() != 0 && commands.size()-4 == real_string_table[rsp]) {
            memory_unit u;
            uint ptr = commands.size()-1;
            u.bytes[3] = commands[ptr];
            u.bytes[2] = commands[ptr-1];
            u.bytes[1] = commands[ptr-2];
            u.bytes[0] = commands[ptr-3];
            //cout << "before move: " << u.uword << endl;
            u.uword = u.uword + rt->GetStringOffset();
            //cout << "after move: " << u.uword << endl;
            //cout << "global offset: " << rt->GetStringOffset() << endl;
            commands[ptr-3] = u.bytes[0];
            commands[ptr-2] = u.bytes[1];
            commands[ptr-1] = u.bytes[2];
            commands[ptr] = u.bytes[3];
            ++rsp;
        }
    }
    //cout << dec << endl;
}
bool Module::loadMetaNM(pheader h, istream &reader){
    if(h.first != "funcs") return false;
    uint temp;
    ifs.seekg(h.second.addr, ios::beg);
    while ((temp = ifs.tellg()) < h.second.next->addr && !ifs.eof()) {
        memory_unit addr;
        for(uint b = 0; b < 4; ++b)
            addr.bytes[b] = ifs.get();
        string ret_str, sign;
        PODTypes ret;
        char ch;
        while((ch = ifs.get()) != ' ')
            ret_str += ch;
        // Why 'switch'? Because switch is faster then 'if' :)
        switch (ret_str[0]) {
        case 'v':
            ret = PODTypes::_NULL;
            break;
        case 'b':
            switch (ret_str[1]) {
            case 'y':
                ret = PODTypes::BYTE;
                break;
            default:
                ret = PODTypes::BOOL;
                break;
            }
            break;
        case 'i':
            switch (ret_str.size()) {
            case 3:
                ret = PODTypes::WORD;
                break;
            default:
                ret = PODTypes::W64;
                break;
            }
            break;
        case 's':
            switch (ret_str[1]) {
            case 't':
                ret = PODTypes::STRING;
                break;
            default:
                ret = PODTypes::HWORD;
                break;
            }
            break;
        case 'd':
            ret = PODTypes::DWORD;
            break;
        default:
            break;
        }
        while((ch = ifs.get()) != 0)
            sign += ch;
        //cout << "Funcltion: " << ret_str << " " << sign << " addr: " << addr.uword << endl;
        auto f = Function(ret, sign, addr.uword);
        funcs.push_back(f);
    }
}

bool Module::loadMemUnit(pheader h, istream &reader){

}
bool Module::loadStrings(pheader h, istream &reader){
    rt->log << "LOG: завантаження імпортованих модулів" << endl;
    ifs.seekg(h.second.addr, ios::beg);
    int temp;
    while ((temp = ifs.tellg()) < h.second.next->addr && !ifs.eof())
    {
        string str = "";
        for (;;)
        {
            char ch;
            ifs.get(ch);
            if(ch == 0)
                break;
            str += ch;
            //cout << ch;
        }
        if(h.first == "imps") {
            rt->log << "LOG: імпорт модуля '" << str.c_str() << "' у модуль " << this->name << endl;
            rt->LoadModule(this, str);
        }
        else if(h.first == "strs") {
            str_stack.push_back(str);

            ++this->stringOffset;
        }
    }
}
bool Module::loadArrays(pheader h, istream &reader){

}
bool Module::loadTyped(pheader h, istream &reader) {
    return false;
}
*/

