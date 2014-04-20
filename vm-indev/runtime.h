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

#ifndef RUNTIME_H
#define RUNTIME_H

#include <iostream>
#include <typeinfo>
#include <dlfcn.h>
#include <sstream>
#include <ctime>
#include <unordered_map>
#include "export.h"
#include "module.h"
#include <sstream>
struct Array;

extern unordered_map<uint, Array> array_map;
extern vector<memory_unit> prog_stack;		//COMPILABLE
//extern vector<Array> array_stack;
//extern map<uint, Array> array_map;
extern vector<string> str_stack;				//COMPILABLE
extern memory_unit regs[9];

extern time_t start_time1, start_time2, end_time;

typedef pair<uint, Array> pointed_array;

#define vr1 regs[0]
#define vr2 regs[1]
#define vr3 regs[2]
#define vr4 regs[3]
#define vr5 regs[4]
#define vri regs[5]
#define vip regs[6]
#define vsp regs[7]
#define vra regs[8]

void native_call(...);

vector<string> split(string p, char ch, bool rmempty);
bool endsWith(string s, const string &str);
bool endsWith(string s, const string &str, bool ignoreCase);
string trim(string str);

class RuntimeException {
    string wat;

    RuntimeException() {

    }

    RuntimeException(string what) {
        wat = what;
    }

    friend class Runtime;
    friend class Module;
    friend class NativeCall;
public:
    enum State {Warning, Fault, CriticalFault};
    string what() {return wat;}
    string where();
    string data();
    State type();

};

class Runtime
{
    //Runtime self;
    int argc;
    const char** argv;
    class Log{
        friend class Runtime;
        ostream *out;
        bool on = false;
    public:
        Log() {
            out = new ofstream("/dev/null");
            //out = &cerr;
        }
        Log(bool on) {
            if(on)
                out = &cerr;
            else
                out = new ofstream("/dev/null");
        }

        ~Log() {
            *out << "\033[0m";
            if(!on)
                delete out;
        }

        void SwitchOn(bool state) {
            if(state && !on) {
                delete out;
                out = &cerr;
            }
            else if(!state && on) {
                out = new ofstream("/dev/null");
            }
        }

        template <typename T> ostream& operator << (const T &arg) {
            //stringstream ss;
            //ss << arg;
            *out << "\e[34m\e[1m" << arg;// << "\033[0m";
            return *out;
        }


    };
    bool(*th)(Runtime*, Module*);
//public:
    Runtime(int argc, const char* argv[]);

    int offset = 0, arrayOffset = 0, stringOffset = 0;
    static bool isLogOn;
    //vector<Module> modules;
    Module entry, *current;

    void printMemUnit(memory_unit u);
public:
    Runtime() {}
    Log log;

    static const int VERSION = 7;

    static Runtime *Create(int argc, const char *argv[]);

    void SetTraceHandler(/*VM::trace th*/bool(*th)(Runtime*, Module*)) {
        this->th = th;
    }

    inline void ThrowException(string what, RuntimeException::State type = RuntimeException::Fault) {
        string msg = "Помилка етапу виконання: \n" + what;
        if(type == RuntimeException::Warning) {
            *err << "\e[34m\e[1m";
            *err << msg + "\n" + "Продовжити виконання? (так/ні) ";
            string a;
            *in >>(a);
            *err << "\033[0m";
            if(a == "так" || a == "1") return;
            else exit(EXIT_FAILURE);

        }
        else throw RuntimeException(msg);
    }

    inline Module* GetModule(Module* m, uint addr) {
        //return &modules[m->moduleOffset + addr];
    }

    inline int GetOffset() {
        return offset;
    }

    inline int GetArrayOffset() {
        return arrayOffset;
    }

    inline int GetStringOffset() {
        return stringOffset;
    }

    inline Module* GetMainModule() {
        return &this->entry;
    }

    static inline void Logging(bool isOn) {
        isLogOn = isOn;
    }

    void PrintRegisters();

    void PrintStackTrace();

    void PrintStringsTrace();

    void PrintVMInfo(Module *m);

    void LoadModule(Module *parent, string str);

    void Handle(RuntimeException rte);

    void EnableTrace() {
        flags.TF = true;
    }

    void Trace(Module*m) {
        if(!th(this, m)) exit(EXIT_SUCCESS);
    }

    void Start();
    void Shutdown();
    static void Destroy(Runtime*);
};

class NativeCall {
    void* so, *fun;

    string name;
    vector<char*> types;

    string cppNameResolver() {
        stringstream sign;
        sign << "_Z";
        sign << name.length();
        sign << name;
        for (int i = 0; i < types.size(); i++) {
           sign << types[i];
        }
        return sign.str();
    }

public:
    NativeCall(string lib, string name, vector<char*> argTypes){
        so = dlopen(lib.c_str(), RTLD_LAZY);
        if (!so) {
            throw RuntimeException("Can't load dynamic library '" + name + "'");
        }
        types = argTypes;
        fun = dlsym(so, name.c_str());
        if (!fun) {
            throw RuntimeException("Can't load function from dynamic library '" + name + "'");
        }
    }

    void operator () ()
    {
        native_call();
        native_call("hello");
    }
};

#endif // RUNTIME_H
