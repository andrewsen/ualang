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
#include <cstdlib>
#include <cstring>
#include <csignal>

using namespace std;

unordered_map<uint, Array> array_map;
vector<memory_unit> prog_stack;
//vector<Array> array_stack;
//map<uint, Array> array_map;
vector<string> str_stack;				//COMPILABLE
memory_unit regs[9];
uint max_idx = 0, min_idx = 0;

time_t start_time1, start_time2, end_time;

string getCurrentDir(string fullName);


bool Runtime::isLogOn = false;

Runtime::Runtime(int argc, const char *argv[])
{
    this->argc = argc;
    this->argv = argv;
}

Runtime *Runtime::Create(int argc, const char* argv[])
{
    start_time1 = time(nullptr);
    struct sigaction sa;
    //memset(&sa, 0, sizeof(sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = [&](int signal, siginfo_t *si, void *arg)
    {
        cout << "Помилка сегментації за адресою " << hex << static_cast<int*>(si->si_addr) << dec << endl;
        exit(EXIT_FAILURE);
    };
    sa.sa_flags   = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);

    Runtime *rt = new Runtime(argc, argv);
    rt->log.SwitchOn(isLogOn);
    rt->log << "Екземпляр Runtime створений" << endl;

    vr1.reg = vr2.reg = vr3.reg = vr4.reg = vr5.reg = vra.reg = 0;
    vr1.bytes[8] = vr2.bytes[8] = vr3.bytes[8] = vr4.bytes[8] = vr5.bytes[8] = vra.bytes[8] = (unsigned char)PODTypes::W64;
    vsp.word = 0;

    prog_stack.clear();
    string work_dir = getenv("PWD");
    work_dir += "/";

    string m = argv[1];
    string p = m.substr(0, m.find_last_of('/'));
    p += "/:";
    auto cur = getCurrentDir(argv[0]);
    if(cur != "") cur += "/modules/:";
    setenv("SVM_PATH", (p + work_dir + "modules/:" + "/usr/share/svm/modules:" + cur).c_str(), 0);


    rt->entry = Module(argv[1], rt, true);
    rt->entry.Load();

    //rt->log << "Program loaded successfully and ready to be executed!" << endl;
    rt->log << "Програма успішно завантажена і готова до виконання" << endl;

    return rt;
}

string getCurrentDir(string fullName) {
    int idx = fullName.find_last_of('/');
    if(idx = string::npos) return "";
    string ret = fullName.erase(idx);
    //cout << (ret + "/") << endl;
    return ret + "/";
}

void Runtime::Start()
{
    start_time2 = time(nullptr);

    if(argc >= 3)
        this->entry.Run(argv[1] + string(" ") + argv[2]);
    else if(argc == 2)
        this->entry.Run(argv[1]);
}

void Runtime::LoadModule(Module *parent, string str) {
    Module imp(str, this);
    //imp.moduleOffset = modules.size();
    imp.Load();
    this->offset += imp.stackOffset;
    this->arrayOffset += imp.arrayOffset;
    this->stringOffset += imp.stringOffset;
    //this->modules.push_back(imp);
    parent->imports.push_back(imp);
}

void Runtime::printMemUnit (memory_unit u) {
    switch ((PODTypes)u.bytes[8]) {
       case PODTypes::BOOL:
           log << "bool:" << u.b;
           break;
       case PODTypes::BYTE:
           log << "byte:" << u.bytes[0];
           break;
       case PODTypes::HWORD:
           log << "short:" << u.hword;
           break;
       case PODTypes::WORD:
           log << "int:" << u.word;
           break;
       case PODTypes::W64:
           log << "int64:" << u.reg;
           break;
       case PODTypes::DWORD:
           log << "double:" << u.dword;
           break;
       case PODTypes::STRING:
           log << "string:" << u.uword;
           break;
       case PODTypes::POINTER:
           log << "pointer/array:" << u.uword;
           break;
       default:
           break;
    }
}

void Runtime::PrintRegisters() {
    log << "Регістри:" << endl;
    for (int i = 0; i < 5; ++i) {
        log << "\tvr" << i+1 << ": TYPE ";
        printMemUnit(regs[i]);
        log << "\n";
    }
    log << "\tvra: " << "TYPE ";
    printMemUnit(vra);
    log << " vsp: " << vsp.uword << " "
        << "vip: " << vip.uword << endl;
}
void Runtime::PrintStackTrace() {
    log << "Стек:" << endl;
    log << "\tрозмір стеку: " << prog_stack.size() << endl;
    int counter = -1;
    for (memory_unit item : prog_stack) {
        log << "\t0x" << hex << ++counter << ": тип ";
        printMemUnit(item);
        log << "\n";
    }
}

void Runtime::PrintStringsTrace() {
    log << "\e[34m\e[1m" ;
    log << "Стек рядків:" << endl;
    log << "\tрозмір стеку рядків: " << str_stack.size() << endl;
    for(int i = 0; i < str_stack.size(); i++) {
        log << "\t0x" << hex << i << ": " << str_stack[i] << endl;
    }
    log << "\e033[0m";
}

void Runtime::PrintVMInfo(Module *m) {
    log << "Інформация про ВМ:" << endl
        << "\tголовний модуль: " << this->entry.name << endl
        << "\tактивний модуль: " << m->name << endl
        << "\tзміщення стеку акт. модуля: " << this->offset << endl
        << "\tзміщення рядків акт. модуля: " << this->arrayOffset << endl
        << "\tзміщення масивів акт. модуля: " << this->stringOffset << endl;
    log << "Байт-код активного модуля:" << endl;
    for (byte b : m->commands) {
        log << hex << (int)b << " ";
    }
    log << "\n";
}

void Runtime::Shutdown() {
    auto cur = time(nullptr);
    log << "Shuting down...\n";
    log << "Time 1: " << difftime(start_time1, cur);
    log << " time 2: " << difftime(start_time1, cur);
    log << "\n";
}

void Runtime::Destroy(Runtime* rt) {
    if(rt != nullptr)
        delete rt;
    cout << "Runtime destroyed!";//\n\e033[0m";
}

vector<string> split(string p, char ch, bool rmempty) {
    if(!rmempty) {
        /*const char *tmp = p.c_str();
        int entries = 1;
        for (; *tmp != '\0'; tmp++) {
            if(*tmp == ch) entries++;
        }
        vector<string> strings;
        //string* strings = new string [entries+1];
        int ptr = 0;
        //ss[0] = "";
        strings[0] = "";
        for (int i = 0; i < p.length(); i++) {
            if (p[i] == ch) {
                ptr++;
                //ss[ptr] = "";
                strings[ptr] = "";
            }
            string stdstr(strings[ptr]);
            stdstr += p[i];
            strings[ptr] = trim(stdstr);
            //strings[ptr] += "\0";
        }
        strings[entries] = "";
        return strings;*/

        vector<string> strings;
        //string* strings = new string [entries + 1];
        //int ptr = 0;
        //strings.push_back("");
        string t = "";
        for (int i = 0; i < p.length(); i++) {
            if (p[i] == ch) {
                //ptr++;
                ++i;
                strings.push_back(t);
                t = "";
            }
            //string stdstr(strings[ptr]);
            t += p[i];
        }
        return strings;
    }
    else {
        vector<string> strings;
        string t = "";
        for (int i = 0; i < p.length(); i++) {
            if (p[i] == ch) {
                if(t != "") {
                    //ptr++;
                    ++i;
                    strings.push_back(t);
                    t = "";
                }
            }
            //string stdstr(strings[ptr]);
            t += p[i];
            //strings[ptr] += p[i];
            //strings[ptr] = trim(stdstr);
            //strings[ptr] += "\0";
        }
        return strings;
    }
}


bool endsWith(string s, const string &str) {
    return (s, str, false);
}

bool endsWith(string s, const string &str, bool ignoreCase) {
    if(!ignoreCase) {
        if(s.find_last_of(str) == s.length() - str.length()) return true;
    }
    else {
        //if(p.lastIndexOf(str.ToLower()) == s.length() - str.length()) return true;
        if(s.find_last_of(str) == s.length() - str.length()) return true;
    }
    return false;
}

string trim(string str) {
    int start = 0, end = str.length();
    for (; str[start] != '\0'; start++) {
        if(str[start] != ' ') break;
    }
    for (; end >= 0; end--) {
        if(str[end] != ' ') break;
    }
    //cout <<  end << endl;
    string array;
    for (int i = 0; i != end-start+1; i++) {
        //cout << this->cstr[i+start] << endl;
        array += str[i+start];
    }
    return array;
}
