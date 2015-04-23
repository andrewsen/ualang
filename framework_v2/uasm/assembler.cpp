#include <algorithm>
#include <functional>
#include "token.h"
#include "assembler.h"

Assembler::Assembler()
{
    Metadata meta;
    meta.key = "internal-api";
    meta.t = Metadata::Boolean;
    meta.bval = true;
    mainModule.metaSeg.elems.push_back(meta);
}
Assembler::~Assembler()
{
    for(int i = 0; i < files.size(); ++i) {
        files[i]->close();
        //delete files[i];
    }
}

Assembler::Assembler(string modname)
{
    mainModule.name = modname;
}

Assembler& Assembler::operator << (string file) {
    ifstream * ifs = new ifstream(file);
    files.push_back(ifs);
    string line;

    while(!ifs->eof()) {
        getline(*ifs, line);

        Token token = line;

        //cout << "Current tok: " + token.ToString() <<  "; length: " << token.ToString().size() <<endl;
        if(token.ToString() == ".globals") {
            getline(*ifs, line);
            while(line != ".end-globals" && !ifs->eof()) {
                mainModule.AddGlobal(readGlobal(line));
                getline(*ifs, line);
            }
        }
        else if(token == ".module") {
            mainModule.SetName(token.Next());
        }
        else if(token == ".import") {
            //mainModule.Import(token.Next());
        }
        else if(token == ".lib") {
            mainModule.mflags.executable_bit = 0;
        }
        else if(token == ".define") {
            checkToken(token.Next(), "(");
            auto key = token.Next();
            checkToken(token.Next(), ",");
            auto val = token.Next();
            auto type = token.Type();
            Metadata meta;
            meta.key = key;
            switch (type)
            {
                case Token::Bool:
                    meta.bval = val == "true" ? true : false;
                    meta.t = Metadata::Boolean;
                    break;
                case Token::String:
                    meta.sval = val;
                    meta.t = Metadata::String;
                    break;
                case Token::Digit:
                    if(val.find('.') != string::npos)
                    {
                        meta.dval = atof(val.c_str());
                        meta.t = Metadata::Float;
                    }
                    else
                    {
                        meta.i32val = atoi(val.c_str());
                        meta.t = Metadata::Integer;
                    }
                    break;
                case Token::Identifier:
                    if("raw") {
                        checkToken(token.Next(), ":");
                        val = token.Next();
                        checkToken(val.substr(0, 2), "0x");
                        val.erase(0, 2);
                        meta.t = Metadata::Raw;
                    }
                    break;

            }
            checkToken(token.Next(), ")");

            mainModule.metaSeg.elems.push_back(meta);

            //configFlags[key] = val;
        }
        else if(token == "public" || token == "private") {
            readFunction(token, files.size()-1);
        }
        //cout << "file is " << (!ifs->eof() ? "good" : "bad") << endl;
    }
    return *this;
}

GlobalVar Assembler::readGlobal(string line) {
    GlobalVar var;
    Token tok = line;
    var.id = atoi(tok.ToString().c_str());
    checkToken(tok.Next(), ":");
    var.type = strToType(tok.Next());
    checkToken(tok.Next(), "->");
    if(tok.Next() == "public") var.isPrivate = false;
    else if(tok.ToString() != "private") {
        throw AssemblerException("Scope specifier expected: " + tok.ToString());
    }

    var.name = tok.Next();
    cout << "vName: " << var.name << endl;
    return var;
}

void Assembler::readFunction(Token &tok, int ifs) {
    Function fun;

    if(tok.ToString() == "public") {
        fun.isPrivate = false;
    }

    fun.retType = strToType(tok.Next(), true);
    fun.sign = tok.Next();
    tok.Next();
    while(tok.Next() != ")") {
        fun.args.push_back(strToType(tok.ToString()));
        if(tok.Next() != ",")
            if(tok.ToString() == ")")
                break;
            else
                throw AssemblerException("Unexpected symbol in function declaration '" + tok.ToString() + "'");
    }

    string line;

    getline(*files[ifs], line);
    if(trim(line) == ".locals") {
        getline(*files[ifs], line);
        while(trim(line) != ".end-locals") {
            Token tok = line;
            LocalVar lv;
            lv.id = atoi(tok.ToString().c_str());
            checkToken(tok.Next(), ":");
            lv.type = strToType(tok.Next());
            fun.localVars.push_back(lv);

            getline(*files[ifs], line);
        }
        getline(*files[ifs], line);
    }
    if(trim(line) != "{")
        throw AssemblerException("'{' expected, but used '" + line + "'");
    fun.beg = files[ifs]->tellg();
    do {
        fun.end = files[ifs]->tellg();
        getline(*files[ifs], line);
    } while(trim(line) != "}");

    fun.file = ifs;
    mainModule.AddFunction(fun);
    //cout << "file is " << (ifs->gcount()) << endl;
}

vector<Type> Assembler::readFunctionArgs(Token &tok) {
    vector<Type> args;
    while(tok.Next() != ")") {
        args.push_back(strToType(tok.ToString()));
        if(tok.Next() != ",")
            if(tok.ToString() == ")")
                break;
            else
                throw AssemblerException("Unexpected symbol in function declaration '" + tok.ToString() + "'");
    }
    return args;
}

void Assembler::setInternal(Type ret, const char* name, farglist types)
{
    Function fint(string(name), types);
    fint.retType = ret;
    fint.imported = true;
    fint.module = "::vm.internal";
    mainModule.functionSeg.funcs.push_back(fint);
}

void Assembler::Compile()
{
    const auto& iapi = mainModule.metaSeg["internal-api"];
    if(iapi.t != Metadata::None && iapi.bval == true) {

        setInternal(Type::VOID, "print", farglist{Type::UTF8});
        setInternal(Type::VOID, "print", farglist{Type::I32});
        setInternal(Type::VOID, "print", farglist{Type::DOUBLE});
        setInternal(Type::DOUBLE, "readd", farglist());
        setInternal(Type::I32, "readi", farglist());
        setInternal(Type::UTF8, "reads", farglist());

        mainModule.mflags.no_internal_bit = 0;
    }

    //mainModule.bytecodeSeg.bytecode.push_back((byte)OpCode::NOP);
    if(mainModule.globalsSeg.vars.size() == 0)
        mainModule.mflags.no_globals_bit = 1;

    for(auto& f : mainModule.functionSeg.funcs) {
        compileFunc(f);
        link(f);
    }

}

void Assembler::compileFunc(Function &f) {
    auto& bc = f.bytecode;

    //f.addr = bc.size();

    ifstream *ifs = files[f.file];
    ifs->clear();
    ifs->seekg(f.beg, ios::beg);
    cout << "file is " << (ifs->good() ? "good" : "bad") << endl;
    string line = "";
    while(ifs->tellg() != f.end && !ifs->eof()) {
        getline(*ifs, line);
        if(trim(line) == "") continue;

        Token tok = line;
        if(tok.Next() == ":") {
            auto name = tok.PushBack();
            bool found = false;
            for(int i = 0; i < f.labelTable.size(); ++i) {
                if(f.labelTable[i].name == name) {
                    if(f.labelTable[i].hasAddr)
                        throw AssemblerException("Label " + name + " already defined");
                    else {
                        f.labelTable[i].hasAddr = true;
                        f.labelTable[i].addr = bc.size();
                    }
                    found = true;
                    break;
                }
            }
            if(!found) {
                Label l;
                l.addr = bc.size();
                l.hasAddr = true;
                l.name = name;
                f.labelTable.push_back(l);
            }
            continue;
        }
        OpCode op = strToOpCode(tok.PushBack());

        switch(op) {
            case OpCode::CALL:
                {
                    bc.push_back((byte)OpCode::CALL);
                    if(tok.Next() != "[") {
                        auto sign = tok.ToString();
                        auto args = move(readFunctionArgs(tok.NextToken()));

                        int addr = -1;

                        for(int i = 0; i < mainModule.functionSeg.funcs.size(); ++i) {
                            auto& f = mainModule.functionSeg.funcs[i];
                            if(f.sign == sign && Function::ArgsEquals(f.args, args))
                                addr = i;
                        }
                        if(addr == -1) throw AssemblerException("Can't find function " + sign);
                        //addressTable.push_back(bc.size());
                        pushAddr(addr, bc);
                    }
                    else {
                        // call [someModule] someFun(i32)
                        // call [this]
                        auto module = tok.NextWhileNot(']', true);
                        auto sign = tok.Next();
                        auto args = move(readFunctionArgs(tok.NextToken()));

                        int addr = -1;

                        for(int i = 0; i < mainModule.functionSeg.funcs.size(); ++i) {
                            auto& f = mainModule.functionSeg.funcs[i];
                            if(f.sign == sign && Function::ArgsEquals(f.args, args))
                                addr = i;
                        }
                        //int addr = -1;
                        if(addr == -1) {
                            cout << "Not found: [" << module << "]" << sign << endl;
                            Function impFun(sign, args);
                            impFun.imported = true;
                            impFun.module = module;

                            //for(int i = 0; i < mainModule.importSeg.imports.size(); ++i) {
                            //   ImportedModule& im = mainModule.importSeg.imports[i];
                            //   if(im.name == module) {
                            //       addr = mainModule.functionSeg.funcs.size();
                            //       im.funcPtrs.push_back(addr);
                            //       break;
                            //   }
                            //}

                            //if(addr == -1) throw AssemblerException("Can't find module " + module);
                            addr = mainModule.functionSeg.funcs.size();
                            mainModule.functionSeg.funcs.push_back(impFun);
                            //mainModule.ImportIfNew(module, sign, args);
                        }
                        pushAddr(addr, bc);
                    }
                    //if(addr == -1) {
                    //•••••••••••••••••••••••••        //    bc.push_back((byte)OpCode::CALL_INTERNAL);
                    //    pushAddr(mainModule.ImportIfNew(sign, args), bc);
                    //}
                }
            break;
            case OpCode::JF:
            case OpCode::JMP:
            case OpCode::JNNULL:
            case OpCode::JNULL:
            case OpCode::JNZ:
            case OpCode::JT:
            case OpCode::JZ:
            {
                bc.push_back((byte)op);
                string name = tok.Next();
                bool found = false;
                for(int i = 0; i < f.labelTable.size(); ++i) {
                    if(f.labelTable[i].name == name) {
                        if(f.labelTable[i].hasAddr)
                            pushAddr(f.labelTable[i].addr, bc);
                        else {
                            f.labelAddrTable.push_back(bc.size());
                            pushAddr(i, bc);
                        }
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    //cout << "Label " << name << " wasn't found yet";
                    Label l;
                    l.hasAddr = false;
                    l.name = name;
                    f.labelAddrTable.push_back(bc.size());
                    pushAddr(f.labelTable.size(), bc);
                    f.labelTable.push_back(l);
                }
            }
            break;
            case OpCode::FREELOC:
            case OpCode::LDARG:
            case OpCode::LDFLD:
            case OpCode::LDLOC:
            case OpCode::LD_I16:
            case OpCode::LD_I32:
            case OpCode::LD_I64:
            case OpCode::LD_UI8:
            case OpCode::LD_UI32:
            case OpCode::LD_UI64:
            case OpCode::STARG:
            case OpCode::STFLD:
            case OpCode::STLOC:
            //case OpCode::TOP:
            {
                bc.push_back((byte)op);
                pushAddr(atoi(tok.Next().c_str()), bc);
            }
            break;
            case OpCode::LD_F:
            {
                bc.push_back((byte)op);
                pushDouble(atof(tok.Next().c_str()), bc);
            }
            break;
            case OpCode::LD_STR:
            {
                bc.push_back((byte)op);
                pushUTF8(tok.Next(), bc);
            }
            break;
            case OpCode::NEWLOC:
            {
                Type t = strToType(tok.Next());
                bc.push_back((byte)op);
                bc.push_back((byte)t);
            }
            break;
            case OpCode::UNDEFINED:
                throw AssemblerException("Unknown opcode " + tok.ToString());
                break;
            default:
                bc.push_back((byte)op);
                break;
        }
    }
}

OpCode Assembler::strToOpCode(string str) {
    switch (str[0]) {
    case 'a':
        if(str == "add") return OpCode::ADD;
        else if(str == "addf") return OpCode::ADDF;
        else if(str == "and") return OpCode::AND;
        break;
    case 'b':
        if(str == "band") return OpCode::BAND;
        else if(str == "bor") return OpCode::BOR;
        break;
    case 'c':
        if(str == "call") return OpCode::CALL;
        else if(str == "conv_f") return OpCode::CONV_F;
        else if(str == "conv_i16") return OpCode::CONV_I16;
        else if(str == "conv_i32") return OpCode::CONV_I32;
        else if(str == "conv_i64") return OpCode::CONV_I64;
        else if(str == "conv_ui8") return OpCode::CONV_UI8;
        else if(str == "conv_ui32") return OpCode::CONV_UI32;
        else if(str == "conv_ui64") return OpCode::CONV_UI64;
        break;
    case 'd':
        if(str == "dec") return OpCode::DEC;
        else if(str == "div") return OpCode::DIV;
        else if(str == "divf") return OpCode::DIVF;
        break;
    case 'e':
        if(str == "eq") return OpCode::EQ;
        break;
    case 'f':
        if(str == "freeloc") return OpCode::FREELOC;
        break;
    case 'g':
        if(str == "gt") return OpCode::GT;
        else if(str == "gte") return OpCode::GTE;
        break;
    case 'i':
        if(str == "inc") return OpCode::INC;
        break;
    case 'j':
        if(str == "jf") return OpCode::JF;
        else if(str == "jmp") return OpCode::JMP;
        else if(str == "jnnull") return OpCode::JNNULL;
        else if(str == "jnull") return OpCode::JNULL;
        else if(str == "jnz") return OpCode::JNZ;
        else if(str == "jt") return OpCode::JT;
        else if(str == "jz") return OpCode::JZ;
        break;
    case 'l':
        if(str == "ld_0") return OpCode::LD_0;
        else if(str == "ld_1") return OpCode::LD_1;
        else if(str == "ld_2") return OpCode::LD_2;
        else if(str == "ldarg") return OpCode::LDARG;
        else if(str == "ldarg_0") return OpCode::LDARG_0;
        else if(str == "ldarg_1") return OpCode::LDARG_1;
        else if(str == "ldarg_2") return OpCode::LDARG_2;
        else if(str == "ldfld") return OpCode::LDFLD;
        else if(str == "ldfld_0") return OpCode::LDFLD_0;
        else if(str == "ldfld_1") return OpCode::LDFLD_1;
        else if(str == "ldfld_2") return OpCode::LDFLD_2;
        else if(str == "ldloc") return OpCode::LDLOC;
        else if(str == "ldloc_0") return OpCode::LDLOC_0;
        else if(str == "ldloc_1") return OpCode::LDLOC_1;
        else if(str == "ldloc_2") return OpCode::LDLOC_2;
        else if(str == "ld_f") return OpCode::LD_F;
        else if(str == "ld_false") return OpCode::LD_FALSE;
        else if(str == "ld_true") return OpCode::LD_TRUE;
        else if(str == "ld_null") return OpCode::LD_NULL;
        else if(str == "ld_str") return OpCode::LD_STR;
        else if(str == "ld_ui8") return OpCode::LD_UI8;
        else if(str == "ld_ui32") return OpCode::LD_UI32;
        else if(str == "ld_ui64") return OpCode::LD_UI64;
        else if(str == "ld_i16") return OpCode::LD_I16;
        else if(str == "ld_i32") return OpCode::LD_I32;
        else if(str == "ld_i64") return OpCode::LD_I64;
        else if(str == "lt") return OpCode::LT;
        else if(str == "lte") return OpCode::LTE;
        break;
    case 'm':
        if(str == "mul") return OpCode::MUL;
        else if(str == "mulf") return OpCode::MULF;
        break;
    case 'n':
        if(str == "neg") return OpCode::NEG;
        else if(str == "neq") return OpCode::NEQ;
        else if(str == "newloc") return OpCode::NEWLOC;
        else if(str == "nop") return OpCode::NOP;
        else if(str == "not") return OpCode::NOT;
        break;
    case 'o':
        if(str == "or") return OpCode::OR;
        break;
    case 'p':
        if(str == "pop") return OpCode::POP;
        else if(str == "pos") return OpCode::POS;
        break;
    case 'r':
        if(str == "rem") return OpCode::REM;
        else if(str == "remf") return OpCode::REMF;
        else if(str == "ret") return OpCode::RET;
        break;
    case 's':
        if(str == "starg") return OpCode::STARG;
        else if(str == "starg_0") return OpCode::STARG_0;
        else if(str == "starg_1") return OpCode::STARG_1;
        else if(str == "starg_2") return OpCode::STARG_2;
        else if(str == "stfld") return OpCode::STFLD;
        else if(str == "stfld_0") return OpCode::STFLD_0;
        else if(str == "stfld_1") return OpCode::STFLD_1;
        else if(str == "stfld_2") return OpCode::STFLD_2;
        else if(str == "stloc") return OpCode::STLOC;
        else if(str == "stloc_0") return OpCode::STLOC_0;
        else if(str == "stloc_1") return OpCode::STLOC_1;
        else if(str == "stloc_2") return OpCode::STLOC_2;
        else if(str == "shl") return OpCode::SHL;
        else if(str == "shr") return OpCode::SHR;
        else if(str == "sizeof") return OpCode::SIZEOF;
        else if(str == "sub") return OpCode::SUB;
        else if(str == "subf") return OpCode::SUBF;
        break;
    case 't':
        if(str == "top") return OpCode::TOP;
        if(str == "typeof") return OpCode::TYPEOF;
        break;
    case 'x':
        if(str == "xor") return OpCode::XOR;
        break;
    }
    return OpCode::UNDEFINED;
}

Type Assembler::strToType(string str, bool voidAlowed) {
    if(str == "ui8") return Type::UI8;
    else if(str == "i16") return Type::I16;
    else if(str == "ui32") return Type::UI32;
    else if(str == "i32") return Type::I32;
    else if(str == "ui64") return Type::UI64;
    else if(str == "i64") return Type::I64;
    else if(str == "double") return Type::DOUBLE;
    else if(str == "string") return Type::UTF8;
    else if(str == "bool") return Type::BOOL;
    else if(str == "void" && voidAlowed) return Type::VOID;
    else throw AssemblerException("Unknown type " + str);
}

void Assembler::link(Function &f) {
    union {
        addr_t a;
        byte bytes[4];
    } a2b;
    auto& bc = f.bytecode;//mainModule.bytecodeSeg.bytecode;

    /*for(addr_t addr : addressTable) {
        a2b.bytes[0] = bc[addr];
        a2b.bytes[1] = bc[addr+1];
        a2b.bytes[2] = bc[addr+2];
        a2b.bytes[3] = bc[addr+3];
        Function f = mainModule.functionSeg.funcs[a2b.a];
        a2b.a = f.addr;
        bc[addr] = a2b.bytes[0];
        bc[addr+1] = a2b.bytes[1];
        bc[addr+2] = a2b.bytes[2];
        bc[addr+3] = a2b.bytes[3];
    }*/
    for(addr_t addr : f.labelAddrTable) {
        a2b.bytes[0] = bc[addr];
        a2b.bytes[1] = bc[addr+1];
        a2b.bytes[2] = bc[addr+2];
        a2b.bytes[3] = bc[addr+3];
        Label l = f.labelTable[a2b.a];
        if(!l.hasAddr) throw AssemblerException("Label " + l.name + " wasn't defined");
        a2b.a = l.addr;
        bc[addr] = a2b.bytes[0];
        bc[addr+1] = a2b.bytes[1];
        bc[addr+2] = a2b.bytes[2];
        bc[addr+3] = a2b.bytes[3];
        //cout << "Jumping to " << l.name << " with addr: 0x" << hex << l.addr << dec << endl;
    }
}

void Assembler::pushAddr(addr_t addr, vector<byte> &bytes) {
    ::pushAddr(addr, bytes);
}

void Assembler::pushDouble(double val, vector<byte> &bytes) {
    union {
        byte bytes[sizeof(double)];
        double val;
    } d2b;
    d2b.val = val;
    for(int i = 0; i < sizeof(double); ++i) bytes.push_back(d2b.bytes[i]);
}

void Assembler::pushUTF8(string str, vector<byte> &bytes) {
    //mainModule.offsetSeg.strings.push_back(bytes.size());
    pushAddr(mainModule.stringSeg.utf8.size(), bytes);
    //bytes.push_back(mainModule.stringSeg.utf8.size());
    for(char ch : str) {
        mainModule.stringSeg.utf8.push_back(ch);
    }
    mainModule.stringSeg.utf8.push_back('\0');
}

void Assembler::Write()
{
    ofstream ofs(mainModule.name + ".sem", ios::out | ios::binary);
    ofs.put(0xC0);
    ofs.put(0xDE);
    ofs.put(0x53);
    ofs.put(0x41);
    //writeAddr(0xC0DE5341, ofs);
    //cout << "SIZEOF: " << sizeof(ModuleFlags) << endl;
    /*ofs.put((char)0xC0);
    ofs.put((char)0xDE);
    ofs.put('S');
    ofs.put('A');*/
    //writeInt(mainModule.bytecodeSeg.bytecode.size(), ofs);

    vector<Segment*> &&segments = mainModule.AllSegments();

    int offset = 8 + sizeof(ModuleFlags);

    for(Segment* s : segments)
        offset += 10 + s->Name().size();
    writeInt(offset, ofs);
    ofs.write((char*)&mainModule.mflags, sizeof(ModuleFlags));

    for(Segment* s : segments) {
        //if(s->getBytes().size() == 0)
        //    continue;
        ofs.write(s->Name().c_str(), s->Name().size());
        ofs.put('\0');
        ofs.put((byte)s->GetType());
        writeInt(offset, ofs);
        offset += s->size();
        writeInt(offset, ofs);
    }
    for(Segment* s : segments) {
        auto bytes = move(s->getBytes());
        for(byte b : bytes) {
            //cout << "Before\n";
            ofs.put(b);
            //cout << "After\n";
        }
    }
    ofs.close();
    cout << "Successfully compiled!\n";
}

void Assembler::writeInt(int i, ofstream &ofs) {
    union {
        int i;
        byte bytes[4];
    } i2b;
    i2b.i = i;
    ofs.put(i2b.bytes[0]);
    ofs.put(i2b.bytes[1]);
    ofs.put(i2b.bytes[2]);
    ofs.put(i2b.bytes[3]);
}


void Assembler::writeAddr(addr_t i, ofstream &ofs, bool bigendian) {
    union {
        addr_t a;
        byte bytes[4];
    } i2b;
    i2b.a = i;
    if(bigendian) {
        ofs.put(i2b.bytes[3]);
        ofs.put(i2b.bytes[2]);
        ofs.put(i2b.bytes[1]);
        ofs.put(i2b.bytes[0]);
    }
    else {
        ofs.put(i2b.bytes[0]);
        ofs.put(i2b.bytes[1]);
        ofs.put(i2b.bytes[2]);
        ofs.put(i2b.bytes[3]);
    }
}

void Assembler::checkToken(string tok, string t) {
    if(tok != t)
        throw AssemblerException(t + " expected");
}

///@deprecated
/*bool Assembler::isInternal(string sign, vector<Type> args){
    if(internalAPI.find(sign) != internalAPI.end())
        if(Function::ArgsEquals(args, internalAPI[sign])) return true;
    return false;
}*/

inline string& Assembler::ltrim(string &s) {
        s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
        return s;
}

inline string& Assembler::rtrim(string &s) {
        s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
        return s;
}

inline string& Assembler::trim(string &s) {
        return ltrim(rtrim(s));
}

void pushInt(int i, vector<byte> &vec) {
    union {
        byte bytes[4];
        int i;
    } i2b;
    i2b.i = i;
    vec.push_back(i2b.bytes[0]);
    vec.push_back(i2b.bytes[1]);
    vec.push_back(i2b.bytes[2]);
    vec.push_back(i2b.bytes[3]);
}

void pushAddr(addr_t a, vector<byte> &vec) {
    union {
        byte bytes[4];
        addr_t addr;
    } addr2bytes;
    addr2bytes.addr = a;
    vec.push_back(addr2bytes.bytes[0]);
    vec.push_back(addr2bytes.bytes[1]);
    vec.push_back(addr2bytes.bytes[2]);
    vec.push_back(addr2bytes.bytes[3]);
}
