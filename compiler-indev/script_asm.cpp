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

// #include "stdafx.h"
#define POSIX
#include <iostream>
#include "parser_common.h"
#include <limits>

using namespace std;

CodeLocale curLocale = CodeLocale::Ukrainian;

string localize(string key);
string localize_UA(string key);
string localize_LA(string key);

string unlocalize(string key);
string unlocalize_UA(string key);
string unlocalize_LA(string key);

void pushC1arg(OpCodes w, Token &token);
void pushC2args(OpCodes w, Token &token, bool oneArg=false);
void defineMacro(string macro, string val);

vector<byte> readInStack(string &t1, Token &token, bool isArray);

OpCodes get_command (const string &str);
OpCodes get_command_UA (const string &str);
OpCodes get_command_LA (const string &str);

OpCodes get_register (const string &str);
OpCodes get_register_UA (const string &str);
OpCodes get_register_LA (const string &str);

bool isRegister(const string &str);
bool isMacro(const string &str);

bool containsCEntry(const string &Key);
CompilerItem findCEntry(const string &Key);\

bool containsFCEntry(const string &Key);
CompilerItem findFCEntry(const string &Key);

PODTypes getType (string str);
PODTypes stringToType(const string &str);

void pushJump (OpCodes com, Token &token);
bool containsJCEntry(const string &Key);
CompilerItem findJCEntry(const string &Key);

bool isValid1arg(OpCodes com);

bool isPODType(string type, bool _void=false);

bool isFullName(const string & str);

string getCurrentNS();
string getFullName(Token & token);
string getFuncArgs (Token &token);

void addFileToPath(string filename);

template <typename Ty> void define(PODTypes pod, Token &token);

//bool isStatic = false;
bool ns_opened = false;
bool isIdent = false;
bool fun_open = false;

const int COMPILER_VERSION = 0x1;
int brace = 0;
int entryPtr = -1;
int line = 0;
string sourceFile;
string bindingLib;

CompilerItem entryFunc;

map <string, unsigned int >globals;
map <string, string > macros;
vector < int >str_stack_table;
vector < int >calls;
vector < int >jumps;
vector < byte >commands;
vector < memory_unit > program_stack;
vector < CompilerItem > procstack;	// NOT COMPILABLE
vector < CompilerItem > jmpstack;	// NOT COMPILABLE
vector < CompilerItem > cstack;
vector < string > str_stack;
vector<Array> arrays;
extern vector<unsigned int>real_addr_table;
extern vector<unsigned int>real_array_table;
extern vector<unsigned int>real_string_table;

void clearCompiler() {
    brace = 0;
    entryPtr = -1;
    line = 0;
    sourceFile = "";
    bindingLib = "";

    macros.clear();
    real_addr_table.clear();
    real_array_table.clear();
    real_string_table.clear();
}

template <> void define <string> (PODTypes pod, Token &token) {
	//numeric_limits<int>::max()
	string identifier;
    Array a;
    bool isArray = false;
    //if (getCurrentNS() == "")
    //{
		// Token tok = token.GetTail();
		// identifier = getFullName(tok, S(""));

    identifier = token.GetNextToken();
    //cout << "IDENTIFIER " << identifier << endl;
    if (identifier == "[") {
        isArray = true;
        a.type = PODTypes::STRING;
        if(token.GetNextToken() == "]") {
            a.size = -1;
        }
        else {
            a.size = atoi(token.GetCurrentToken().c_str()) * sizeof(uint);
            if(a.size <= 0)
                throw CompilerException("Розмір масиву має бути більшим за 0!", sourceFile, line);
            if(token.GetNextToken() != "]") throw CompilerException("']' очікувалась у визначенні масиву!", sourceFile, line);
        }
        identifier = token.GetNextToken();
    }
    //}
    //else
    //	identifier = getCurrentNS() + "::" + token.GetNextToken();
	// cout << "\nType '" << type << "\'\nIdentifier \'" <<
	// identifier << "'\n";
	if (token.GetTokenType() != Token::IDENTIFIER)
        // throw (S("Поганий ідентифікатор: ") + identifier).ToCString();
            throw CompilerException("Поганий ідентифікатор " + identifier,
			sourceFile, line);
	if (containsCEntry(identifier) || containsFCEntry(identifier)
		|| containsJCEntry(identifier))
        throw CompilerException("Ідентифікатор " + identifier + " вже визначений", sourceFile, line);

    if(token.GetNextToken() != ":" && token.GetTokenType() != Token::EOF)
        throw CompilerException(": очікувалось!", sourceFile, line);

	string val = token.GetNextToken();
	Token::TokenType t = token.GetTokenType();
	if (t != Token::STRING && t != Token::EOF
		&& t != Token::COMMENT)
        if(!isArray)
            throw CompilerException(string("Лексема не є рядком: ") +
		val, sourceFile, line);
    if (t != Token::STRING)
        val = "";
    if(isArray && t != Token::EOF) {
        //cout << "In \t\t\tArray" << endl;
        int counter = 0;
        for(int i = 0; val != "}"; i++) {
            val = token.GetNextToken();
            //if (val == "}") break;

            memory_unit u;

            if(token.GetTokenType() != Token::STRING)
                throw CompilerException("Лексема не є рядком: " + val, sourceFile, line);
            str_stack.push_back(val);
            u.word = str_stack.size()-1;
            for (int i = 0; i < sizeof(int); i++)
                a.items.push_back(u.bytes[i]);
            ++counter;

            if (token.GetNextToken() == "}") break;
            if (token.GetCurrentToken() != ",")
                throw CompilerException("Очікувалась кома! Лексема: " + token.GetCurrentToken(), sourceFile, line);
        }
        if(a.size == -1) a.size = counter * sizeof(int);
        //cout << "Type of array " << identifier << " is string " << " size: " << a.size << endl;
        for (auto iter = a.items.begin(); iter != a.items.end(); ++iter) {
            //cout << (int)*iter << " ";
        }
    }
    if(a.size == -1) throw CompilerException ("В масиві не зазначений ні розмір ні список ініціалізації! ", sourceFile, line);
    memory_unit unit;
	unit.reg = 0;
	str_stack.push_back(val);
	unit.word = str_stack.size() - 1;
    unit.bytes[8] = (byte)PODTypes::STRING;
    //str_stack_table.push_back(val.length());
	CompilerItem entry;

    entry.type = PODTypes::STRING;
    if(isArray) {
        arrays.push_back(a);
        entry.addr = program_stack.size();//arrays.size() - 1;
        unit.uword = arrays.size() - 1;
        entry.type = PODTypes::POINTER;
        unit.bytes[8] = (byte)PODTypes::POINTER;
        program_stack.push_back(unit);
    }
    else {
        entry.addr = program_stack.size();
        program_stack.push_back(unit);
    }
    //cout << "String address: " << entry.addr << endl;
    //entry.addr = program_stack.size() - 1;
	entry.name = identifier;
	cstack.push_back(entry);
    globals.insert(pair < string, int >(identifier,program_stack.size() - 1));
}

void parserEntry(string file, bool isProj)
{

	//cout << "exception test" << endl;
	//throw CompilerException("Test exception!", "some file",  0);
	//cout << exc.getMessage();
	commands.push_back(0x0);
	sourceFile = file;
	if (!isProj)
	{
        addFileToPath(file);
        defineMacro("$SOURCE", file);
        defineMacro("$MODULE", moduleName);
        //defineMacro("$", moduleName);
        ifstream ifs(file);
		if (!ifs)
		{
			// cout << "Very, very bad file! :'(";
			// cin.get();
			throw Exception("Bad file (file not found)", file);
		}
		// line = 1;
		// loop = 1;
		while (!ifs.eof())
        {
            char buf[255];
            ifs.getline(buf, 255);
            string tmp = buf;
			//cout << "at parser" << endl;
            tmp = trim(tmp);
            while (endsWith(tmp, " \\"))
            {
                char b[255];
                ifs.getline(b, 255);
                string str = b;
                str = trim(str);
                tmp = tmp.erase(0, 1);
				tmp += str;
			}
			++line;
			//cout << tmp << endl;
			// cin.get();
			
            cout << "Компіляція: " << tmp << endl;
			precompile(tmp);
			// line++;
		}
		link();
		ifs.close();
		return;
	}
	else
	{
		for (string f:proj_files)
		{
            cout << "FILE: \"" << f << "" << endl;
			ifstream ifs;
            ifs.open(f);
			if (!ifs)
			{
                cout << "Very, very bad file! :'(" << endl;
				exit(1);
				// cin.get();
			}
			while (!ifs.eof())
            {
                char buf[255];
                ifs.getline(buf, 255);
                string tmp = buf;
                tmp = trim(tmp);
                while (endsWith(tmp, " \\"))
                {
                    char b[255];
                    ifs.getline(b, 255);
                    string str = b;
                    str = trim(str);
                    tmp = tmp.erase(0, 1);
					tmp += str;
				}
				predefine(tmp);
			}
			ifs.close();
		}
		cout << "Compilation" << endl;
		for (string f:proj_files)
		{
			cout << "FILE: " << f << endl;
			ifstream ifs;
            ifs.open(f);
			if (!ifs)
			{
				cout << "Very, very bad file! :'(";
				// cin.get();
				exit(1);
			}
			while (!ifs.eof())
            {
                char buf[255];
                ifs.getline(buf, 255);
                string tmp = buf;
                tmp = trim(tmp);
                while (endsWith(tmp, " \\"))
				{
                    char b[255];
                    ifs.getline(b, 255);
                    string str = b;
                    str = trim(str);
                    tmp = tmp.erase(0, 1);
					tmp += str;
				}
				++line;
				precompile(tmp, true);
				// line++;
			}
			ifs.close();
		}
		link();
	}
}

void precompile(string & str, bool isProject)
{
    bool isNative = false, isPrivate = modFlags.canExecute ? true : false;
    if (str == "" || str.c_str() == nullptr)
		return;
	// cout << "compilling '" << str << "'\n";
	Token token = str;
	string str_com = token.GetNextToken();
	Token::TokenType type = token.GetTokenType();
	if (str_com == ";" || type == Token::COMMENT)
	{
		// cout << endl << "DEV: comment" << endl;
		return;
	}
	if (type == Token::TokenType::EOF)
	{
		// cout << endl << "DEV: eof" << endl;
		return;
	}

	if (str_com == "{")
	{
		++brace;
		return;
	}
	if (str_com == "}")
	{
		--brace;
		if (brace < 0)
            throw CompilerException("Зайва '}'!", sourceFile, line);
		current_ns.pop_back();
		return;
	}

    if (str_com == localize("native"))
	{
		isNative = true;
		str_com = token.GetNextToken();
        if (str_com != localize("prot"))
            throw CompilerException("Неочікувана лексема 'нативний'!", sourceFile, line);
	}

    if (type == Token::DIRECTIVE && str_com == localize(".public")) {
        isPrivate = false;
        if(token.GetNextToken() != localize("prot"))
            throw CompilerException("Очікувався прототип", sourceFile, line);
        str_com = token.GetCurrentToken();
    }
    if (type == Token::DIRECTIVE && str_com == localize(".private")) {
        isPrivate = true;
        if(token.GetNextToken() != localize("prot"))
            throw CompilerException("Очікувався прототип", sourceFile, line);
        str_com = token.GetCurrentToken();
    }
    else if (type == Token::DIRECTIVE)
	{
		// cout << endl << "DEV: starts with '.'" << endl;
        string dir = str_com;
        if (dir == ".eng") {
            curLocale = CodeLocale::English;
            return;
        }
        else if (dir == ".укр") {
            curLocale = CodeLocale::Ukrainian;
        }
        else if (dir == ".lat") {
            curLocale = CodeLocale::Latin;
            return;
        }
        else if (dir == localize(".entry"))
		{
			if (entryFunc.name != "")
                throw CompilerException("Вхідна функція вже визначена!",
				sourceFile, line);
			//string s;
            string name = token.GetNextToken();//getFullName(token);
			if(token.GetNextToken() != "(") 
                throw CompilerException("'(' очікувалось! " + token.GetCurrentToken(), sourceFile, line);
			name += "(";
			name += getFuncArgs(token);
			if(token.GetNextToken() != ")") 
                throw CompilerException("')' очікувалось! Token: '" + token.GetCurrentToken() + "'", sourceFile, line);
			name += ")";
			
            cout << "Вхідна функція" << name << endl;
			entryFunc.name = name;
			//cout << "at .ENTRY" << endl;
			// if(token.GetTokenType() == Token::IDENTIFIER)
			// entryFunc.name = name;
			// else
			// throw "Illegal function name " + name;
            return;
		}
        else if (dir == localize(".binding"))
		{
			modFlags.isBinding = true;
			modFlags.canExecute = false;
			bindingLib = token.GetNextToken();
			if(token.GetTokenType() != Token::STRING)
                throw CompilerException("Очікувався рядок з назвою бібліотеки", sourceFile, line);
			str_stack.push_back(bindingLib);
            return;
		}
        //if (dir == localize(".static"))
        //	isStatic = true;
        else if (dir == localize(".type")) {
            if(token.GetNextToken() == localize("library")) {
				modFlags.canExecute = false;
                isPrivate = false;
            }
            else if(token.GetCurrentToken() == localize("executable")){
                modFlags.canExecute = true;
                isPrivate = true;
            }
            else throw CompilerException("Невідоме значення директиви .тип: " + token.GetCurrentToken(), sourceFile, line);
            return;
        }
			
        else if (dir == localize(".load"))
		{
			if(modFlags.isBinding)
                throw CompilerException("Binding library mustn't contains everything except prototypes and namespaces!", sourceFile, line);
			string modName;
			string tok = token.GetNextToken();
			bool addExt = true;
			if(token.GetTokenType() == Token::STRING)
				modName = tok;
			else if(token.GetTokenType() == Token::IDENTIFIER)
			{
				modName = tok;
				token.GetNextToken();
                while (token.GetTokenType() != Token::EOF && token.GetTokenType() != Token::COMMENT)
				{
					if(token.GetCurrentToken() != ".") 
                        throw CompilerException(".(крапка) очікувалась! " + token.GetCurrentToken(), sourceFile, line);
					modName += ".";
					modName += token.GetNextToken();
					if(token.GetTokenType() != Token::IDENTIFIER) 
                        throw CompilerException("Поганий ідентифікатор: " + token.GetCurrentToken(), sourceFile, line);
					token.GetNextToken();
				}
				if(token.GetCurrentToken() == ".") 
                    throw CompilerException("Неочікувана крапка!", sourceFile, line);
				addExt = false;
			}
            //if(modName == "builtin") useBuiltin = true;
            /*
                prot void __builtin_call (string)
                fun void __builtin_call (string)
                    call_native [vsp],
                end
            */
			try {
				Module import(modName, addExt);
				imports.push_back(import);
			}
			catch (ModuleException e) {
				//cerr << "Module: " << e.GetName();
                cerr << e.GetReason() << endl;
                cerr << e.GetExtra();
			}

            return;
			//.load some.module
			//.load "dir/other-mod.slm"

			//.module:
			//some.module\0dir/other-mod.slm\0
		}
        //if (dir == localize(".end"))
        //{
        //	if (isStatic)
        //		isStatic = false;
        //}
        //cout << "at ret" << endl;
	}
	// cout << "Command '" << str_com << "'\n";
	OpCodes com = get_command(str_com);
	string label = "";
	if (com == OpCodes::UNKNOWN && type != Token::DIRECTIVE
		&& (label = token.GetNextToken()) != ":")
	{
        throw CompilerException("Невідома лексема '" + str_com + "'", sourceFile,
			line);
		return;
	}
	else if (label == ":")
	{
        // if(token.GetTokenType() != Token::IDENTIFIER) throw (string("Bad
		// Identifier: ") + str_com).ToCString();
//		CompilerItem e = { 0, PODTypes::LABEL, str_com };
		if (!containsJCEntry(str_com))
		{
            CompilerItem c = { commands.size(), PODTypes::LABEL, str_com };
			jmpstack.push_back(c);
		}
		else
			for (int i = 0; i < jmpstack.size(); i++)
				if (jmpstack[i].name == str_com)
					jmpstack[i].addr = commands.size();
		return;
	}

	if(modFlags.isBinding && com != OpCodes::PROT && com != OpCodes::NAMESPACE) {
        throw CompilerException("Binding library mustn't contains everything except prototypes and namespaces!", sourceFile, line);
	}

#pragma region commands
	switch (com)
	{
	case OpCodes::ADD:
        pushC2args(com, token);
		break;
    case OpCodes::ALLOC:
        /// alloc <type>, <size> ; vra <- pointer
        pushC2args(com, token);
        /// mov [vsp-1], 5
        /// mov | stack reg vsp sub digit 0 0 0 1 | digit 5
        /// 0xAA 0xFF 0xCC 0xBB 0x11 0x22 0x00000001 0x22 0x00000005
        break;
	case OpCodes::AND:
		pushC2args(com, token);
		break;
		// case OpCodes::BAND:
		// pushC2args(com, token);
		// break;
		// case OpCodes::BTC:
		// break;
		// case OpCodes::BOR:
		// pushC2args(com, token);
		// break;
		// case OpCodes::BTS:
		// break;
		// case OpCodes::BXOR:
		// pushC2args(com, token);
		// break;
	case OpCodes::CALLE:
	case OpCodes::CALLNE:
	case OpCodes::CALL:
		{
			bool is_full_name = false;
			string name = token.GetNextToken();
			// if(token.GetNextToken() == ":");
			Token tok = token.GetTail();
            if (token.GetTokenType() != Token::IDENTIFIER)	// throw (string("Bad
				// Identifier: ")
					// +
						// name).ToCString();
                            throw CompilerException("Поганий ідентифікатор: " + name, sourceFile,
							line);
			if (tok.GetNextToken() == ":")
				if (tok.GetNextToken() == ":")
				{
					is_full_name = true;
					name += "::" + tok.GetNextToken();
					for (string s = tok.GetNextToken();
						tok.GetTokenType() == Token::IDENTIFIER || s == ":";
						s = tok.GetNextToken())
						name += s;
				}
				
			if(token.GetNextToken() != "(") 
                throw CompilerException("'(' очікувалось!", sourceFile, line);
			name += "(";
			name += getFuncArgs(token);
			if(token.GetNextToken() != ")") 
                throw CompilerException("')' очікувалось!", sourceFile, line);
			name += ")";

			// TODO:
            //if (!containsFCEntry(getCurrentNstring() + "::" + name))
            //	if (!containsFCEntry(name))	// throw (string("Undefined function:
					// ") + name).ToCString();
			//			throw CompilerException("Undefined function: " + name,
			//			sourceFile, line);
			memory_unit u;
			int addr = -1;
			byte mod = 0;
			bool internal = true;
			for (int i = 0; i < procstack.size(); i++)
			{
				if (procstack[i].name == name)
					addr = i;
				if (procstack[i].name == getCurrentNS() + "::" + name)
					addr = i;
			}
            if (addr == -1) internal = false;
			for(int i = 0; i < imports.size(); i++) {
				//throw CompilerException("", sourceFile, line);
				auto funcs = imports[i].GetFunctions();
                for (auto t = 0; t < funcs.size(); t++) {
                    if(funcs[t].GetSignature() == name) {
                        addr = t;
                        mod = i + 1;
                    }
                    //cout << "SIGNATURE: " << funcs.size() << endl;
                }
			}

			if (addr == -1)
                throw CompilerException("Щось не так, можливо фуцкція " +
                name + " не визначена...", sourceFile, line);
            //cout << "\tCALL ADDR: " << addr << endl;
			u.word = addr;
			// vector<CompilerItem>::const_iterator it =
			// find(procstack.begin(), procstack.end(), findFCEntry(name));
			// size_t idx = -1;
			// if ( it!=procstack.end() ) {
			// idx = it - procstack.begin();
			// }
			commands.push_back((byte)com);
            //cout << "\tCALL MODULE: " << (int)mod << endl;
			// commands.push_back(u2);
			calls.push_back(commands.size());
			commands.push_back(mod);
			commands.push_back(u.bytes[0]);
            //cout << "\tCALL ADDR[0]: " << u.bytes[0] << endl;
			commands.push_back(u.bytes[1]);
            //cout << "\tCALL ADDR[1]: " << u.bytes[1] << endl;
			commands.push_back(u.bytes[2]);
			commands.push_back(u.bytes[3]);
		}
		break;
	case OpCodes::CMP:
		pushC2args(com, token);
		break;
	case OpCodes::DEC:
		pushC1arg(com, token);
		break;
	case OpCodes::DEF:
		{
			if (isProject)
				return;
			// printf("def\n");
            if (!isProject)
			{
				string type = token.GetNextToken();
				
                if(type == localize("bool")) define<bool> (PODTypes::BOOL, token);
                else if(type == localize("byte")) define<byte> (PODTypes::BYTE, token);
                else if(type == localize("short")) define<short int> (PODTypes::HWORD, token);
                else if(type == localize("int")) define<int> (PODTypes::WORD, token);
                else if(type == localize("double")) define<double> (PODTypes::DWORD, token);
                else if(type == localize("int64")) define<__int64> (PODTypes::W64, token);
                else if(type == localize("string")) define<string>(PODTypes::STRING, token);
                else throw CompilerException("Поганий тип " + type, sourceFile, line);
			}
		}
		break;
	case OpCodes::DIV:
		pushC2args(com, token);
		break;
    case OpCodes::END:
        if(!fun_open)
            throw CompilerException("Зайва команда 'кінець (end)'", sourceFile, line);
        commands.push_back((byte)OpCodes::RET);
        fun_open = false;
		break;
		// case OpCodes::EXC:
		// break;
    case OpCodes::HALT:
		commands.push_back((byte)com);
		break;
    case OpCodes::FREE:
        pushC1arg(com, token);
        break;
	case OpCodes::FUN:
		{
            if(fun_open)
                throw CompilerException("Визначення функції всередині іншої неможливе!", sourceFile, line);
            fun_open = true;
			string type = token.GetNextToken();
			if(!isPODType(type, true)) throw 
                CompilerException ("Невідомий тип: " + type, sourceFile, line);

			string name = token.GetNextToken();

			if (token.GetTokenType() != Token::IDENTIFIER)
                throw CompilerException(string("Поганий ідентифікатор: ") + name,
				sourceFile, line);
			
			if(token.GetNextToken() != "(") 
                throw CompilerException("'(' очікувалось!", sourceFile, line);
			name += "(";
			name += getFuncArgs(token);
			if(token.GetNextToken() != ")") 
                throw CompilerException("')' очікувалось! Лексема: " + token.GetCurrentToken(), sourceFile, line);
			name += ")";

			for (int i = 0; i < procstack.size(); i++)
			{
				if (procstack[i].name == getCurrentNS() + "::" + name)
				{
					procstack[i].addr = commands.size();
					if (entryFunc.name == getCurrentNS() + "::" + name)
						entryFunc.addr = commands.size();
					return;
				}
				else if (getCurrentNS() == "" && procstack[i].name == name)
				{
					procstack[i].addr = commands.size();
					if (entryFunc.name == name)
						entryFunc.addr = commands.size();
					return;
				}
			}
			throw
                CompilerException(string
                ("Функція не має прототипа\n\t(функциї повинні мати прототип): ")
				+ name, sourceFile, line);
		}
		break;
	case OpCodes::INC:
		pushC1arg(com, token);
		break;
	case OpCodes::JMP:
		pushJump(com, token);
		break;
	case OpCodes::JE:
		pushJump(com, token);
		break;
	case OpCodes::JNE:
		pushJump(com, token);
		break;
	case OpCodes::JG:
		pushJump(com, token);
		break;
	case OpCodes::JL:
		pushJump(com, token);
		break;
	case OpCodes::JGE:
		pushJump(com, token);
		break;
	case OpCodes::JLE:
		pushJump(com, token);
		break;
	case OpCodes::MOV:
		pushC2args(com, token);
		break;
	case OpCodes::MUL:
		pushC2args(com, token);
		break;
	case OpCodes::NAMESPACE:
		{
			// namespaces.push_back(token.GetNextToken());
			string name = token.GetNextToken();
			if (token.GetTokenType() != Token::IDENTIFIER)
                throw CompilerException(string("Поганий ідентифікатор: ") + name,
				sourceFile, line);
			current_ns.push_back(name);
			namespaces.push_back(getCurrentNS());
			if (token.GetNextToken() == "{")
				++brace;
			else
                throw CompilerException("'{' очікувалось!", sourceFile, line);
		}
		break;
	case OpCodes::NEG:
		pushC1arg(com, token);
		break;
	case OpCodes::NOP:
		commands.push_back((byte)com);
		break;
	case OpCodes::NOT:
		pushC1arg(com, token);
		break;
	case OpCodes::OR:
		pushC2args(com, token);
		break;
	case OpCodes::POP:
		{
            pushC1arg(com, token);
            /*
            commands.push_back((byte)com);
			string indx = token.GetNextToken();
			if (token.GetTokenType() != Token::DIGIT)
				commands.push_back(1);
			else
			{
                int i = atoi(indx.c_str());
				if (i >= 0xFF)
					throw
                    CompilerException("Pop: Number is bigger then 0xFF: " + indx, sourceFile, line);
				else
					if (i < 0x00)
                        throw CompilerException(string("Pop: Number is smaller then 0x00: ")
						+ indx, sourceFile, line);
					else
						commands.push_back((byte)i);
            }
            */
		}
		break;
	case OpCodes::PROT:
		{
			// prot myfunc(int, string, byte) -> void
			// fun f2(void) -> string
			if (isProject)
				return;
			// cout << "PROT: " << str_com << endl;
			string libName = "";
			if(isNative) {
				if(token.GetNextToken() != "[") 
                    throw CompilerException ("'[' очікувалось! " + token.GetCurrentToken(), sourceFile, line);
				libName = token.GetNextToken();
				if(token.GetTokenType() != Token::STRING)
                    throw CompilerException ("Очікувалась назва бібліотеки", sourceFile, line);
				if(token.GetNextToken() != "]") 
                    throw CompilerException ("']' очікувалось!", sourceFile, line);
				//commands.push_back((byte)OpCodes::CALL_NATIVE);
				
				str_stack.push_back(libName);
				//unsigned int saddr = str_stack.size() - 1;

				//natFunc.name = 
			}

            //if (!isStatic)
            //	throw
            //	CompilerException
            //    ("Prototypes can be only static!\n\t(must de defined in .static block)",
            //	sourceFile, line);
			// if(isProject) throw (S("Cannot define prototype at compilation
			// stage!")).ToCString();
            string type = token.GetNextToken();
			if(!isPODType(type, true)) throw 
                CompilerException ("Невідомий тип: " + type, sourceFile, line);

			string name;
			if (getCurrentNS() == "")
			{
				// if(isFullName(token.GetTail()))
				//auto s = S("");
                name = token.GetNextToken();
				//cout << "NAME: " << token.GetNextToken() << endl;
				// else
				// name = token.GetNextToken();
			}
			else
			{
				name = getCurrentNS() + "::" + token.GetNextToken();
				if (token.GetTokenType() != Token::IDENTIFIER)
                    throw CompilerException("Поганий ідентифікатор: " + name,
					sourceFile, line);
			}
			auto t = token.GetNextToken();
			if(t != "(") 
                throw CompilerException("'(' очікувалось! " + t + " :" + name, sourceFile, line);
			name += "(";
			name += getFuncArgs(token);
			if(token.GetNextToken() != ")") 
                throw CompilerException("')' очікувалось! Token: " + token.GetNextToken(), sourceFile, line);
			name += ")";

			if (containsFCEntry(name))
				throw
                CompilerException("Функція або мітка вже визначена: "	+ name, sourceFile, line);
            //cout << "Unlocalized type: " << unlocalize(type) << endl;
            CompilerItem e = { 0, PODTypes::FUNC, name, unlocalize(type) };
            e._private = isPrivate;
			if(isNative || modFlags.isBinding) {
				memory_unit u;
				if(isNative)
					u.uword = str_stack.size() - 1;
				else
					for(int i = 0; i < str_stack.size(); i++) {
						if(bindingLib == str_stack[i])
							u.uword = i;
					}
				e.addr = commands.size();
				commands.push_back((byte)OpCodes::CALL_NATIVE);
                real_string_table.push_back(commands.size());
				commands.push_back(u.bytes[0]);
				commands.push_back(u.bytes[1]);
				commands.push_back(u.bytes[2]);
				commands.push_back(u.bytes[3]);
				commands.push_back((byte)OpCodes::RET);
			}
			//cout << "After" << endl;
			procstack.push_back(e);
		}
		break;
	case OpCodes::PUSH:
		pushC1arg(com, token);
		break;
	case OpCodes::RET:
		commands.push_back((byte)com);
		break;
	case OpCodes::ROL:
		pushC2args(com, token);
		break;
	case OpCodes::ROR:
		pushC2args(com, token);
		break;
	case OpCodes::SET:
		break;
	case OpCodes::SHL:
		pushC2args(com, token);
		break;
	case OpCodes::SHR:
		pushC2args(com, token);
		break;
	case OpCodes::SUB:
		pushC2args(com, token);
		break;
    case OpCodes::STDIN:
        {
            auto tok = token.GetNextToken();
            if (tok == localize("double") || tok == localize("int") || tok == localize("short") || tok == localize("byte")
                || tok == localize("bool") || tok == localize("int64") || tok == localize("string"))
            {
                commands.push_back((byte)OpCodes::STDIN);
                commands.push_back((byte)PODTypes::_NULL);
                commands.push_back((byte)stringToType(tok));
            }
            else if(tok == localize("void")) {
                commands.push_back((byte)OpCodes::STDIN);
                commands.push_back((byte)PODTypes::_NULL);
                commands.push_back((byte)PODTypes::_NULL);
            }
            else {
                token.PushBack();
                pushC1arg(com, token);
            }
        }
		break;
	case OpCodes::STDOUT:
		pushC1arg(com, token);
		break;
	case OpCodes::XCH:
		pushC2args(com, token);
		break;
	case OpCodes::XOR:
		pushC2args(com, token);
		break;
	default:
		break;
	}
#pragma endregion
}

void predefine(string & str)
{
	bool isNative;
    if (str == "")
		return;
	// cout << "compilling '" << str << "'\n";
	Token token = str;
	string str_com = token.GetNextToken();
	Token::TokenType type = token.GetTokenType();
	if (str_com == ";" || type == Token::COMMENT)
	{
		// cout << endl << "DEV: comment" << endl;
		return;
	}
	if (type == Token::TokenType::EOF)
	{
		// cout << endl << "DEV: eof" << endl;
		return;
	}
	
    if (str_com == localize("native"))
	{
		isNative = true;
		str_com = token.GetNextToken();
        if (str_com != localize("prot"))
            throw CompilerException("Неочікувана лексема 'нативний'!", sourceFile, line);
	}

	if (type == Token::DIRECTIVE)
	{
		// cout << endl << "DEV: starts with '.'" << endl;
        string dir = str_com;
        //if (dir == localize(".static"))
        //	isStatic = true;
        //if (dir == localize(".end"))
        //{
        //	if (isStatic)
        //		isStatic = false;
        //}
	}
	// cout << "Command '" << str_com << "'\n";
	OpCodes com = get_command(str_com);
#pragma region commands
	switch (com)
	{
	case OpCodes::DEF:
		{
#pragma region DEF
            // printf("def\n");
				string type = token.GetNextToken();
				
                if(type == localize("bool")) define<bool> (PODTypes::BOOL, token);
                else if(type == localize("byte")) define<byte> (PODTypes::BYTE, token);
                else if(type == localize("short")) define<short int> (PODTypes::HWORD, token);
                else if(type == localize("int")) define<int> (PODTypes::WORD, token);
                else if(type == localize("double")) define<double> (PODTypes::DWORD, token);
                else if(type == localize("int64")) define<__int64> (PODTypes::W64, token);
                else if(type == localize("string")) define<string>(PODTypes::STRING, token);
                else throw CompilerException("Поганий тип " + type, sourceFile, line);
#pragma endregion
		}
		break;

	case OpCodes::PROT:
		{
			string libName = "";
			if(isNative) {
				if(token.GetNextToken() != "[") 
                    throw CompilerException ("'[' очікувалось! " + token.GetCurrentToken(), sourceFile, line);
				libName = token.GetNextToken();
				if(token.GetTokenType() != Token::STRING)
                    throw CompilerException ("Очікувалась назва бібліотеки!", sourceFile, line);
				if(token.GetNextToken() != "]") 
                    throw CompilerException ("']' очікувалось!", sourceFile, line);
				//commands.push_back((byte)OpCodes::CALL_NATIVE);
				
				str_stack.push_back(libName);
				//unsigned int saddr = str_stack.size() - 1;

				//natFunc.name = 
			}

            //if (!isStatic)
            //	throw
            //	CompilerException
            //    ("Prototypes can be only static!\n\t(must de defined in .static block)",
            //	sourceFile, line);
			// if(isProject) throw (S("Cannot define prototype at compilation
			// stage!")).ToCString();
			string type = token.GetNextToken();
			if(!isPODType(type, true)) throw 
                CompilerException ("Невідомий тип: " + type, sourceFile, line);

			string name;
			if (getCurrentNS() == "")
			{
				// if(isFullName(token.GetTail()))
				//auto s = S("");
				name = getFullName(token);
				//cout << "NAME: " << token.GetNextToken() << endl;
				// else
				// name = token.GetNextToken();
			}
			else
			{
				name = getCurrentNS() + "::" + token.GetNextToken();
				if (token.GetTokenType() != Token::IDENTIFIER)
                    throw CompilerException("Поганий ідентифікатор: " + name,
					sourceFile, line);
			}
			auto t = token.GetNextToken();
			if(t != "(") 
                throw CompilerException("'(' очікувалось! " + t, sourceFile, line);
			name += "(";
			name += getFuncArgs(token);
			if(token.GetNextToken() != ")") 
                throw CompilerException("')' очікувалось! Лексема: " + token.GetCurrentToken(), sourceFile, line);
			name += ")";

			if (containsFCEntry(name))
				throw
                CompilerException("Функція або мітка вже визначена: "
				+ name, sourceFile, line);
            //cout << "Before" << endl;
            CompilerItem e = { 0, PODTypes::FUNC, name, type, false }; ////////////////////////////////////////////////////////////////////// <- FIXME
			if(isNative || modFlags.isBinding) {
				memory_unit u;
				if(isNative)
					u.uword = str_stack.size() - 1;
				else
					for(int i = 0; i < str_stack.size(); i++) {
						if(bindingLib == str_stack[i])
							u.uword = i;
					}
				e.addr = commands.size();
				commands.push_back((byte)OpCodes::CALL_NATIVE);
				commands.push_back(u.bytes[0]);
				commands.push_back(u.bytes[1]);
				commands.push_back(u.bytes[2]);
				commands.push_back(u.bytes[3]);
				commands.push_back((byte)OpCodes::RET);
			}
			//cout << "After" << endl;
			procstack.push_back(e);
		}
		break;
	default:
		break;
	}
#pragma endregion
	// if(is_digits_only(array[1]));
}

void defineMacro(string macro, string val) {
    if(macros.find(macro) == macros.end())
        macros.insert(pair<string,string>(macro, val));
    else
        macros.at(macro) = val;
}

bool is_digits_only(const char *str)
{
    //cout << "DIGIT: '" << str << "' ";
	if (str == 0)
		return false;
	bool hasDot = false, hasMin = false;
	if (str[0] == '.') {
        //cout << "str: '" << str << "' (dot) isn't a digit! line:" << line  << endl;
		return false;
	}
	if (str[0] == '-')
	{
		hasMin = true;
	}
	for (; *str != '\0'; str++)
	{
		if (!hasMin && *str == '-')
		{
            //cout << "str: '" << str << "' (-) isn't a digit! line:" << line  << endl;
			return false;
		}
		if (hasMin)
		{
			hasMin = false;
			// continue; 
		}
		if (*str == '.')
			if (hasDot) {
                //cout << "str: '" << str << "' (.) isn't a digit! line:" << line  << endl;
				return false;
			}
			else
				hasDot = true;
		else if (*str != '.')
			if (*str < '0' || *str > '9') {
                //cout << "str: '" << str << "' (inv char 0x" << hex << (int)*str <<dec << ") isn't a digit! line:" << line  << endl;
				return false;
			}
	}
	return true;
}

OpCodes get_register(const string & s)
{
    if(curLocale == CodeLocale::Latin) return get_register_LA(s);
    if(curLocale != CodeLocale::English ||
            (curLocale == CodeLocale::Default && CodeLocale::Default == CodeLocale::Ukrainian)) return get_register_UA(s);
    string str = s;
    if (str == "vr1")
        return OpCodes::VR1;
    else if (str == "vr2")
        return OpCodes::VR2;
    else if (str == "vr3")
        return OpCodes::VR3;
    else if (str == "vr4")
        return OpCodes::VR4;
    else if (str == "vr5")
        return OpCodes::VR5;
    else if (str == "vri")
        return OpCodes::VRI;
    else if (str == "vip")
        return OpCodes::VIP;
    else if (str == "vsp")
        return OpCodes::VSP;
    else if (str == "vra")
        return OpCodes::VRA;
    else if (str == "flags")
        return OpCodes::FLAGS;
    else
    {
        // cout << endl << "unknown register: " << str << endl;
        return OpCodes::UNKNOWN;
    }
}

OpCodes get_register_LA(const string & s)
{
    //cout << "LATINA\n";
    string str = s;
    if (str == "vr1")
        return OpCodes::VR1;
    else if (str == "vr2")
        return OpCodes::VR2;
    else if (str == "vr3")
        return OpCodes::VR3;
    else if (str == "vr4")
        return OpCodes::VR4;
    else if (str == "vr5")
        return OpCodes::VR5;
    else if (str == "vri")
        return OpCodes::VRI;
    else if (str == "vdm")
        return OpCodes::VIP;
    else if (str == "vsm")
        return OpCodes::VSP;
    else if (str == "vre")
        return OpCodes::VRA;
    else if (str == "marco")
        return OpCodes::FLAGS;
    else
    {
        // cout << endl << "unknown register: " << str << endl;
        return OpCodes::UNKNOWN;
    }
}

OpCodes get_register_UA(const string & s)
{
    string str = s;
    if (str == "рег1")
        return OpCodes::VR1;
    else if (str == "рег2")
        return OpCodes::VR2;
    else if (str == "рег3")
        return OpCodes::VR3;
    else if (str == "рег4")
        return OpCodes::VR4;
    else if (str == "рег5")
        return OpCodes::VR5;
    else if (str == "рі")
        return OpCodes::VRI;
    else if (str == "раі")
        return OpCodes::VIP;
    else if (str == "рс")
        return OpCodes::VSP;
    else if (str == "рр")
        return OpCodes::VRA;
    else if (str == "рфл")
        return OpCodes::FLAGS;
    else
    {
        // cout << endl << "unknown register: " << str << endl;
        return OpCodes::UNKNOWN;
    }
}

// mov vr1 34
// add vr1 arg
// call fun1
// pop

OpCodes get_command (const string &str_a) {
    if(curLocale != CodeLocale::English)
        if ((curLocale == CodeLocale::Default && CodeLocale::Default == CodeLocale::Ukrainian)
                || curLocale == CodeLocale::Ukrainian) return get_command_UA(str_a);
        else if ((curLocale == CodeLocale::Default && CodeLocale::Default == CodeLocale::Latin)
                 || curLocale == CodeLocale::Latin) return get_command_LA(str_a);
    string str = trim(str_a);
    const char * cstr = str.c_str();
    switch (cstr[0])
    {
    case 'a':
        if(!strcmp(cstr, "add")) return OpCodes::ADD ;
        else if(!strcmp(cstr, "and")) return OpCodes::AND ;
        else if(!strcmp(cstr, "alloc")) return OpCodes::ALLOC ;
        break;
    //case 'B':
    //	if(!strcmp(cstr, "BAND")) return OpCodes::BAND ;
    //	else if(!strcmp(cstr, "BTC")) return OpCodes::BTC ;
    //	else if(!strcmp(cstr, "BOR")) return OpCodes::BOR ;
    //	else if(!strcmp(cstr, "BTS")) return OpCodes::BTS ;
    //	else if(!strcmp(cstr, "BXOR")) return OpCodes::BXOR ;
    //	break;
    case 'c':
        if(!strcmp(cstr, "call")) return OpCodes::CALL ;
        if(!strcmp(cstr, "calle")) return OpCodes::CALLE ;
        if(!strcmp(cstr, "callne")) return OpCodes::CALLNE ;
        else if(!strcmp(cstr, "cmp")) return OpCodes::CMP ;
        break;
    case 'd':
        if(!strcmp(cstr, "dec")) return OpCodes::DEC ;
        else if(!strcmp(cstr, "def")) return OpCodes::DEF ;
        else if(!strcmp(cstr, "div")) return OpCodes::DIV ;
        break;
    case 'e':
        if(!strcmp(cstr, "end")) return OpCodes::END ;
        break;
    case 'f':
        if(!strcmp(cstr, "fun")) return OpCodes::FUN ;
        else if(!strcmp(cstr, "free")) return OpCodes::FREE ;
        break;
    case 'h':
        if(!strcmp(cstr, "halt")) return OpCodes::HALT ;
        break;
    case 'i':
        if(!strcmp(cstr, "inc")) return OpCodes::INC ;
        break;
    case 'j':
        if(!strcmp(cstr, "jmp")) return OpCodes::JMP ;
        else if(!strcmp(cstr, "je")) return OpCodes::JE ;
        else if(!strcmp(cstr, "jne")) return OpCodes::JNE ;
        else if(!strcmp(cstr, "jg")) return OpCodes::JG ;
        else if(!strcmp(cstr, "jl")) return OpCodes::JL ;
        else if(!strcmp(cstr, "jge")) return OpCodes::JGE ;
        else if(!strcmp(cstr, "jle")) return OpCodes::JLE ;
        break;
    case 'm':
        if(!strcmp(cstr, "mov")) return OpCodes::MOV ;
        else if(!strcmp(cstr, "mul")) return OpCodes::MUL ;
        break;
    case 'n':
        if(!strcmp(cstr, "namespace")) return OpCodes::NAMESPACE ;
        else if(!strcmp(cstr, "neg")) return OpCodes::NEG ;
        else if(!strcmp(cstr, "nop")) return OpCodes::NOP ;
        else if(!strcmp(cstr, "not")) return OpCodes::NOT ;
        break;
    case 'o':
        if(!strcmp(cstr, "or")) return OpCodes::OR ;
        break;
    case 'p':
        if(!strcmp(cstr, "pop")) return OpCodes::POP ;
        else if(!strcmp(cstr, "prot")) return OpCodes::PROT ;
        else if(!strcmp(cstr, "push")) return OpCodes::PUSH ;
        break;
    case 'r':
        if(!strcmp(cstr, "ret")) return OpCodes::RET ;
        if(!strcmp(cstr, "rol")) return OpCodes::ROL ;
        else if(!strcmp(cstr, "ror")) return OpCodes::ROR ;
        break;
    case 's':
        //if(!strcmp(cstr, "SET")) return OpCodes::SET ;
        if(!strcmp(cstr, "shl")) return OpCodes::SHL ;
        else if(!strcmp(cstr, "shr")) return OpCodes::SHR ;
        else if(!strcmp(cstr, "sub")) return OpCodes::SUB ;
        else if(!strcmp(cstr, "stdin")) return OpCodes::STDIN ;
        else if(!strcmp(cstr, "stdout")) return OpCodes::STDOUT ;
        break;
    case 'x':
        if(!strcmp(cstr, "xch")) return OpCodes::XCH ;
        else if(!strcmp(cstr, "xor")) return OpCodes::XOR ;
        break;
    default:
        return OpCodes::UNKNOWN;
        break;
    }
    return OpCodes::UNKNOWN;
}
OpCodes get_command_LA (const string &str_a) {
    string str = trim(str_a);
    const char * cstr = str.c_str();
    switch (cstr[0])
    {
    case 'a':
        if(!strcmp(cstr, "addo")) return OpCodes::ADD ;
        else if(!strcmp(cstr, "absque")) return OpCodes::POP ;  ////////////////////////////// #
        else if(!strcmp(cstr, "aequo")) return OpCodes::CMP ;  ////////////////////////////// #
        else if(!strcmp(cstr, "ams")) return OpCodes::SHL ;
        else if(!strcmp(cstr, "aml")) return OpCodes::SHR ;
        break;
    //case 'B':
    //	if(!strcmp(cstr, "BAND")) return OpCodes::BAND ;
    //	else if(!strcmp(cstr, "BTC")) return OpCodes::BTC ;
    //	else if(!strcmp(cstr, "BOR")) return OpCodes::BOR ;
    //	else if(!strcmp(cstr, "BTS")) return OpCodes::BTS ;
    //	else if(!strcmp(cstr, "BXOR")) return OpCodes::BXOR ;
    //	break;
    case 'c':
        if(!strcmp(cstr, "commutatio")) return OpCodes::XCH ;
        break;
    case 'd':
        if(!strcmp(cstr, "dec")) return OpCodes::DEC ;
        else if(!strcmp(cstr, "defino")) return OpCodes::DEF ;
        break;
    case 'e':
        if(!strcmp(cstr, "et")) return OpCodes::AND ;
        else if(!strcmp(cstr, "exitus")) return OpCodes::HALT ;
        else if(!strcmp(cstr, "exemplar")) return OpCodes::PROT ;  ////////////////////////////// #
        break;
    case 'f':
        if(!strcmp(cstr, "finis")) return OpCodes::END ;  ////////////////////////////// #
        break;
    case 'i':
        if(!strcmp(cstr, "inc")) return OpCodes::INC ;
        break;
    case 'l':
        if(!strcmp(cstr, "libero")) return OpCodes::FREE ;  ////////////////////////////// #
        break;
    case 'm':
        if(!strcmp(cstr, "munus")) return OpCodes::FUN ;  ////////////////////////////// #
        else if(!strcmp(cstr, "multiplico")) return OpCodes::MUL ;
        else if(!strcmp(cstr, "minuere")) return OpCodes::SUB ;
        break;
    case 'n':
        if(!strcmp(cstr, "namespace")) return OpCodes::NAMESPACE ;
        else if(!strcmp(cstr, "negare")) return OpCodes::NEG ;
        else if(!strcmp(cstr, "nop")) return OpCodes::NOP ;
        else if(!strcmp(cstr, "nec")) return OpCodes::NOT ;
        break;
    case 'p':
        if(!strcmp(cstr, "permoveo")) return OpCodes::MOV ;  ////////////////////////////// #
        else if(!strcmp(cstr, "propvel")) return OpCodes::XOR ;
        else if(!strcmp(cstr, "partio")) return OpCodes::DIV ;  ////////////////////////////// #
        else if(!strcmp(cstr, "propvel")) return OpCodes::XOR ;
        break;
    case 'r':
        if(!strcmp(cstr, "redeo")) return OpCodes::RET ;
        break;
    case 's':
        if(!strcmp(cstr, "salio")) return OpCodes::JMP ;  ////////////////////////////// #
        else if(!strcmp(cstr, "sa")) return OpCodes::JE ;  ////////////////////////////// #
        else if(!strcmp(cstr, "sna")) return OpCodes::JNE ;  ////////////////////////////// #
        else if(!strcmp(cstr, "sm")) return OpCodes::JG ;  ////////////////////////////// #
        else if(!strcmp(cstr, "si")) return OpCodes::JL ;  ////////////////////////////// #
        else if(!strcmp(cstr, "sma")) return OpCodes::JGE ;  ////////////////////////////// #
        else if(!strcmp(cstr, "sia")) return OpCodes::JLE ;  ////////////////////////////// #
        //if(!strcmp(cstr, "SET")) return OpCodes::SET ;
        break;
    case 'v':
        if(!strcmp(cstr, "vel")) return OpCodes::OR ;  ////////////////////////////// #
        else if(!strcmp(cstr, "vos")) return OpCodes::ROL ;
        else if(!strcmp(cstr, "vol")) return OpCodes::ROR ;
        else if(!strcmp(cstr, "ventilabis")) return OpCodes::PUSH ;  ////////////////////////////// #
        else if(!strcmp(cstr, "vexin")) return OpCodes::STDIN ;
        else if(!strcmp(cstr, "vexex")) return OpCodes::STDOUT ;
        else if(!strcmp(cstr, "voco")) return OpCodes::CALL ;  ////////////////////////////// #
        else if(!strcmp(cstr, "vocoa")) return OpCodes::CALLE ;  ////////////////////////////// #
        else if(!strcmp(cstr, "vocona")) return OpCodes::CALLNE ;  ////////////////////////////// #
        else if(!strcmp(cstr, "venitat")) return OpCodes::ALLOC ;  ////////////////////////////// #
        break;
    default:
        return OpCodes::UNKNOWN;
        break;
    }
    return OpCodes::UNKNOWN;
}
OpCodes get_command_UA (const string &str_a) {
    string str = trim(str_a);
    const char * cstr = str.c_str();

    if(!strcmp(cstr, "або")) return OpCodes::OR ;
    else if(!strcmp(cstr, "або_викл")) return OpCodes::XOR ;
//case 'B':
//	if(!strcmp(cstr, "BAND")) return OpCodes::BAND ;
//	else if(!strcmp(cstr, "BTC")) return OpCodes::BTC ;
//	else if(!strcmp(cstr, "BOR")) return OpCodes::BOR ;
//	else if(!strcmp(cstr, "BTS")) return OpCodes::BTS ;
//	else if(!strcmp(cstr, "BXOR")) return OpCodes::BXOR ;
    else if(!strcmp(cstr, "виклик")) return OpCodes::CALL ;
    else if(!strcmp(cstr, "виклик_д")) return OpCodes::CALLE ;
    else if(!strcmp(cstr, "виклик_нд")) return OpCodes::CALLNE ;
    else if(!strcmp(cstr, "вихід")) return OpCodes::HALT ;
    else if(!strcmp(cstr, "виділити")) return OpCodes::ALLOC ;
    else if(!strcmp(cstr, "зняти")) return OpCodes::POP ;
    else if(!strcmp(cstr, "відняти")) return OpCodes::SUB ;
    else if(!strcmp(cstr, "ввести")) return OpCodes::STDIN ;
    else if(!strcmp(cstr, "вивести")) return OpCodes::STDOUT ;
    else if(!strcmp(cstr, "додати")) return OpCodes::ADD ;
    else if(!strcmp(cstr, "декр")) return OpCodes::DEC ;
    else if(!strcmp(cstr, "кінець")) return OpCodes::END ;
    //if(!strcmp(cstr, "END")) return OpCodes::END ;
    //else if(strcmp(cstr, "EXC")) return OpCodes::EXC;
    else if(!strcmp(cstr, "звільнити")) return OpCodes::FREE;
    else if(!strcmp(cstr, "зсув_л")) return OpCodes::SHL ;
    else if(!strcmp(cstr, "зсув_п")) return OpCodes::SHR ;
    else if(!strcmp(cstr, "змінна")) return OpCodes::DEF ;
    else if(!strcmp(cstr, "стоп")) return OpCodes::HALT ;
    else if(!strcmp(cstr, "інкр")) return OpCodes::INC ;
    else if(!strcmp(cstr, "і")) return OpCodes::AND ;
    else if(!strcmp(cstr, "к_зсув_л")) return OpCodes::ROL ;
    else if(!strcmp(cstr, "к_зсув_п")) return OpCodes::ROR ;
    else if(!strcmp(cstr, "множити")) return OpCodes::MUL ;
    else if(!strcmp(cstr, "негатив")) return OpCodes::NEG ;
    else if(!strcmp(cstr, "не")) return OpCodes::NOT ;
    else if(!strcmp(cstr, "перейти")) return OpCodes::JMP ;
    else if(!strcmp(cstr, "ділити")) return OpCodes::DIV ;
    else if(!strcmp(cstr, "порівн")) return OpCodes::CMP ;
    else if(!strcmp(cstr, "перейти_д")) return OpCodes::JE ;
    else if(!strcmp(cstr, "перейти_нд")) return OpCodes::JNE ;
    else if(!strcmp(cstr, "перейти_б")) return OpCodes::JG ;
    else if(!strcmp(cstr, "перейти_м")) return OpCodes::JL ;
    else if(!strcmp(cstr, "перейти_бд")) return OpCodes::JGE ;
    else if(!strcmp(cstr, "перейти_мд")) return OpCodes::JLE ;
    else if(!strcmp(cstr, "прост_імен")) return OpCodes::NAMESPACE ;
    else if(!strcmp(cstr, "перем")) return OpCodes::MOV ;
    else if(!strcmp(cstr, "пусто")) return OpCodes::NOP ;
    else if(!strcmp(cstr, "прототип")) return OpCodes::PROT ;
    else if(!strcmp(cstr, "покласти")) return OpCodes::PUSH ;
    else if(!strcmp(cstr, "поверн")) return OpCodes::RET ;
    else if(!strcmp(cstr, "помін")) return OpCodes::XCH ;
    else if(!strcmp(cstr, "та")) return OpCodes::AND ;
    else if(!strcmp(cstr, "функ")) return OpCodes::FUN ;
    return OpCodes::UNKNOWN;
}

CompilerItem findCEntry(const string & Key)
{
	vector < CompilerItem >::iterator iter = cstack.begin();
	for (; iter != cstack.end(); ++iter)
		if ((*iter).name == Key)
			return *iter;
    CompilerItem ce = { -1, PODTypes::_NULL, "" };
	return ce;
}

bool containsCEntry(const string & Key)
{
	Token token = Key;

    string ns, name = token.GetNextToken();
	// if(token.GetNextToken() == ":");
	Token tok = token.GetTail();
    // if(token.GetTokenType() != Token::IDENTIFIER) throw (S("Поганий ідентифікатор:
	// ") + name).ToCString();
	if (tok.GetNextToken() == ":")
		if (tok.GetNextToken() == ":")
		{
			name += "::" + tok.GetNextToken();
			for (string s = tok.GetNextToken();
				tok.GetTokenType() == Token::IDENTIFIER || s == ":";
				s = tok.GetNextToken())
				name += s;
		}
		vector < CompilerItem >::iterator iter = cstack.begin();
		for (; iter != cstack.end(); ++iter)
			if ((*iter).name == Key)
				return true;
		return false;
}

CompilerItem findFCEntry(const string & Key)
{
	vector < CompilerItem >::iterator iter = procstack.begin();
	for (; iter != procstack.end(); ++iter)
		if ((*iter).name == Key)
			return *iter;
    CompilerItem ce = { 0, PODTypes::_NULL, ""};
	return ce;
}

bool containsFCEntry(const string & Key)
{
	vector < CompilerItem >::iterator iter = procstack.begin();
	for (; iter != procstack.end(); ++iter)
		if ((*iter).name == Key)
			return true;
	return false;
}


CompilerItem findJCEntry(const string & Key)
{
	vector < CompilerItem >::iterator iter = jmpstack.begin();
	for (; iter != jmpstack.end(); ++iter)
		if ((*iter).name == Key)
			return *iter;
    CompilerItem ce = { 0, PODTypes::_NULL, "" };
	return ce;
}

bool containsJCEntry(const string & Key)
{
	vector < CompilerItem >::iterator iter = jmpstack.begin();
	for (; iter != jmpstack.end(); ++iter)
		if ((*iter).name == Key)
			return true;
	return false;
}

bool isRegister(const string & str)
{
	// cout << endl << "register: " << str << endl;
	if (get_register(str) == OpCodes::UNKNOWN)
		return false;
	else
	{
		// cout << endl << "true register: " << str << endl;
		return true;
	}

}

PODTypes getType(string sstr)
{
    char * str = new char [sstr.length()];
    strcpy(str, sstr.c_str());
	bool hasDot = false;
	for (; *str != '\0'; str++)
	{
		if (*str == '.')
            if (hasDot)
            {
                //delete [] str;
				return PODTypes::_NULL;
            }
            else
            {
                //delete [] str;
				hasDot = true;
            }
		else if (*str != '.')
            if (*str < '0' || *str > '9') {
                //delete [] str;
				return PODTypes::_NULL;
            }
    }
    //delete [] str;
	if (hasDot)
		return PODTypes::DWORD;
	else
		return PODTypes::WORD;
}

bool isMacro(string str) {

}

void pushC1arg(OpCodes w, Token & token)
{
    pushC2args(w, token, true);
    return;

	commands.push_back((byte)w);
	PODTypes cast = PODTypes::_NULL;
	string t1 = "";//, s = "";
    if (isFullName(trim(token.GetTail())))
	{
		t1 = getFullName(token);
		commands.push_back((byte)PODTypes::IDENTIFIER);
		CompilerItem entry = findCEntry(t1);
		memory_unit w;
		w.word = entry.addr;
		commands.push_back(w.bytes[0]);
		commands.push_back(w.bytes[1]);
		commands.push_back(w.bytes[2]);
		commands.push_back(w.bytes[3]);
		return;
	}
	else
		t1 = token.GetNextToken();	// .Append(0);
	// for(char ch : string(t1)) {
	// cout << "\tchar: " << ch << "\tdec: " << (int)ch << endl;
	// }
	// cout << "argument: '" << t1 << "-Hello" << endl;
    if (t1 == localize("double") || t1 == localize("int") || t1 == localize("short") || t1 == localize("byte")
        || t1 == localize("bool") || t1 == localize("int64"))
    {
        // cout << "CAST" << endl;
        if (t1 == localize("double"))
            cast = PODTypes::DWORD;
        else if (t1 == localize("int64"))
            cast = PODTypes::W64;
        else if (t1 == localize("int"))
            cast = PODTypes::WORD;
        else if (t1 == localize("short"))
            cast = PODTypes::HWORD;
        else if (t1 == localize("byte"))
            cast = PODTypes::BYTE;
        else if (t1 == localize("bool"))
			cast = PODTypes::BOOL;
        else
            throw CompilerException("Неправильне приведення типу: " + t1, sourceFile,
			line);
		if (token.GetNextToken() != ":")
            throw CompilerException("':' очікувалось!" + t1, sourceFile, line);
		t1 = token.GetNextToken();
		commands.push_back((byte)OpCodes::CAST);
		commands.push_back((byte)cast);
	}
	if (t1 == "[")
	{
		commands.push_back((byte)OpCodes::SQBRCK);
		if (isFullName(token.GetTail()))
		{
			t1 = getFullName(token);
			commands.push_back((byte)PODTypes::IDENTIFIER);
			CompilerItem entry = findCEntry(t1);
			memory_unit w;
			w.word = entry.addr;
			commands.push_back(w.bytes[0]);
			commands.push_back(w.bytes[1]);
			commands.push_back(w.bytes[2]);
			commands.push_back(w.bytes[3]);
			if (token.GetNextToken() != "]")
                throw CompilerException("']' очікувалось!", sourceFile, line);
			return;
		}
		string tmp = token.GetNextToken();

		if (isRegister(tmp))
			commands.push_back((byte)get_register(tmp));
        else if (is_digits_only(tmp.c_str()))
		{
			commands.push_back((byte)PODTypes::WORD);
			memory_unit w;
            w.word = atoi(tmp.c_str());
			commands.push_back(w.bytes[0]);
			commands.push_back(w.bytes[1]);
			commands.push_back(w.bytes[2]);
			commands.push_back(w.bytes[3]);
		}
		else if (containsCEntry(getCurrentNS() + "::" + t1))
		{
			commands.push_back((byte)PODTypes::IDENTIFIER);
			CompilerItem entry = findCEntry(getCurrentNS() + "::" + t1);
			if (entry.addr == -1)
                throw CompilerException("Неможливо знайти: '" + t1 + "'",
				sourceFile, line);
			memory_unit w;
			w.word = entry.addr;
			commands.push_back(w.bytes[0]);
			commands.push_back(w.bytes[1]);
			commands.push_back(w.bytes[2]);
			commands.push_back(w.bytes[3]);
		}
		else if (getCurrentNS() == "" && containsCEntry(t1))
		{
			commands.push_back((byte)PODTypes::IDENTIFIER);
			CompilerItem entry = findCEntry(t1);
			memory_unit w;
			w.word = entry.addr;
			commands.push_back(w.bytes[0]);
			commands.push_back(w.bytes[1]);
			commands.push_back(w.bytes[2]);
			commands.push_back(w.bytes[3]);
		}
		else
            throw CompilerException("Неправильний операнд: '" + t1 + "'", sourceFile,
			line);
		if (token.GetNextToken() != "]")
            throw CompilerException("']' очікувалось!", sourceFile, line);
	}
	else if (isRegister(t1))
	{
		commands.push_back((byte)get_register(t1));
	}
    else if (containsCEntry(t1))
	{
		commands.push_back((byte)PODTypes::IDENTIFIER);
		CompilerItem entry = findCEntry(t1);
		memory_unit w;
		w.word = entry.addr;
		commands.push_back(w.bytes[0]);
		commands.push_back(w.bytes[1]);
		commands.push_back(w.bytes[2]);
		commands.push_back(w.bytes[3]);
	}
	else if (t1 == "-")
	{
        if (is_digits_only((t1 = token.GetNextToken()).c_str()))
		{
			if (!isValid1arg(w))
			{
				// cout << "loop: " << loop << endl;
                throw CompilerException("Неочікувана константа: " + t1,
					sourceFile, line);
			}
			PODTypes type = getType(t1);
			t1 = "-" + t1;
			if (type == PODTypes::_NULL)
                throw CompilerException("Лексема не є числом: " + t1, sourceFile,
				line);
			commands.push_back((byte)type);
			memory_unit w;
			if (type == PODTypes::WORD)
			{
                w.word = atoi(t1.c_str());	/*
									commands.push_back(0xCA);
									commands.push_back(0xFE);
									commands.push_back(0xDE);
									commands.push_back(0xAD); */
				commands.push_back(w.bytes[0]);
				commands.push_back(w.bytes[1]);
				commands.push_back(w.bytes[2]);
				commands.push_back(w.bytes[3]);
			}
			else
			{
                w.dword = atof(t1.c_str());
				commands.push_back(w.bytes[0]);
				commands.push_back(w.bytes[1]);
				commands.push_back(w.bytes[2]);
				commands.push_back(w.bytes[3]);
				commands.push_back(w.bytes[4]);
				commands.push_back(w.bytes[5]);
				commands.push_back(w.bytes[6]);
				commands.push_back(w.bytes[7]);
			}
		}
	}
    else if (is_digits_only(t1.c_str()))
	{
		if (!isValid1arg(w))
		{
			// cout << "loop: " << loop << endl;
            throw CompilerException("Неочікувана константа: " + t1,
				sourceFile, line);
		}
		PODTypes type = getType(t1);
		if (type == PODTypes::_NULL)
            throw CompilerException("Лексема не є числом: " + t1, sourceFile,
			line);
		commands.push_back((byte)type);
		memory_unit w;
		if (type == PODTypes::WORD)
		{
            w.word = atoi(t1.c_str());
			commands.push_back(w.bytes[0]);
			commands.push_back(w.bytes[1]);
			commands.push_back(w.bytes[2]);
			commands.push_back(w.bytes[3]);
		}
		else if (type == PODTypes::DWORD)
		{
            w.dword = atof(t1.c_str());
			commands.push_back(w.bytes[0]);
			commands.push_back(w.bytes[1]);
			commands.push_back(w.bytes[2]);
			commands.push_back(w.bytes[3]);
			commands.push_back(w.bytes[4]);
			commands.push_back(w.bytes[5]);
			commands.push_back(w.bytes[6]);
			commands.push_back(w.bytes[7]);
		}
	}
	else if (token.GetTokenType() == Token::STRING)
	{
		commands.push_back((byte)PODTypes::STRING);
		memory_unit unit;
		unit.reg = 0;
		str_stack.push_back(t1);
		unit.word = str_stack.size() - 1;
		commands.push_back(unit.bytes[0]);
		commands.push_back(unit.bytes[1]);
		commands.push_back(unit.bytes[2]);
		commands.push_back(unit.bytes[3]);
        str_stack_table.push_back(t1.length());
	}
	else
        throw CompilerException("Неправильний операнд: '" + t1 + "'", sourceFile,
		line);
}

void pushC2args(OpCodes w, Token & token, bool oneArg)
{
	int l = 1;
	// cout << "COM: " << 
	commands.push_back((byte)w);
loop:
    bool _sizeof = false, _addrof = false;
	PODTypes cast = PODTypes::_NULL;
	string t1 = "";//, s = "";
	if (isFullName(token.GetTail()))
	{
		t1 = getFullName(token);
		commands.push_back((byte)PODTypes::IDENTIFIER);
		CompilerItem entry = findCEntry(t1);
		memory_unit w;
		w.word = entry.addr;
		commands.push_back(w.bytes[0]);
		commands.push_back(w.bytes[1]);
		commands.push_back(w.bytes[2]);
		commands.push_back(w.bytes[3]);
        if (l == 1 && !oneArg)
		{
			if (token.GetNextToken() != ",")
                throw CompilerException("',' очікувалось!", sourceFile, line);
			// cout << "loop incremented" << endl;
			l++;

			goto loop;
		}
		return;
	}
	else
		t1 = token.GetNextToken();
    if(w == OpCodes::ALLOC) {
        //cout << "ALLOC: T1: " << t1 << endl;
    }
    if(token.GetTokenType() == Token::EOF && w == OpCodes::POP) {
        commands.push_back((byte)PODTypes::BYTE);
        commands.push_back(1);
        return;
    }
	//	t1 = t1.Trim();
    if (t1 == localize("double") || t1 == localize("int") || t1 == localize("short") || t1 == localize("byte")
        || t1 == localize("bool") || t1 == localize("int64"))
        if(token.GetNextToken() == ":")
        {
            token.PushBack();
            // cout << "CAST" << endl;
            if (t1 == localize("double"))
                cast = PODTypes::DWORD;
            else if (t1 == localize("int64"))
                cast = PODTypes::W64;
            else if (t1 == localize("int"))
                cast = PODTypes::WORD;
            else if (t1 == localize("short"))
                cast = PODTypes::HWORD;
            else if (t1 == localize("byte"))
                cast = PODTypes::BYTE;
            else if (t1 == localize("bool"))
                cast = PODTypes::BOOL;
            else
                throw CompilerException("Неправильне приведення типу: " + t1, sourceFile,
                line);
            if (token.GetNextToken() != ":")
                throw CompilerException("Неправильне приведення типу: " + t1, sourceFile,
                line);
            t1 = token.GetNextToken();
            commands.push_back((byte)OpCodes::CAST);
            commands.push_back((byte)cast);
        }
        else if(w == OpCodes::ALLOC) {
            commands.push_back((byte)stringToType(t1));
            //cout << "ALLOC TYPE: " << token.GetCurrentToken() << endl;
            token.PushBack();
        }
    if (t1 == localize("sizeof") && token.GetTokenType() != Token::STRING) {
        if(l == 1 && !isValid1arg(w))
            throw CompilerException("Неочікуваний оператор 'розм'",
                sourceFile, line);
        _sizeof = true;
        commands.push_back((byte)OpCodes::SIZEOF);
        if(token.GetNextToken() != "(")
            throw CompilerException("'(' очікувалось після 'розм'", sourceFile, line);
        t1 = token.GetNextToken();
    }
    else if (t1 == localize("addr") && token.GetTokenType() != Token::STRING) {
        if(l == 1 && !isValid1arg(w))
            throw CompilerException("Неочікуваний оператор 'адр'",
                sourceFile, line);
        _addrof = true;
        commands.push_back((byte)OpCodes::ADDR);
        if(token.GetNextToken() != "(")
            throw CompilerException("'(' очікувалось після 'адр'", sourceFile, line);
        t1 = token.GetNextToken();
    }
    if (t1 == "[" && token.GetTokenType() != Token::STRING) {
        //t1 = token.getn
        auto v = readInStack(t1, token, false);
        if(token.GetNextToken() == "[") {
            commands.push_back((byte)PODTypes::POINTER);
            if(isIdent)
            {
                *(real_addr_table.end()) += 1;
                isIdent = false;
            }
            commands.insert(commands.end(), v.begin(), v.end());
            //t1 = token.GetNextToken();
            auto v2 = readInStack(t1, token, true);
            commands.insert(commands.end(), v2.begin(), v2.end());
        }
        else {
            token.PushBack();
            commands.insert(commands.end(), v.begin(), v.end());
        }
    }
    else if (isRegister(t1))
	{
        // mov myarray[5], 123;
        // mov | pointer identifier myarray digit 5 | digit 123
        // mov vr1[vr2], 123
        // mov | pointer vr1 vr2 |
        bool array_element = false;
        if(token.GetNextToken() == "[")  {
            commands.push_back((byte)PODTypes::POINTER);
            array_element = true;
        }
        else token.PushBack();
		commands.push_back((byte)get_register(t1));
        if(array_element) {
            auto v = readInStack(t1, token, true);
            commands.insert(commands.end(), v.begin(), v.end());
        }
    }
    else if (containsCEntry(t1))
    {
        //commands.push_back((byte)PODTypes::IDENTIFIER);
        CompilerItem entry = findCEntry(t1);
        bool array_element = false;
        if(token.GetNextToken() == "[")  {
            commands.push_back((byte)PODTypes::POINTER);
            array_element = true;
        }
        else token.PushBack();
        commands.push_back((byte)OpCodes::SQBRCK);
        commands.push_back((byte)PODTypes::WORD);
        memory_unit w;
        w.word = entry.addr;
        if(entry.type == PODTypes::POINTER)
            real_array_table.push_back(w.word);
        //else
            real_addr_table.push_back(commands.size());
        commands.push_back(w.bytes[0]);
        commands.push_back(w.bytes[1]);
        commands.push_back(w.bytes[2]);
        commands.push_back(w.bytes[3]);
        if(array_element) {
            auto v = readInStack(t1, token, true);
            commands.insert(commands.end(), v.begin(), v.end());
        }
	}
    else if (t1 == "-" && token.GetTokenType() != Token::STRING)
	{
        if (is_digits_only((t1 = token.GetNextToken()).c_str()))
		{
            if (l == 1 && !isValid1arg(w))
			{
				// cout << "loop: " << loop << endl;
                throw CompilerException("Неочікувана константа: " + t1,
					sourceFile, line);
			}
			PODTypes type = getType(t1);
			t1 = "-" + t1;
			if (type == PODTypes::_NULL)
                throw CompilerException("Не число: " + t1, sourceFile,
				line);
			commands.push_back((byte)type);
			memory_unit w;
			if (type == PODTypes::WORD)
			{
                w.word = atoi(t1.c_str());	/*
									commands.push_back(0xCA);
									commands.push_back(0xFE);
									commands.push_back(0xDE);
									commands.push_back(0xAD); */
				commands.push_back(w.bytes[0]);
				commands.push_back(w.bytes[1]);
				commands.push_back(w.bytes[2]);
				commands.push_back(w.bytes[3]);
			}
			else
			{
                w.dword = atof(t1.c_str());
				commands.push_back(w.bytes[0]);
				commands.push_back(w.bytes[1]);
				commands.push_back(w.bytes[2]);
				commands.push_back(w.bytes[3]);
				commands.push_back(w.bytes[4]);
				commands.push_back(w.bytes[5]);
				commands.push_back(w.bytes[6]);
				commands.push_back(w.bytes[7]);
			}
        }
	}
    else if (is_digits_only(t1.c_str()))
	{
        if (l == 1 && !isValid1arg(w))
		{
			// cout << "loop: " << loop << endl;
            throw CompilerException("Неочікувана константа: " + t1,
				sourceFile, line);
		}
        PODTypes type = getType(t1);
        //if (!(type > (byte)PODTypes::BOOL))
        //    throw CompilerException("Не число: " + t1, sourceFile,
        //	line);
		commands.push_back((byte)type);
        memory_unit unit;
        /*if(w == OpCodes::POP) {
            int t = atoi(t1.c_str());
            if(t > 0xFF)
                throw CompilerException("Pop: Number is bigger then 0xFF: " + t, sourceFile, line);
            commands.push_back((byte)t);
        }
        else*/ if (type == PODTypes::WORD)
		{
            unit.word = atoi(t1.c_str());
            commands.push_back(unit.bytes[0]);
            commands.push_back(unit.bytes[1]);
            commands.push_back(unit.bytes[2]);
            commands.push_back(unit.bytes[3]);
		}
		else
		{

            unit.dword = atof(t1.c_str());
            commands.push_back(unit.bytes[0]);
            commands.push_back(unit.bytes[1]);
            commands.push_back(unit.bytes[2]);
            commands.push_back(unit.bytes[3]);
            commands.push_back(unit.bytes[4]);
            commands.push_back(unit.bytes[5]);
            commands.push_back(unit.bytes[6]);
            commands.push_back(unit.bytes[7]);
		}
        //cout << "CURRENT TOKEN: " << token.GetCurrentToken() << endl;
    }
    //else if(isMacro(t1)) {
//
    //}
    else if (token.GetTokenType() == Token::STRING && w != OpCodes::POP)
    {
        if (l == 1 && !isValid1arg(w))
        {
            // cout << "loop: " << loop << endl;
            throw CompilerException("Неочікувана рядкова константа: " + t1,
                sourceFile, line);
        }
        commands.push_back((byte)PODTypes::STRING);
        memory_unit unit;
        unit.reg = 0;
        str_stack.push_back(t1);
        unit.word = str_stack.size() - 1;
        real_string_table.push_back(commands.size());
        commands.push_back(unit.bytes[0]);
        commands.push_back(unit.bytes[1]);
        commands.push_back(unit.bytes[2]);
        commands.push_back(unit.bytes[3]);
        str_stack_table.push_back(t1.length());
    }
    else if(t1 == localize("true"))
    {
        commands.push_back((byte)PODTypes::BOOL);
        commands.push_back(1);
    }
    else if(t1 == localize("false"))
    {
        commands.push_back((byte)PODTypes::BOOL);
        commands.push_back(0);
    }
    else if (w != OpCodes::ALLOC){
        if(l != 1) throw CompilerException("Неправильний другий операнд: '" + t1 + "'", sourceFile, line);
        else throw CompilerException("Неправильний перший операнд: '" + t1 + "'", sourceFile, line);
	}	

    if(_sizeof || _addrof) {
        //cout << token.GetCurrentToken() << " ";
        if(token.GetNextToken() != ")")
            throw CompilerException("')' очікувалось! " +token.GetCurrentToken(), sourceFile, line);
    }
    if (l == 1 && !oneArg)
    {
        //cout << "CURRENT TOKEN: " << token.GetCurrentToken() << endl;
        //if(token.GetCurrentToken() != "," && w != OpCodes::ALLOC)
            if (token.GetNextToken() != ",")
                throw CompilerException("',' очікувалось: " + token.GetCurrentToken(), sourceFile, line);

		// cout << "loop incremented" << endl;
		l++;
		goto loop;
	}
}

vector<byte>  readInStack(string &t1, Token &token, bool isArray) {
    //cout << "Token 1 is " << t1 << endl;
    vector<byte> internal;
    if(!isArray)
        internal.push_back((byte)OpCodes::SQBRCK);
    /*if (isFullName(token.GetTail()))
    {
        t1 = getFullName(token);
        internal.push_back((byte)PODTypes::IDENTIFIER);
        CompilerItem entry = findCEntry(t1);
        memory_unit unit;
        unit.word = entry.addr;
        internal.push_back(unit.bytes[0]);
        internal.push_back(unit.bytes[1]);
        internal.push_back(unit.bytes[2]);
        internal.push_back(unit.bytes[3]);
        if (token.GetNextToken() != "]")
            throw CompilerException("']' expected", sourceFile, line);

    }*/
    string tmp = token.GetNextToken();
    if (isRegister(tmp))
    {
        // cout << endl << "register" << endl;
        internal.push_back((byte)get_register(tmp));
    }
    else if (is_digits_only(tmp.c_str()))
    {
        internal.push_back((byte)PODTypes::WORD);
        memory_unit unit;
        unit.word = atoi(tmp.c_str());
        internal.push_back(unit.bytes[0]);
        internal.push_back(unit.bytes[1]);
        internal.push_back(unit.bytes[2]);
        internal.push_back(unit.bytes[3]);
    }
    else if (containsFCEntry(tmp)) {
        //internal.push_back((byte)PODTypes::FUNC);
        //auto e = findFCEntry(t1);
        //procstack.
    }
    else if (containsCEntry(tmp))
    {
        internal.push_back((byte)PODTypes::WORD);
        CompilerItem entry = findCEntry(tmp);
        memory_unit unit;
        unit.word = entry.addr;
        real_addr_table.push_back(commands.size()+internal.size());
        internal.push_back(unit.bytes[0]);
        internal.push_back(unit.bytes[1]);
        internal.push_back(unit.bytes[2]);
        internal.push_back(unit.bytes[3]);
        isIdent = true;
    }
    else
        throw CompilerException("Неправильний операнд: '" + tmp + "'", sourceFile,
        line);
    if (token.GetNextToken() != "]") {
        internal.push_back((byte)OpCodes::EXTEND);
        string t = token.GetCurrentToken();
        if(t == "+")
            internal.push_back((byte)OpCodes::ADD);
        else if(t == "-")
            internal.push_back((byte)OpCodes::SUB);
        else if(t == "*")
            internal.push_back((byte)OpCodes::MUL);
        else if(t == "/")
            internal.push_back((byte)OpCodes::DIV);
        else
            throw CompilerException("Неочікувана лексема " + t, sourceFile, line);
         t1 = token.GetNextToken();
         if(token.GetTokenType() == Token::DIGIT) {
             memory_unit u;
             internal.push_back((byte)OpCodes::DIGIT);
             u.uword = atoi(t1.c_str());
             internal.push_back(u.bytes[0]);
             internal.push_back(u.bytes[1]);
             internal.push_back(u.bytes[2]);
             internal.push_back(u.bytes[3]);
         }
         if(isRegister(t1)) {
             internal.push_back((byte)get_register(t1));
         }
         if(token.GetNextToken() != "]")
             throw CompilerException("']' очікувалось! " + t1, sourceFile, line);
    }
    return internal;
}

void pushJump(OpCodes com, Token & token)
{
	commands.push_back((byte)com);
	string label = token.GetNextToken();
	if (token.GetTokenType() != Token::IDENTIFIER)
        throw CompilerException("Поганий ідентифікатор: " + label, sourceFile,
		line);

	memory_unit addr;
	if (!containsJCEntry(label))
	{
		CompilerItem c = { 0, PODTypes::LABEL, label };
		jmpstack.push_back(c);
		addr.word = jmpstack.size() - 1;
	}
	else
	{
		for (int i = 0; i < jmpstack.size(); i++)
			if (jmpstack[i].name == label)
				addr.word = i;
	}
	jumps.push_back(commands.size());
	commands.push_back(addr.bytes[0]);
	commands.push_back(addr.bytes[1]);
	commands.push_back(addr.bytes[2]);
	commands.push_back(addr.bytes[3]);
}

bool isValid1arg(OpCodes com)
{
    switch (com) {
    case OpCodes::MOV:
    case OpCodes::XCH:
    case OpCodes::INC:
    case OpCodes::DEC:
    case OpCodes::FREE:
    case OpCodes::STDIN:
        return false;
        break;
    default:
        return true;
        break;
    }
    /*if (com == OpCodes::MOV || com == OpCodes::XCH
		|| com == OpCodes::ROR || com == OpCodes::ROL
		|| com == OpCodes::SHL || com == OpCodes::SHR
		|| com == OpCodes::STDIN || com == OpCodes::INC
        || com == OpCodes::DEC || com)*/
}

void link()
{
    for (auto iter = calls.begin(); iter != calls.end();
		++iter)
	{
        if(*iter >= commands.size()) throw CompilerException(string("Неправильний ітератор в функції link(): "), sourceFile, line);
        //cout << "\tLINK: MODULE: 0x"  << hex << *iter << endl;
        if(commands[*iter] == 0) {
            ++(*iter);
			memory_unit u, t;
            u.reg = 0x0;
			u.bytes[0] = commands[*iter];
			u.bytes[1] = commands[*iter + 1];
			u.bytes[2] = commands[*iter + 2];
            u.bytes[3] = commands[*iter + 3];
            cout << "\t\t\tIn Link Block! " << dec << u.word << endl;
            //int addr = procstack[u.word].addr;
            t.uword = procstack[u.word].addr;
            cout << "\t\t\tIn Link Block 2!" << endl;
            commands[*iter] = t.bytes[0];
			commands[*iter + 1] = t.bytes[1];
			commands[*iter + 2] = t.bytes[2];
            commands[*iter + 3] = t.bytes[3];
		}
	}
	for (vector < int >::iterator jter = jumps.begin(); jter != jumps.end();
		++jter)
	{
		memory_unit u, t;
		u.bytes[0] = commands[*jter];
		u.bytes[1] = commands[*jter + 1];
		u.bytes[2] = commands[*jter + 2];
		u.bytes[3] = commands[*jter + 3];
		int addr = jmpstack[u.word].addr;
		if (jmpstack[u.word].addr == 0)
            throw LinkerException("Мітка " + jmpstack[u.word].name +
            " не визначена",
            "Вихідний з " + sourceFile, *jter);
		t.word = jmpstack[u.word].addr;
		commands[*jter] = t.bytes[0];
		commands[*jter + 1] = t.bytes[1];
		commands[*jter + 2] = t.bytes[2];
		commands[*jter + 3] = t.bytes[3];
	}
}

string getCurrentNS()
{
	string ns = "";
	if (current_ns.size() == 0)
		return "";
	else
	{
		vector < string >::iterator iter = current_ns.begin();
		ns += *iter;
		++iter;
		for (; iter != current_ns.end(); ++iter)
		{
			ns += "::" + *iter;
		}
		return ns;
	}
}

string getFullName(Token & token)
{
	// Token tok = str;
	string tmp = token.GetNextToken();
	auto s = tmp;
    if (tmp.length() > 0 && isalpha(tmp[0]) && tmp != "double" && tmp != "int"
        && tmp != "short" && tmp != "byte" && tmp != "bool" && tmp != "int64"
		&& token.GetTokenType() == Token::IDENTIFIER)
	{
		// if(token.GetNextToken() == ":");
		if ((s = token.GetNextToken()) == ":")
			if ((s = token.GetNextToken()) == ":")
			{
				tmp += "::" + (s = token.GetNextToken());
				for (s = token.GetNextToken();
					token.GetTokenType() == Token::IDENTIFIER || s == ":";
					s = token.GetNextToken())
					tmp += s;
			}
	}
	token.PushBack();
	return tmp;
}

bool isFullName(const string & str)
{
	Token tok = str;
	string tmp = tok.GetNextToken();

    if (tmp.length() > 0 && isalpha(tmp[0]) && !isPODType(tmp)
		&& tok.GetTokenType() == Token::IDENTIFIER)
	{
		// if(token.GetNextToken() == ":");
		if (tok.GetNextToken() == ":")
			if (tok.GetNextToken() == ":")
			{
				string end = tok.GetNextToken();
				tmp += "::" + end;
				for (string s = tok.GetNextToken();
					tok.GetTokenType() == Token::IDENTIFIER || s == ":";
					s = tok.GetNextToken())
				{
					tmp += s;
					end = s;
				}
				for (vector < string >::iterator iter = namespaces.begin();
					iter != namespaces.end(); ++iter)
					if (*iter + "::" + end == tmp)
						return true;
			}
	}
	return false;
}

string getFuncArgs (Token &token) {
	string tok, allArgs;
	bool firstComa = true;

	while ((tok = token.GetNextToken()) != ")")
	{
		//cout << "Tok: " << tok << endl;
		if(!firstComa) {
			if(tok != ",")
                throw CompilerException("Кома очікувалась! '" + tok + "'", sourceFile, line);
			allArgs += ",";
			tok = token.GetNextToken();
		}
		firstComa = false;

        if(isPODType(tok, true)) {
            allArgs += unlocalize(tok);
        }
		else if(tok == ".") {
			if(token.GetNextToken() == "." && token.GetNextToken() == ".") {
				allArgs += "...";
				if(token.GetNextToken() != ")")
                    throw CompilerException(") очікувалось! " + token.GetCurrentToken() , sourceFile, line);
				token.PushBack();
				return allArgs;
			}
		}
		else
            throw CompilerException("Невідомий тип: " + token.GetCurrentToken(), sourceFile, line);
		//cout << "End of tok: " << tok << endl;
	}
	token.PushBack();
	//cout << "Token PushBacked!" << endl;
	return allArgs;
}

bool isPODType(string type, bool _void) {
    if(_void && type == localize("void")) return true;
    if(type != localize("bool") && type != localize("byte") && type != localize("short") &&
        type != localize("int") && type != localize("double") && type != localize("int64") && type != localize("string"))
		return false;
	return true;
}

void addFileToPath(string filename) {
    if(!filename.find("\\") != string::npos && !filename.find("/") != string::npos) return;

	string outPath = "";
	int last;
    for(int i = 0; i < filename.length(); i++) {
		if(filename[i] == '\\' || filename[i] == '/') last = i;
	}

	for(int i = 0; i < last; i++) {
		outPath += filename[i];
	}

    outPath += "/";
	path.push_back(outPath);
	cout << "Path '" << outPath << "' added to PATH" << endl;
}

template <typename Ty> void define(PODTypes pod, Token &token) {
	//numeric_limits<int>::max()
    bool isArray = false;
	string identifier;
    Array a;
    ///if (getCurrentNS() == "")
    ///{
		// Token tok = token.GetTail();
		// identifier = getFullName(tok, S(""));
		identifier = token.GetNextToken();
        if (identifier == "[") {
            isArray = true;
            a.type = pod;
            if(token.GetNextToken() == "]") {
                a.size = -1;
            }
            else {
                a.size = atoi(token.GetCurrentToken().c_str()) * sizeof(Ty);
                if(a.size <= 0)
                    throw CompilerException("Розмір масиве має бути більше 0!", sourceFile, line);
                if(token.GetNextToken() != "]") throw CompilerException("']' очікувалось у визначенні масиву!", sourceFile, line);
            }
            identifier = token.GetNextToken();
        }
    ///}
    ///else
    ///	identifier = getCurrentNS() + "::" + token.GetNextToken();

	// cout << "\nType '" << type << "\'\nIdentifier \'" <<
	// identifier << "'\n";
	if (token.GetTokenType() != Token::IDENTIFIER)
        // throw (S("Поганий ідентифікатор: ") + identifier).ToCString();
            throw CompilerException("Поганий ідентифікатор" + identifier,
			sourceFile, line);
	if (containsCEntry(identifier) || containsFCEntry(identifier)
		|| containsJCEntry(identifier))
        throw CompilerException("Ідентифікатор " + identifier + " вже визначений", sourceFile, line);

    if(token.GetNextToken() != ":" && token.GetTokenType() != Token::EOF) throw CompilerException(": очікувалось!", sourceFile, line);

    string val;
    if(token.GetTokenType() != Token::EOF)
        val = token.GetNextToken();
    Token::TokenType t = token.GetTokenType();

    //cout << t << endl;

    if (t != Token::DIGIT && t != Token::EOF && t != Token::COMMENT && typeid(Ty) != typeid(bool))
        if(!isArray || val != "{")                                                                                      ///TESTME
            throw CompilerException("Лексема не є числом або логічним типом: " +
            val, sourceFile, line);

	Ty _trueval = 0;
	if (t == Token::DIGIT)
	{
		if(typeid(Ty) == typeid(short int) || typeid(Ty) == typeid(int)
			|| typeid(Ty) == typeid(byte) || typeid(Ty) == typeid(char))
            _trueval = atoi(val.c_str());
        else if(typeid(Ty) == typeid(__int64))
            _trueval = atoll(val.c_str());
		else if(typeid(Ty) == typeid(double))
            _trueval = atof(val.c_str());
		//if (_trueval > 0xFFFF)
		//	throw
		//	CompilerException(S
		//	("Number is larger then size of half word (0xFFFF): ")
		//	+ val, sourceFile, line);
	}
    else if (t == Token::IDENTIFIER)
	{
        //();
        if (val == "true")
			_trueval = true;
        else if (val != "false")
            throw CompilerException("Лексема не є логічним типом: " +
			val, sourceFile, line);
	}
    else if(isArray && t != Token::EOF) {
        int counter = 0;
        for(int i = 0; val != "}"; i++) {
            val = token.GetNextToken();
            //if (val == "}") break;

            memory_unit u;

            if(typeid(Ty) == typeid(short int) || typeid(Ty) == typeid(int)
                || typeid(Ty) == typeid(byte) || typeid(Ty) == typeid(char)) {
                if(token.GetTokenType() != Token::DIGIT)
                    throw CompilerException("Лексема не є числом: " + val, sourceFile, line);
                u.word = atoi(val.c_str());
                for (int i = 0; i < sizeof(Ty); i++)
                    a.items.push_back(u.bytes[i]);
                ++counter;

            }
            else if(typeid(Ty) == typeid(__int64)) {
                if(token.GetTokenType() != Token::DIGIT)
                    throw CompilerException("Лексема не є числом: " + val, sourceFile, line);
                u.reg = atoll(val.c_str());
                for (int i = 0; i < sizeof(__int64); i++)
                    a.items.push_back(u.bytes[i]);
                ++counter;
            }
            else if(typeid(Ty) == typeid(double))
            {
                if(token.GetTokenType() != Token::DIGIT)
                    throw CompilerException("Лексема не є числом: " + val, sourceFile, line);
                u.dword = atof(val.c_str());
                for (int i = 0; i < sizeof(double); i++)
                    a.items.push_back(u.bytes[i]);
                ++counter;

            }
            else if(typeid(Ty) == typeid(bool))
            {
                u.bytes[0] = false;
                if (val == localize("true"))
                    u.bytes[0] = true;
                else if (val != localize("false"))
                    throw CompilerException("Лексема не є логічним типом: " + val, sourceFile, line);
                a.items.push_back(u.bytes[0]);
                ++counter;
            }

            if (token.GetNextToken() == "}") break;
            if (token.GetCurrentToken() != ",")
                throw CompilerException("Кома очікувалась! " + token.GetCurrentToken(), sourceFile, line);
        }
        if(a.size == -1) a.size = counter * sizeof(Ty);
        //cout << "Type of array " << identifier << " is " << (byte)a.type << " size: " << a.size << endl;
        for (auto iter = a.items.begin(); iter != a.items.end(); ++iter) {
            //cout << (int)*iter << " ";
        }
    }
    if(a.size == -1) throw CompilerException ("В масиві не зазначений ні розмір ні список ініціалізації! ", sourceFile, line);
	memory_unit unit;
    unit.reg = 0;
    //unit.hword = (Ty)_trueval; //////// ------------------------- ???
    if(typeid(Ty) == typeid(short int) || typeid(Ty) == typeid(int)
        || typeid(Ty) == typeid(byte) || typeid(Ty) == typeid(char))
        unit.word = _trueval;
    else if(typeid(Ty) == typeid(__int64))
        unit.reg = _trueval;
    else if(typeid(Ty) == typeid(double))
        unit.dword = _trueval;
    else if(typeid(Ty) == typeid(bool))
        unit.bytes[0] = (byte)(unsigned int)_trueval;
    //unit.hword = _trueval;
    unit.bytes[8] = (byte)pod;
	CompilerItem entry;
    entry.type = pod;
    if(isArray) {
        arrays.push_back(a);
        unit.uword = arrays.size() - 1;
        entry.addr = program_stack.size();//arrays.size() - 1;
        entry.type = PODTypes::POINTER;
        unit.bytes[8] = (byte)PODTypes::POINTER;
        program_stack.push_back(unit);
    }
    else {
        entry.addr = program_stack.size();
        program_stack.push_back(unit);
    }
	entry.name = identifier;
	cstack.push_back(entry);
    globals.insert(pair <string, int >(identifier, program_stack.size() - 1));

}

string localize(string key) {
    if(curLocale == CodeLocale::English ||
            (curLocale == CodeLocale::Default && CodeLocale::Default == CodeLocale::English)) {
        return key;
    }
    if(curLocale == CodeLocale::Ukrainian) return localize_UA(key);
    if(curLocale == CodeLocale::Latin) return localize_LA(key);
}

string localize_UA(string key) {
    //cout << "UA: key=" << key << endl;
    if(key == ".public") return ".публічний";
    if(key == ".private") return ".приватний";
    if(key == ".entry") return ".вхід";
    if(key == ".static") return ".статик";
    if(key == ".load") return ".імпорт";
    if(key == ".binding") return ".обгортка";
    if(key == ".type") return ".тип";
    if(key == ".end") return ".кінець";

    if(key == "native") return "нативний";

    if(key == "prot") return "прототип";
    if(key == "def") return "змінна";

    if(key == "sizeof") return "розм";
    if(key == "addr") return "адр";
    if(key == "true") return "так";
    if(key == "false") return "ні";

    if(key == "void") return "воід";
    if(key == "bool") return "буль";
    if(key == "byte") return "байт";
    if(key == "short") return "коротке";
    if(key == "int") return "ціле";
    if(key == "int64") return "довге";
    if(key == "double") return "подвійне";
    if(key == "string") return "рядок";
    if(key == "library") return "бібліотека";
    if(key == "executable") return "програма";
}

string localize_LA(string key) {
    //cout << "UA: key=" << key << endl;
    if(key == ".public") return ".publica";
    if(key == ".private") return ".privatis";
    if(key == ".entry") return ".introitus";
    //if(key == ".static") return ".статик";
    if(key == ".load") return ".onero";
    if(key == ".binding") return ".alligans";
    if(key == ".type") return ".type";
    if(key == ".end") return ".finus";

    if(key == "native") return "patria";

    if(key == "prot") return "exemplar";
    if(key == "def") return "defino";

    if(key == "sizeof") return "probo";
    if(key == "addr") return "oratio";
    if(key == "true") return "verum";
    if(key == "false") return "falsus";

    if(key == "void") return "inane";
    if(key == "bool") return "logicus";
    if(key == "byte") return "byte";
    if(key == "short") return "brevis";
    if(key == "int") return "integer";
    if(key == "int64") return "longis";
    if(key == "double") return "geminus";
    if(key == "string") return "chorda";
    if(key == "library") return "bibliotheca";
    if(key == "executable") return "exequor";
}

string unlocalize(string key) {
    if(curLocale == CodeLocale::English ||
            (curLocale == CodeLocale::Default && CodeLocale::Default == CodeLocale::English)) {
        return key;
    }
    if(curLocale == CodeLocale::Ukrainian) return unlocalize_UA(key);
    if(curLocale == CodeLocale::Latin) return unlocalize_LA(key);
}

string unlocalize_UA(string key) {
    //cout << "UA: key=" << key << endl;
    if(key == "воід") return "void";
    if(key == "буль") return "bool";
    if(key == "байт") return "byte";
    if(key == "коротке") return "short";
    if(key == "ціле") return "int";
    if(key == "довге") return "int64";
    if(key == "подвійне") return "double";
    if(key == "рядок") return "string";
}

string unlocalize_LA(string key) {
    //cout << "UA: key=" << key << endl;
    if(key == "inane") return "void";
    if(key == "logicus") return "bool";
    if(key == "byte") return "byte";
    if(key == "коротке") return "short";
    if(key == "integer") return "int";
    if(key == "longis") return "int64";
    if(key == "geminus") return "double";
    if(key == "chorda") return "string";
}

PODTypes stringToType(const string &str) {
    if(str == localize("bool")) return PODTypes::BOOL;
    else if(str == localize("byte")) return PODTypes::BYTE;
    else if(str == localize("short")) return PODTypes::HWORD;
    else if(str == localize("int")) return PODTypes::WORD;
    else if(str == localize("string")) return PODTypes::STRING;
    else if(str == localize("int64")) return PODTypes::W64;
    else if(str == localize("double")) return PODTypes::DWORD;
    return PODTypes::_NULL;
}










