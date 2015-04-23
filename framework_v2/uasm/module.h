#ifndef MODULE_H
#define MODULE_H
#include "common.h"
#include "importedmodule.h"
#include <algorithm>

interface Segment {
public:
    enum Type : byte {
        Bytecode=1, Strings, Imports, Metadata, Functions, Globals, User
    };

protected:
    string name;
    Segment::Type type;
public:

    virtual uint size() = 0;
    virtual void appendTo(vector<byte> &bytes) = 0;
    virtual vector<byte> getBytes() = 0;
    virtual string Name() { return name; }
    virtual Segment::Type GetType() { return type; }
};

struct Metadata {
    enum Type : byte {
        Void, Integer, Float, Boolean, String, Raw, None
    };

    string key;
    Metadata::Type t;
    string sval;
    vector<byte> rval;
    union {
        int i32val;
        uint dconv[2];
        double dval;
        bool bval;
    };

    bool compilable = true;
};

/*class BytecodeSegment : public Segment
{
public:
    vector<byte> bytecode;

    BytecodeSegment() {
        type = Segment::Bytecode ;
        name = "bytecode";
    }

    virtual uint size() {
        return bytecode.size();
    }

    virtual void appendTo(vector<byte> &bytes) {
        bytes.insert(bytes.end(), bytecode.begin(), bytecode.end());
    }

    virtual vector<byte> getBytes() {
        return bytecode;
    }
};*/

class GlobalsSegment : public Segment {
public:
    vector<GlobalVar> vars;
    vector<byte> bytes;

    GlobalsSegment() {
        type = Segment::Globals;
        name = "globals";
    }

    virtual uint size() {
        if(bytes.empty())
            generateBytes();
        return bytes.size();
    }

    virtual void appendTo(vector<byte> &bytes) {
        throw "Unimplemented";
    }

    virtual vector<byte> getBytes() {
        if(bytes.empty())
            generateBytes();
        return bytes;
    }
private:
    void generateBytes() {
        if(vars.size() == 0)
        {
            bytes.clear();
            return;
        }
        sort(vars.begin(), vars.end(), [&](const GlobalVar &v1, const GlobalVar &v2) {return v1.id < v2.id;});
        pushAddr(vars.size(), bytes);

#ifdef DEBUG
        for(int i = 0; i < vars.size(); ++i) {
            if(vars[i].id != i) cout << "Wrong id: " << vars[i].id << " at: " << i << endl;
            else cout << "Right id:" << i << endl;
            //cout << "Var name: " << vars[i].name << endl;
        }
#endif

        for(GlobalVar var : vars) {
            bytes.push_back((byte)var.type);
            //pushInt(var.id, bytes);

            if(var.isPrivate)
                bytes.push_back(1);
            else
                bytes.push_back(0);
            for(char ch : var.name) bytes.push_back(ch);
            bytes.push_back('\0');
        }
    }
};

class MetaSegment : public Segment {
public:
    vector<::Metadata> elems;
    vector<byte> bytes;

    MetaSegment() {
        type = Segment::Metadata;
        name = "meta";
    }

    virtual uint size() {
        if(bytes.empty())
            generateBytes();
        return bytes.size();
    }

    virtual void appendTo(vector<byte> &bytes) {
        throw "Unimplemented";
    }

    virtual vector<byte> getBytes() {
        if(bytes.empty())
            generateBytes();
        return bytes;
    }

    ::Metadata operator [](string key)
    {
        for(auto& m : elems)
            if(m.key == key)
                return m;
        ::Metadata none;
        none.sval = "";
        none.t = ::Metadata::None;
        return none;
    }

private:
    void generateBytes() {
        if(elems.size() == 0)
        {
            bytes.clear();
            return;
        }
        pushAddr(elems.size(), bytes);

        uint offset = 4 + elems.size() * 4;

        for(::Metadata m : elems)
        {
            if(!m.compilable)
                continue;
            pushAddr(offset, bytes);
            offset += m.key.size() + 2;
            switch (m.t) {
                case ::Metadata::Boolean:
                    ++offset;
                    break;
                case ::Metadata::Integer:
                    offset += 4;
                    break;
                case ::Metadata::Float:
                    offset += 8;
                    break;
                case ::Metadata::String:
                    offset += m.sval.size() + 5;
                    break;
                case ::Metadata::Raw:
                    offset += m.rval.size() + 5;
                    break;
                default:
                    break;
            }
        }

        for(::Metadata m : elems)
        {
            if(!m.compilable)
                continue;
            bytes.insert(bytes.end(), m.key.begin(), m.key.end());
            bytes.push_back(0);
            bytes.push_back((byte)m.t);
            switch (m.t) {
                case ::Metadata::Boolean:
                    bytes.push_back((byte)m.bval);
                    break;
                case ::Metadata::Integer:
                    pushInt(m.i32val, bytes);
                    break;
                case ::Metadata::Float:
                    pushAddr(m.dconv[0], bytes);
                    pushAddr(m.dconv[1], bytes);
                    break;
                case ::Metadata::String:
                    pushAddr(m.sval.size(), bytes);
                    bytes.insert(bytes.end(), m.sval.begin(), m.sval.end());
                    break;
                case ::Metadata::Raw:
                    pushAddr(m.sval.size(), bytes);
                    bytes.insert(bytes.end(), m.rval.begin(), m.rval.end());
                    break;
                default:
                    break;
            }
        }
    }
};

class FunctionSegment : public Segment {
public:
    vector<Function> funcs;
    vector<byte> bytes;

    FunctionSegment(){
        type = Segment::Functions;
        name = "funcs";
    }

    virtual uint size() {
        if(bytes.empty())
            generateBytes();
        return bytes.size();
    }

    virtual void appendTo(vector<byte> &bytes) {
        throw "Unimplemented";
    }

    virtual vector<byte> getBytes() {
        if(bytes.empty())
            generateBytes();
        return bytes;
    }

private:
    void generateBytes() {
        //sort(funcs.begin(), funcs.end(), [](const Function &v1, const Function &v2) {return v1.isPrivate && !v2.isPrivate;});
        pushAddr(funcs.size(), bytes);
        for(Function& f : funcs) {
#ifdef DEBUG
            cout << "Bytecode size in " << f.sign << f.argStr << " is " << f.bytecode.size() << endl;
#endif
            //bytes.push_back(0xCC);
            //pushAddr(f.addr, bytes);
            bytes.push_back((byte)f.retType);

            for(char ch : f.sign) bytes.push_back(ch);
            bytes.push_back('\0');
            for(::Type t : f.args) bytes.push_back((byte)t);
            bytes.push_back('\0');

            if(f.isPrivate)
                bytes.push_back(1);
            else
                bytes.push_back(0);

            if(f.imported) {
                bytes.push_back(1);
                for(char ch : f.module) bytes.push_back(ch);
                bytes.push_back('\0');
            }
            else {
                bytes.push_back(0);
                pushAddr(f.localVars.size(), bytes);
                if(f.localVars.size() > 0) {
                    uint size = 0;
                    vector<byte> types;
                    for(LocalVar var : f.localVars) {
                        //TEST: I dont push var.id in bytes - this may cause problems in VM!
                        size += sizeOf(var.type);
                        types.push_back((byte)var.type);
                    }
                    pushAddr(size, bytes);
                    bytes.insert(bytes.end(), types.begin(), types.end());
                }
                pushAddr(f.bytecode.size(), bytes);
                bytes.insert(bytes.end(), f.bytecode.begin(), f.bytecode.end());
                //bytes.push_back(f.bytecode.size());
            }

            //bytes.push_back(0xDD);
        }
    }
};

class StringSegment : public Segment {
public:
    vector<byte> utf8;

    StringSegment() {
        type = Segment::Strings;
        name = "strings";
    }

    virtual uint size() {
        return utf8.size(); //FIXME!!!!!
    }

    virtual void appendTo(vector<byte> &bytes) {
        bytes.insert(bytes.end(), utf8.begin(), utf8.end());
    }

    virtual vector<byte> getBytes() {
#ifdef DEBUG
        cout << "Strings size: " << utf8.size() << endl;
#endif
        return utf8;
    }
};

/*class OffsetSegment : public Segment {
public:
    vector<addr_t> strings;

    OffsetSegment() {
        type = Segment::Offsets;
        name = "offsets";
    }

    virtual uint size() {
        return strings.size()*4; //FIXME!!!!!
    }

    virtual void appendTo(vector<byte> &bytes) {
        //bytes.insert(bytes.end(), strings.begin(), strings.end());
    }

    virtual vector<byte> getBytes() {
        vector<byte> bytes;
        bytes.reserve(strings.size()*4);
        for(addr_t a: strings) {
            pushAddr(a, bytes);
        }
        return bytes;
    }
};

class ImportSegment : public Segment {
public:
    vector<ImportedModule> imports;
    //vector<ImportedFunction> importedFuncs;
    vector<byte> bytes;

    ImportSegment() {
        type = Segment::Imports;
        name = "imports";
    }

    virtual uint size() {
        if(bytes.empty())
            generateBytes();
        return bytes.size();
    }

    virtual void appendTo(vector<byte> &bytes) {
        //FIXME!!!!!
    }

    virtual vector<byte> getBytes() {
        if(bytes.empty())
            generateBytes();
        return bytes;
    }
private:
    void generateBytes() {
        for(ImportedModule im : imports) {
            for(char ch : im.name)
                bytes.push_back((byte)ch);
            bytes.push_back('\0');
            pushInt(im.funcPtrs.size()*4, bytes);
            for(addr_t a : im.funcPtrs)
                pushAddr(a, bytes);
        }
        union {
            addr_t a;
            byte bytes[4];
        } a2b;
        a2b.a = bytes.size() + 4;
        bytes.insert(bytes.begin(), a2b.bytes[3]);
        bytes.insert(bytes.begin(), a2b.bytes[2]);
        bytes.insert(bytes.begin(), a2b.bytes[1]);
        bytes.insert(bytes.begin(), a2b.bytes[0]);
        for(ImportedFunction ifunc : importedFuncs) {
            for(char ch: ifunc.func.sign)
                bytes.push_back((byte)ch);
            bytes.push_back('\0');
            for(char ch : ifunc.func.argStr) bytes.push_back(ch);
                bytes.push_back('\0');
            pushAddr(ifunc.addrs.size()*4, bytes);
            for(addr_t a : ifunc.addrs)
                pushAddr(a, bytes);
        }
    }
};*/

class Module
{
    string name = "";
    MetaSegment metaSeg;
    GlobalsSegment globalsSeg;
    FunctionSegment functionSeg;
    //ImportSegment importSeg;
    StringSegment stringSeg;
    //OffsetSegment offsetSeg;
    vector<Segment*> userSegments;
    ModuleFlags mflags;

    friend class Assembler;
public:
    Module();

    void AddGlobal(GlobalVar var);
    void AddFunction(const Function &fun);
    vector<Segment *> AllSegments();
    void ImportIfNew(string module, string sign, vector<Type> &args);

    void Import(string mod);

    string GetName();
    void SetName(string name);
};

#endif // MODULE_H





