#ifndef MODULE_H
#define MODULE_H

#include <string>
#include <vector>
#include <fstream>

#include <iostream>

#include "common.h"
#include "opcodes.h"

using namespace std;

class Runtime;
class Module;

static const uint SIGN_LENGTH = 80;

struct LocalVar
{
    uint addr;
    Type type;
};

struct Function
{
    static const uint ARGS_LENGTH = 15;

    OpCode* bytecode;
    uint bc_size;
    LocalVar* locals; //TODO: create local variabels memory
    uint local_mem_size; //TODO: create local variabels memory
    uint locals_size;
    bool isPrivate;
    char sign[SIGN_LENGTH];
    uint argc;
    uint args_size;
    LocalVar args[ARGS_LENGTH];
    Type ret;
    bool internal;

    Module* module;
};

struct Header {
    enum Type : byte {
        Bytecode=1, Strings, Imports, Metadata, Functions, Globals, User
    } type;
    string name;
    uint begin;
    uint end;
};

struct GlobalVar {
    byte* addr;
    char name[SIGN_LENGTH];
    Type type;
    bool isPrivate;
};

class Module
{
    friend class Runtime;

    const static uint MOD_MAGIC = 0x4153DEC0;

    ModuleFlags mflags;

    Runtime * rt;
    Module * included;
    Function ** functions;
    GlobalVar * globals;    
    char * strings;

    uint func_count;
    uint globals_count;
    uint included_count;
    uint strings_count;

    vector<Header> headers;

    Function* __global_constructor__ = nullptr;

    string file;
public:
    class InvalidMagicException : public Exception{
        uint magic;

    public:
        InvalidMagicException(uint mg) : magic(mg)
        {
            what = "Invalid module magic";
        }

        uint GetMagic() {return magic;}
    };

    Module();

    void Load(string file);
private:
    void readSegmentHeaders(ifstream &ifs, uint hend);
    void readSegments(ifstream &ifs);
};

#endif // MODULE_H
