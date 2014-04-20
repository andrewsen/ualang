/*
    uac -- bilexical compiler. A part of bilexical programming language gramework
    Copyright (C) 2013-2014 Andrew Senko.

    This file is part of uac.

    uac is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    uac is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with uac.  If not, see <http://www.gnu.org/licenses/>.
*/

/*  Written by Andrew Senko <andrewsen@yandex.ru>. */


#ifndef PARSER_COMMONS
#define PARSER_COMMONS

#ifndef WIN32
#define __int64 long long
#endif
#define DEBUG

#include <vector>
#include <stack>
#include <map>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <typeinfo>
#include "asm_keywords.h"
#include "Module.h"
#undef EOF

//#define true 0
//#define false 1
//#define ifstream8 ifstream
//#define ofstream8 ofstream
//#define ifstream wifstream
//#define ofstream wofstream
//#define cout wcout
//#define cin wcin

using namespace std;

typedef unsigned int dword;

enum class CodeLocale {
    Default = 0, English = 0, Ukrainian, Latin
};

union memory_unit {
	double dword;
	__int64 reg;
	int word;
	unsigned int uword;
	short int hword;
	bool b;
	byte _byte;
	byte bytes [9];
};

union command_unit {
	short int hword;
	byte bytes [2];
};

class Token {
    string::iterator ptr, prevIndex;
	int tokenIndex;
	string token, curToken;
	bool firstTime;
public:
	enum TokenType {
		DIRECTIVE, KEYWORD, DIGIT, STRING, IDENTIFIER, UNKNOWN, DELIMETER, EOF, COMMENT
	};
	
	Token(string source);

	string GetCurrentToken();
	string GetNextToken();
	void PushBack();
	int GetTokenIndex();
	TokenType GetTokenType();
	string GetTail();

	~Token() {
		//delete [] ptr;
	}
private:
	string getNexToken_priv();
	TokenType type;
};

struct CompilerItem {
	unsigned int addr;
	PODTypes type;
	string name;
	string returnType;
    bool _private;
};

struct ProjectItem {
	string key;
	string value;
};

class Exception {
protected:
    string message;
    string file;
	int line;
public:
	Exception () {}
	
    Exception(string  _message) {
        //this->message = new char [strlen(_message)];
        this->message = _message;
        this->file = "";
		line = -1;
	}
	
    Exception (string  _message, string  _file) {
        //this->message = new char [strlen(_message) + 1];
        //this->file = new char [strlen(_file) + 1];
        this->message = _message;
        this->file = _file;
		this->line = -1;
	}
	
    Exception (string  _message, string  _file, int line) {
        //this->message = new char [strlen(_message) + 1];
        //this->file = new char [strlen(_file) + 1];
        this->message = _message;
        this->file = _file;
		this->line = line;
		//cout << "DEBUG EXCEPTION: " << this->message << " |DEBUG| " << this->file<< endl;
	}
	

	
	~Exception() {
		//if(message != NULL) delete [] message;
		
		//f(file != NULL) delete [] file;
	}
	
    virtual string  getMessage() {
		return message;
	}
	
    virtual string  getSource() {
		return file;
	}
	
	virtual int getLine() {
		return line;
	}
protected:
    virtual void setMessage(string  mes) {
        message = mes;
        //strcpy(message, mes);
	}
	
    virtual void setFile(string  f) {
        //strcpy(file, f);
        file = f;
	}
	
	virtual void setLine(int l) {
		line = l;
	}
};

class CompilerException : public Exception {
public:
    CompilerException(string  _message)  :  Exception(_message) {
		//wcscpy(this->message, message);
		//file = "";
		//line = -1;
        //this->message = new char [strlen(_message)];
        //this->file = new char [strlen(_file)];
	}
	
    CompilerException (string  _message, string  _file) : Exception(_message, _file, -1)
	{	}
	
    CompilerException (string  _message, string  _file, int _line) : Exception(_message, _file, _line)
	{	}

    //CompilerException(string  _message)  :  Exception(_message) {
	//}
	//
    //CompilerException (string  _message, string  _file) : Exception(_message, _file, -1)
	//{	}
	//
    //CompilerException (string  _message, string  _file, int _line) : Exception(_message, _file, _line)
	//{	}
};

class LinkerException : public Exception {
public:
    LinkerException(string  _message) : Exception(_message) {	}
	
    LinkerException (string  _message, string  _file) : Exception(_message, _file, -1){	}
	
    LinkerException (string  _message, string  _file, int _line) : Exception(_message, _file, _line) {	}
	//
    //LinkerException(string  _message) : Exception(_message) {	}
	//
    //LinkerException (string  _message, string  _file) : Exception(_message, _file, -1){	}
	//
    //LinkerException (string  _message, string  _file, int _line) : Exception(_message, _file, _line) {	}
};

struct Array {
    int size;
    PODTypes type;
    vector<byte> items;
};



class MetaItem {
    PODTypes valType;
    int ival;
    string sval, key;
public:
    MetaItem (string key, string value) : key(key), sval(value) {
        if(value == "")
            valType = PODTypes::_NULL;
    }
    MetaItem (string key, int value) : key(key), ival(value), valType(PODTypes::WORD) {}

    string GetKey() { return key; }

    PODTypes GetType() {return valType; }

    int GetIntValue() { return ival; }
    string GetStringValue() { return sval; }

    void SetIntValue(int val) {
        ival = val;
        valType = PODTypes::WORD;
    }
    void SetStringValue(string val) {
        sval = val;
        if(val == "")
            valType = PODTypes::_NULL;
        else valType = PODTypes::STRING;
    }
};

class Segment {
public:
    union Flags{
        unsigned type : 1;
        unsigned load_on_init : 1;
        unsigned reserved : 6;
        byte val = 0;
        Flags(byte v=0) : val(v) {}
    };
protected:

    enum Type : unsigned char {Binary = 0, Meta = 1};

    string name;
    Type type;
    vector<unsigned char> data;
    Flags flags;
public:

    explicit Segment(string name, Flags f=0) {
        this->name = name;
        type = Binary;
        flags = f;
        flags.type = (byte)Binary;
    }

    Segment(string name, bool i) {
        this->name = name;
        type = Binary;
        flags.load_on_init = i;
        flags.type = (byte)Binary;
    }

    void AddData(vector<unsigned char> d) {
        data.insert(data.end(), d.begin(), d.end());
    }
    void AddData(byte b) {
        data.push_back(b);
    }


    virtual string GetName() { return name; }
    virtual int GetSize() { return data.size(); }
    virtual vector<unsigned char> &GetData() { return data; }
    virtual Type GetType() { return type; }
    virtual Flags GetFlags() { return flags; }
};

class MetaSegment : public Segment{
    vector<MetaItem> items;

    void compile() {
        data.clear();
        for(MetaItem item : items) {
            for(int i = 0; i < item.GetKey().size(); ++i)
                data.push_back(item.GetKey()[i]);
            data.push_back(0);
            data.push_back((unsigned char)item.GetType());
            if(item.GetType() == PODTypes::STRING) {
                for(int t = 0; t < item.GetStringValue().size(); ++t)
                    data.push_back(item.GetStringValue()[t]);
                data.push_back(0);
            }
            else if(item.GetType() == PODTypes::WORD) {
                union {
                    unsigned char bytes[4];
                    int val;
                } transformer;

                transformer.val = item.GetIntValue();
                data.push_back(transformer.bytes[0]);
                data.push_back(transformer.bytes[1]);
                data.push_back(transformer.bytes[2]);
                data.push_back(transformer.bytes[3]);
            }
            //else if(item.GetType() == PODTypes::_NULL)
                //cout << "Metaitem " << item.GetKey() << " has null type" << endl;
        }
        //cout << "Meta segment '" << this->name << "' compiled! data.size() = " << data.size() << endl;
    }

public:
    explicit MetaSegment(string name, Flags flags=0) : Segment(name, flags) {
        //this->name = name;
        type = Meta;
        flags.type = (byte)Meta;
    }

    MetaSegment(string name, bool l) : Segment(name, l) {
        //this->name = name;
        type = Meta;
        flags.type = (byte)Meta;
    }

    void AddMeta(MetaItem i) {
        for(auto iter = items.begin(); iter != items.end(); ++iter) {
            if(iter->GetKey() == i.GetKey()) {
                *iter = i;
                return;
            }
        }
        items.push_back(i);
    }

    void AddMeta(string key, string val) {
        for(auto iter = items.begin(); iter != items.end(); ++iter) {
            if(iter->GetKey() == key) {
                *iter = MetaItem(key, val);
                return;
            }
        }
        items.push_back(MetaItem(key, val));
    }

    void AddMeta(string key, int val) {
        for(auto iter = items.begin(); iter != items.end(); ++iter) {
            if(iter->GetKey() == key) {
                *iter = MetaItem(key, val);
                return;
            }
        }
        items.push_back(MetaItem(key, val));
    }

    void AddMeta(string key) {
        for(auto iter = items.begin(); iter != items.end(); ++iter) {
            if(iter->GetKey() == key) {
                *iter = MetaItem(key, "");
                return;
            }
        }
        items.push_back(MetaItem(key, ""));
    }

    void RemoveMeta(string key) {
        for(auto iter = items.begin(); iter != items.end(); ++iter) {
            if(iter->GetKey() == key) items.erase(iter);
        }
    }

    MetaItem GetMeta(string key) {
        for(auto iter = items.begin(); iter != items.end(); ++iter) {
            if(iter->GetKey() == key) return *iter;
        }
    }

    MetaItem GetByIndex(int idx) {
        return items[idx];
    }

    virtual int GetSize() override { compile(); return data.size(); }
    virtual vector<unsigned char> &GetData() override
    {
        //cout << "Getting metadata\n";
        compile();
        return data;
    }

};

//extern memory_unit vr1, vr2, vr3, vr4, vr5, vri, vrl, vrs, vrflags;
extern const int COMPILER_VERSION;
extern bool isOutSpecified;

extern CompilerItem entryFunc;
extern string outName;
extern string moduleName;
extern _flags modFlags;

extern map<string , unsigned int> globals;  //*COMPILABLE
extern vector<byte> commands;			//COMPILABLE
extern vector<int> calls;						//NOT COMPILABLE
extern vector<int> jumps;						//NOT COMPILABLE
extern vector<int> str_stack_table;				//COMPILABLE
extern vector<memory_unit> program_stack;		//COMPILABLE
extern vector<CompilerItem> cstack;			//NOT COMPILABLE
extern vector<CompilerItem> procstack;			//*COMPILABLE
extern vector<CompilerItem> jmpstack;			//NOT COMPILABLE
extern vector<Module> imports;
//extern vector<MetaItem> metadata;
extern vector<string> namespaces;
extern vector<string> current_ns;
extern vector<string> str_stack;				//COMPILABLE
extern vector<Array> arrays;
extern vector<string> proj_files;
extern MetaSegment meta;


void parserEntry(string file, bool isProj = false);

bool IsCharAlpha(int wch);

bool is_digits_only (const char * str);

bool endsWith(string s, const string &str);
bool endsWith(string s, const string &str, bool ignoreCase);

string trim(string str);

void predefine(string & str);

void precompile(string &str, bool isProject = false);

void link();

void clearCompiler();

void write();
#endif
