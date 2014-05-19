#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "common.h"
#include "module.h"
#include "token.h"
#include <map>

class Assembler
{
    typedef pair<string, ifstream*> namedFile;

    Module module;

    vector<string> sources;
    vector<Function> funcs;
    map<string, ifstream*> files;

public:
    Assembler();
    Assembler(string modname);

    Assembler& operator <<(string file);

    void SetArg(string key, bool arg);
    void SetArg(string key, string arg);

    void Compile();

    void Write();

private:
    void readGlobals(ifstream *ifs);

    void checkForSymbol(string sym, string tok, Token::TokType type);

    Type stringToType(string str);

    OpCodes stringToOp(string str);
};

#endif // ASSEMBLER_H
