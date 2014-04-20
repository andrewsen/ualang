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

#include "Module.h"
#include "parser_common.h"
#include <fstream>

vector<string> path;

const char * Module::DEF_LIB_EXT = "slm";
const char * Module::DEF_EXE_EXT = "sem";

Module::Module(string _name, bool _addExt)
{
	this->name = _name;
	//if(_addExt) this->name += ".";
	//C:\folder\file.sky
	//C:\folder

    addExt = true;
    auto modPath = findModule();
    if(modPath == "") {
        addExt = false;
        modPath = findModule();
    }

	if(modPath == "") {
		string _path = "Searched in:\n";
        for(auto p : path)
            _path = _path + "\t" + p + "\n";
        _path = _path + "\t" + "and in compiler directory\n";
		throw_("Module " + _name + " does not exists!", _path);
	}

    module.open(modPath, ios::in);

    if(!module) throw_("Invalid module " + modPath + "!");

	char vasm [5];
	module.seekg(0);
	module.get(vasm[0]);
	module.get(vasm[1]);
	module.get(vasm[2]);
	module.get(vasm[3]);
	vasm[4] = '\0';

    if(strcmp(vasm, "VASM")) throw_("Invalid magic! " + string(vasm));

	compiler_ver = readInt();
	target_ver = readInt();
	min_ver = readInt();
	max_ver = readInt();

	module.get((char&)flags.value);

	int entry = -1;

	if(flags.canExecute) entry = readInt();

    int real_addr_ptr = readInt(),
            real_array_ptr = readInt(),
            real_string_ptr = readInt(),
        stack_ptr = readInt(),
        array_ptr = readInt(),
        com_ptr = readInt(),
		fun_ptr = readInt(),
		str_ptr = readInt();
    //cout << "Fun ptr: " << hex << fun_ptr << endl;
	module.seekg(fun_ptr);

	int temp;
    while ((temp = module.tellg()) < str_ptr && !module.eof())
	{
		unsigned addr = readInt();
		string ret, sign;
		char ch;
		module.get(ch);
		while(ch != ' ') {
            ret += ch;
			module.get(ch);
		}
		
		module.get(ch);
		while(ch != 0) {
            sign += ch;
			module.get(ch);
		}
        //cout << hex << addr << " " << ret << " " << sign << endl;
		Function func(sign, ret, addr);
		funcs.push_back(func);
	}

	module.close();
}

string Module::findModule() {
	string _name = this->name;
    if(this->addExt) _name += string(".") + DEF_LIB_EXT;

    module.open(_name);
    if(module) {
        module.close();
        return _name;
    }

	for(string p : path) {
        cout << "Пошук " << _name << " in " << p << endl;
        ifstream module(p + _name);
		if(module) return p + _name;
	}

	return "";
}

_flags Module::GetFlags() 
{
	return flags;
}

bool Module::canExecute()
{
	return flags.canExecute;
}
bool Module::isBinding()
{
	return flags.isBinding;
}
bool Module::canBeLinked()
{
	return flags.canBeLinked;
}
bool Module::useOsApi()
{
	return flags.useOsAPI;
}
bool Module::isKernelModule()
{
	return flags.isKernelModule;
}

string Module::GetName()
{
	return name;
}

vector<Module::Function> Module::GetFunctions()
{
	return funcs;
}

void Module::throw_(string reason, string extra) throw(ModuleException)
{
	if(module.is_open()) this->module.close();
	throw ModuleException(this->name, reason, extra);
}

int Module::readInt() {
	memory_unit u;
	
	module.get((char&)u.bytes[0]);
	module.get((char&)u.bytes[1]);
	module.get((char&)u.bytes[2]);
	module.get((char&)u.bytes[3]);

	return u.word;
}

Module::~Module(void)
{
}

