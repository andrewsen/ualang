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

#ifndef MODULE_H
#define MODULE_H

#include <vector>
//#include "String.h"
#include "asm_keywords.h"
#include <fstream>
//#include "parser_common.h"

using namespace std;
typedef unsigned char byte;

extern vector<string> path;

union _flags
{
	struct 
	{
		unsigned canExecute : 1;
		unsigned isKernelModule : 1;
		unsigned canBeLinked : 1;
		unsigned useOsAPI : 1;
		unsigned isBinding : 1;
		unsigned reserved : 3;
	} ;
	byte value;
};

class ModuleException
{
public:
	ModuleException(string m, string r, string e="") : reason(r), modName(m), extra(e)
	{ }

	string GetName() { return modName; }
	string GetReason() { return reason; }
	string GetExtra() { return extra; }

private:
	string reason, modName, extra;
};


class Module
{
	bool addExt, valid;
	string name;
	string reason;
	_flags flags;

	int compiler_ver;
	int target_ver;
	int min_ver;
	int max_ver;

protected:
	ifstream module;
	string findModule();
	void throw_(string reason, string extra="") throw(ModuleException);
	int readInt();
public:
	
	static const char * DEF_LIB_EXT;// = ".slm";
	static const char * DEF_EXE_EXT;// = ".sem";


	class Function
	{
		string sign;
		string retType;
		unsigned int addr;

		friend class Module;

	public:
		string GetSignature() { return sign; }
		string GetReturnType() { return retType; }
		unsigned int GetAddress() { return addr; }

	private:
		Function(string _sign, string _retType, unsigned int _addr) : sign(_sign), retType(_retType), addr(_addr)
		{ }
	};

	Module(string name, bool addExt=false);
	Module(const Module &o) {
		//module = &o.module;
		compiler_ver = o.compiler_ver;
		target_ver = o.target_ver;
		min_ver = o.min_ver;
		max_ver = o.max_ver;
		reason = o.reason;
		addExt = o.addExt;
		flags = o.flags;
		valid = o.valid;
		funcs = o.funcs;
		name = o.name;
	}

	_flags GetFlags();

	bool canExecute();
	bool isBinding();
	bool canBeLinked();
	bool useOsApi();
	bool isKernelModule();

	string GetName();
	vector<Function> GetFunctions();

	virtual ~Module(void);

private:
	vector<Function> funcs;
};

#endif
