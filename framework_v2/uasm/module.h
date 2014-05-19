#ifndef MODULE_H
#define MODULE_H

#include <fstream>
#include "common.h"

class Function {
public:
    string signature;
    Type retType;
    uint addr;
    bool isPublic;
    ifstream::pos_type stream_beg;
    ifstream::pos_type stream_end;

    vector<string> body;
};

struct GlobalVar {
    Type type;
    bool isPublic;
    string name;
    int id;

    GlobalVar() : id(-1), type(Type::NUL), isPublic(false)
    {   }

    GlobalVar(int id, Type type) : id(id), type(type), isPublic(false)
    {   }

    GlobalVar(int id, Type type, string name) : id(id), type(type), isPublic(true), name(name)
    {   }
};

class Segment {
protected:
    string name;
    int size;

    virtual void calcSize() = 0;
public:
    virtual string GetName() {
        return name;
    }

    virtual int GetSize() {

    }
};

class BytecodeSegment : Segment {
protected:
    virtual void calcSize() {

    }

public:
    vector<byte> bytecode;
};

class StringSegment : Segment {
protected:
    virtual void calcSize() {

    }
public:
    vector<string> strings;
};

class GlobalsSegment : Segment {
protected:
    virtual void calcSize() {

    }
public:
    vector<GlobalVar> gvars;
};

struct header {
    Segment * seg;
    ui32 addr;
};

class Module
{
    string name;
    vector <header> headers;
    
    BytecodeSegment bytecode_seg;
    StringSegment string_seg;
    GlobalsSegment vars_seg;
    Function entry;
public:
    friend class Assembler;

    Module();

    Module(string name);

    string GetName();
};


#endif // MODULE_H
