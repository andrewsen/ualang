#include "assembler.h"
#include <fstream>
#include <algorithm>

string trim(string source);

Assembler::Assembler()
{

}
Assembler::Assembler(string modname) : module(modname)
{

}

Assembler& Assembler::operator <<(string file)
{
    string line;
    ifstream * source = new ifstream;

    source->open(module.GetName().c_str());

    while(!source->eof()) {
        getline(*source, line);

        Token tok(line);

        if(tok.Current() == ".module") {
            if(module.name != "")
                throw "Module name already defined!";
            this->module.name = tok.Next();
        }
        else if(tok.Current() == "globals") {
            if(tok.Next() != "[" && tok.Type())
                throw "'[' expected!";
            readGlobals(source);
        }
        else if(tok.Current() == ".entry") {
            module.entry.signature = tok.Tail();
        }
        else if(tok.Current() == "public" || tok.Current() == "private") {
            Function fun;
            fun.isPublic = tok.Current() == "public" ? true : false;
            fun.retType = stringToType(tok.Next());
            fun.signature = trim(tok.Tail());

            getline(*source, line);
            checkForSymbol("{", trim(line), Token::DELIM);

            fun.stream_beg = source->tellg();

            do {
                getline(*source, line);
                Token body = line;
                if(body.Current() == "}" && body.Type() == Token::DELIM)
                    break;
                fun.stream_end = source->tellg();
            } while(true);

            funcs.push_back(fun);
        }
    }

    this->sources.push_back(file);
    files.insert(namedFile(file, source));
    return *this;
}

void Assembler::readGlobals(ifstream *ifs) {
    string line;
    getline(*ifs, line);
    while(true) {
        Token tok = line;
        if(tok.Current() == "]") break;

        GlobalVar var;
        var.id = atoi(tok.Current().c_str());

        checkForSymbol(":", tok.Next(), tok.Type());

        var.type = stringToType(tok.Next());

        tok.Next();

        if(tok.Type() != Token::EOS && tok.Current() == "->") {
            var.isPublic = true;
            var.name = tok.Next();
        }

        module.vars_seg.gvars.push_back(var);
    }
}

void Assembler::checkForSymbol(string sym, string tok, Token::TokType type) {
    if(tok == sym && type != Token::STRING) return;
    else throw ("'" + sym + "' expected!").c_str();
}

Type Assembler::stringToType(string str) {
    if(str == "ui8") return Type::UI8;
    else if(str == "ui16") return Type::UI16;
    else if(str == "i16") return Type::UI16;
    else if(str == "ui32") return Type::UI32;
    else if(str == "i32") return Type::I32;
    else if(str == "ui64") return Type::UI64;
    else if(str == "i64") return Type::I64;
    else if(str == "double") return Type::DOUBLE;
    else if(str == "bool") return Type::BOOL;
    else if(str == "string") return Type::UTF8;

    else throw "Illegal type " + str;
}

void Assembler::Compile() {
    sort(module.vars_seg.gvars.begin(), module.vars_seg.gvars.end(), [&](GlobalVar v1, GlobalVar v2) {return v1.id > v2.id;});

    for(auto f : funcs) {

    }

    for(auto file : files) {
        file.second->close();
    }
}

OpCodes Assembler::stringToOp(string str) {
    switch(str[0]) {
    case 'a':
        if(str == "add") return OpCodes::ADD;
        else if(str == "addf") return OpCodes::ADDF;
        else if(str == "and") return OpCodes::AND;
        break;
    case 'b':
        if(str == "band") return OpCodes::BAND;
        else if(str == "bor") return OpCodes::BOR;
        break;
    case 'c':
        if(str == "conv_ui8") return OpCodes::CONV_UI8;
        else if(str == "conv_i16") return OpCodes::CONV_I16;
        else if(str == "conv_ui32") return OpCodes::CONV_UI32;
        else if(str == "conv_i32") return OpCodes::CONV_I32;
        else if(str == "conv_ui64") return OpCodes::CONV_UI64;
        else if(str == "conf_i64") return OpCodes::CONV_I64;
        else if(str == "conv_f") return OpCodes::CONV_F;
        else if(str == "call") return OpCodes::CALL;
        break;
    case 'd':
        break;
    case 'e':
        break;
    case 'f':
        break;
    case 'i':
        break;
    case 'j':
        break;
    case 'l':
        break;
    case 'n':
        break;
    case 'o':
        break;
    case 'r':
        break;
    case 's':
        break;
    case 't':
        break;
    case 'x':
        break;
    }
}

string trim(string source) {
    int beg = 0, end = source.length()-1;

    for(; beg < source.length(); ++beg)
        if(source[beg] != ' ' && source[beg] != '\t') break;
    for(; beg >= 0; --beg)
        if(source[beg] != ' ' && source[beg] != '\t') break;

    return source.substr(beg, end);
}
