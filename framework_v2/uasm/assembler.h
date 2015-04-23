#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include "common.h"
#include "module.h"
#include "opcodes.h"
#include "token.h"
#include <map>


class Assembler
{

    typedef vector<Type> argList;

    vector<addr_t> addressTable;

    map<string, string> configFlags {
        {"internal-api", "enabled"},
        {"default-scope", "private"}
    };

    Module mainModule;
    vector<ifstream*> files;
public:
    Assembler();
    Assembler(string modname);

    Assembler& operator << (string file);

    void Compile();

    void Write();
    ~Assembler();
private:
    GlobalVar readGlobal(string line);
    void readFunction(Token &tok, int ifs);
    void checkToken(string tok, string t);
    Type strToType(string str, bool voidAlowed=false);
    void compileFunc(Function &f);
    OpCode strToOpCode(string str);
    vector<Type> readFunctionArgs(Token &tok);
    bool isInternal(string sign, vector<Type> args);
    string &ltrim(string &s);
    string &rtrim(string &s);
    string &trim(string &s);
    void pushAddr(addr_t addr, vector<byte> &bytes);
    void pushUTF8(string str, vector<byte> &bytes);
    void pushDouble(double val, vector<byte> &bytes);
    void link(Function &f);
    void writeInt(int i, ofstream &ofs);
    void writeAddr(addr_t i, ofstream &ofs, bool bigendian=false);
    void setInternal(Type ret, const char *name, farglist types);
};

#endif // ASSEMBLER_H
