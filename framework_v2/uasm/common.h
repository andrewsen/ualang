#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <cstdlib>
#include <vector>
#include "opcodes.h"

#define DEBUG
#define interface class

using namespace std;

typedef unsigned char byte;
typedef unsigned int addr_t;

class Exception {
protected:
    string msg;
public:
    virtual string Message() {
        return msg;
    }
};

class AssemblerException : public Exception {
public:
    AssemblerException(string ex)
    { msg = ex; }
};

struct GlobalVar {
    int id;
    string name = "";
    bool isPrivate = true;
    Type type;

    GlobalVar() {
        id = 0;
        name = "";
        isPrivate = true;
        type = Type::VOID;
    }

    GlobalVar(const GlobalVar &var) {
        id = var.id;
        //if(!var.isPrivate)
        name = var.name;
        isPrivate = var.isPrivate;
        type = var.type;
    }
};

struct LocalVar {
    int id = -1;
    Type type = Type::VOID;
};

struct Label {
    string name;
    addr_t addr;
    bool hasAddr;
};

typedef vector<Type> farglist;

class Function {
public:
    int file = 0;
    ifstream::pos_type beg;
    ifstream::pos_type end;

    Type retType = Type::VOID;
    string sign = "";
    string module = "";
    string argStr = "";
    farglist args;

    vector<LocalVar> localVars;
    vector<addr_t> labelAddrTable;
    vector<byte> bytecode;
    vector<Label> labelTable;

    bool isPrivate = true;
    bool imported = false;

    Function() {
        file = 0;
        args.clear();
    }
    ~Function() {
        args.clear();
    }
    Function(string name) : sign(name) {}
    Function(const Function &f) {
        file = f.file;
        beg = f.beg;
        end = f.end;
        retType = f.retType;
        sign = f.sign;
        args.insert(args.begin(), f.args.begin(), f.args.end());
        argStr = f.argStr;
        module = f.module;
        isPrivate = f.isPrivate;
        imported = f.imported;
        localVars.clear();
        localVars.insert(localVars.begin(), f.localVars.begin(), f.localVars.end());
        labelAddrTable.clear();
        labelAddrTable.insert(labelAddrTable.begin(), f.labelAddrTable.begin(), f.labelAddrTable.end());
        bytecode.clear();
        bytecode.insert(bytecode.begin(), f.bytecode.begin(), f.bytecode.end());
        labelTable.clear();
        labelTable.insert(labelTable.begin(), f.labelTable.begin(), f.labelTable.end());
    }
    Function(const Function &&f) {
        file = f.file;
        beg = f.beg;
        end = f.end;
        retType = f.retType;
        sign = f.sign;
        args.insert(args.begin(), f.args.begin(), f.args.end());
        argStr = f.argStr;
        module = f.module;
        isPrivate = f.isPrivate;
        imported = f.imported;
        localVars.clear();
        localVars.insert(localVars.begin(), f.localVars.begin(), f.localVars.end());
        labelAddrTable.clear();
        labelAddrTable.insert(labelAddrTable.begin(), f.labelAddrTable.begin(), f.labelAddrTable.end());
        bytecode.clear();
        bytecode.insert(bytecode.begin(), f.bytecode.begin(), f.bytecode.end());
        labelTable.clear();
        labelTable.insert(labelTable.begin(), f.labelTable.begin(), f.labelTable.end());
    }
    Function& operator=(const Function &f) {
        file = f.file;
        beg = f.beg;
        end = f.end;
        retType = f.retType;
        sign = f.sign;
        args.insert(args.begin(), f.args.begin(), f.args.end());
        argStr = f.argStr;
        module = f.module;
        isPrivate = f.isPrivate;
        imported = f.imported;
        localVars.clear();
        localVars.insert(localVars.begin(), f.localVars.begin(), f.localVars.end());
        labelAddrTable.clear();
        labelAddrTable.insert(labelAddrTable.begin(), f.labelAddrTable.begin(), f.labelAddrTable.end());
        bytecode.clear();
        bytecode.insert(bytecode.begin(), f.bytecode.begin(), f.bytecode.end());
        labelTable.clear();
        labelTable.insert(labelTable.begin(), f.labelTable.begin(), f.labelTable.end());
        return *this;
    }

    Function(string name, farglist &args) {
        sign = name;
        this->args = move(args);
        for(auto t : this->args) {
            argStr += (char)(byte)t;
        }
    }
    Function(string name, farglist &&args) {
        sign = name;
        this->args = move(args);
        for(auto t : this->args) {
            argStr += (char)(byte)t;
        }
    }

    /*void operator=(const Function &f) {
        file = f.file;
        beg = f.beg;
        end = f.end;
        retType = f.retType;
        sign = f.sign;
        args.insert(args.begin(), f.args.begin(), f.args.end());
        argStr = f.argStr;
        //addr = f.addr;
        isPrivate = f.isPrivate;
    }*/

    static bool ArgsEquals(farglist &a1, farglist &a2) {
        if(a1.size() != a2.size()) return false;
        else for(int i = 0; i < a1.size(); ++i)
            if(a1[i] != a2[i]) return false;
        return true;
    }
};

void pushInt(int i, vector<byte> &vec);
void pushAddr(addr_t a, vector<byte> &vec);

void warning(string msg);

inline static uint sizeOf(Type t) {
    switch (t) {
        case Type::BOOL:
        case Type::UI8:
            return 1;
        case Type::I16:
            return 2;
        case Type::I32:
        case Type::UI32:
        case Type::UTF8:
            return 4;
        case Type::DOUBLE:
        case Type::I64:
        case Type::UI64:
            return 8;
        default:
            return 0;
    }
}

#endif // COMMON_H

