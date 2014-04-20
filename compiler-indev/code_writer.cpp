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

#include "parser_common.h"
#include "modulefile.h"

void push_int(int num);
void push_string(const string &str);
vector<unsigned char> compiled_exe;
vector<unsigned char> compiled_funcs;
vector<unsigned int>real_addr_table;
vector<unsigned int>real_array_table;
vector<unsigned int>real_string_table;
vector<Module> imports;
bool newFormat = false;

MetaSegment meta("meta");
//bool canExecute;

int getFuncTabSize();

/*
	.magic
	.flags (byte): 
		1: can execute
		1: is kernel module
		1: can be used as library
		1: use os-specific API
		1: is native-binding library
		3: reserved
	.ptr table
		.stack ptr
		.commands ptr
		.functions ptr
	.program stack
	.commands
	.functions (signature + addr [dword foo(word,string) + 0x89ABCDEF])
	.metadata
*/

void write() {
    compiled_exe.clear();
    compiled_funcs.clear();
    //ModuleFile file;
    //file.SetFlags(modFlags.value);
    
    if(newFormat) {
        string ext;
        if(modFlags.canExecute && !isOutSpecified) ext =  Module::DEF_EXE_EXT;
        else if(!isOutSpecified) ext = Module::DEF_LIB_EXT;
        ModuleFile file(moduleName, ext);

        meta.AddMeta(MetaItem("module", moduleName));
        if(modFlags.canExecute && entryFunc.name != "") {
            for( int i = 0; i < procstack.size(); i++) {
                if(procstack[i].name == entryFunc.name) {
                    meta.AddMeta(MetaItem("entry", procstack[i].addr));
                    break;
                }
            }
        }


        Segment rdt("rdt", true);
        Segment rrt("rrt", true);
        Segment rst("rst", true);
        for(int i : real_addr_table) {
            memory_unit u;
            u.word = i;
            rdt.AddData(u.bytes[0]);
            rdt.AddData(u.bytes[1]);
            rdt.AddData(u.bytes[2]);
            rdt.AddData(u.bytes[3]);
        }
        for(int i : real_array_table) {
            memory_unit u;
            u.word = i;
            rdt.AddData(u.bytes[0]);
            rdt.AddData(u.bytes[1]);
            rdt.AddData(u.bytes[2]);
            rdt.AddData(u.bytes[3]);
        }
        for(int i : real_string_table) {
            memory_unit u;
            u.word = i;
            rdt.AddData(u.bytes[0]);
            rdt.AddData(u.bytes[1]);
            rdt.AddData(u.bytes[2]);
            rdt.AddData(u.bytes[3]);
        }

        vector<byte>st;
        for(memory_unit u : program_stack) {
            st.push_back(u.bytes[0]);
            st.push_back(u.bytes[1]);
            st.push_back(u.bytes[2]);
            st.push_back(u.bytes[3]);
            st.push_back(u.bytes[4]);
            st.push_back(u.bytes[5]);
            st.push_back(u.bytes[6]);
            st.push_back(u.bytes[7]);
            st.push_back(u.bytes[8]);
        }

        MetaSegment imps("imports", true);

        for(Module m : imports)
            imps.AddMeta(m.GetName());


        Segment stack("stack", true);
        stack.AddData(st);

        Segment arr("arrays", true);

        for(Array a : arrays) {
            arr.AddData((byte)a.type);
            memory_unit u;
            u.word = a.size;
            arr.AddData(u.bytes[0]);
            arr.AddData(u.bytes[1]);
            arr.AddData(u.bytes[2]);
            arr.AddData(u.bytes[3]);
            arr.AddData(a.items);
        }

        Segment code("code", true);
        code.AddData(commands);

        MetaSegment funcs("funcs");
        for(auto iter = procstack.begin(); iter != procstack.end(); ++iter) {
            if(!iter->_private) {
                funcs.AddMeta(iter->returnType + " " + iter->name, iter->addr);
            }
        }

        MetaSegment strs("strings", true);
        for(string str : str_stack) {
            //cout << "Writing string to meta: " << str << endl;
            strs.AddMeta(str);
        }

        file.AddSegment(meta);
        file.AddSegment(rdt);
        file.AddSegment(rrt);
        file.AddSegment(rst);
        //if(!imports.empty())
        file.AddSegment(imps);
        file.AddSegment(stack);
        //if(!arrays.empty())
        file.AddSegment(arr);
        file.AddSegment(code);
        //if(!procstack.empty())
        file.AddSegment(funcs);
        file.AddSegment(strs);

        file.Write();

        return;
    }

    if(modFlags.canExecute && !isOutSpecified) outName = moduleName + "." + Module::DEF_EXE_EXT;
    else if(!isOutSpecified) outName = moduleName + "." + Module::DEF_LIB_EXT;
	cout << "OUT: " << outName << endl;
	compiled_exe.push_back('V');
	compiled_exe.push_back('A');
	compiled_exe.push_back('S');
	compiled_exe.push_back('M');
	
    push_int(0x7); //4x4
	push_int(0x1);
	push_int(0x1);
    push_int(0x0);

	compiled_exe.push_back(modFlags.value);

	// Начало таблицы указателей
	//push_int(25 + entryFunc.name.Length()+4);
    int beg = 53;
	
	for(auto m : imports) 
        beg += m.GetName().length()+1;
    //cout << "Total module strings: " << beg - 37 << endl;

	if(modFlags.canExecute && entryFunc.name != "") {
		for( int i = 0; i < procstack.size(); i++) {
			if(procstack[i].name == entryFunc.name) {
                //cout << "entry addr: " << i << endl;
                push_int(procstack[i].addr);
				break;
			}
		}
		beg += 4;
	}

	//auto name = entryFunc.name;
	//push_int(entryFunc.addr);
	//for(int index = 0; index < name.Length(); index++)
	//	compiled_exe.push_back(name.operator[](index));

    vector<byte> temp;
    for(Array a : arrays) {
        //cout << "ARRAY type: " << (int)a.type << endl;
        memory_unit u;
        temp.push_back((byte)a.type);
        u.word = a.size;
        temp.push_back(u.bytes[0]);
        temp.push_back(u.bytes[1]);
        temp.push_back(u.bytes[2]);
        temp.push_back(u.bytes[3]);
        u.word = a.items.size();
        temp.push_back(u.bytes[0]);
        temp.push_back(u.bytes[1]);
        temp.push_back(u.bytes[2]);
        temp.push_back(u.bytes[3]);
        temp.insert(temp.end(), a.items.begin(), a.items.end());
    }
    int real_addr_ptr = beg;
    int real_array_ptr = real_addr_ptr + real_addr_table.size()*4;
    int real_string_ptr = real_array_ptr + real_array_table.size()*4;
    int stack_ptr = real_string_ptr + real_string_table.size()*4;
    int array_ptr = stack_ptr+program_stack.size()*9;
    int com_ptr = array_ptr + temp.size();
	int fun_ptr = com_ptr+commands.size(); 
	int str_ptr = fun_ptr + getFuncTabSize();

	//int str_ptr = str_tab_ptr + str_stack_table.size()*4;
    cout << "Вказівники:" << endl;
    cout << "RDP " << real_addr_ptr << endl;

    cout << "RRP " << real_array_ptr << endl;

    cout << "RSP " << real_string_ptr << endl;

    cout << "SP " << stack_ptr << endl;

    cout << "AP " << array_ptr << endl;

    cout << "COM " << com_ptr << endl;
	//cout << fun_tab_ptr << endl;
    cout << "FUN " << fun_ptr << endl;
	//cout << str_tab_ptr << endl;
    cout << "STR " << str_ptr << endl;

    push_int(real_addr_ptr); // Указатель на стек
    push_int(real_array_ptr); // Указатель на стек
    push_int(real_string_ptr); // Указатель на стек
    push_int(stack_ptr); // Указатель на стек
    push_int(array_ptr); // Указатель на стек
	push_int(com_ptr); // Указатель на память команд
	//push_int(fun_tab_ptr);  // Указатель на таблицу функций
	push_int(fun_ptr);  // Указатель на таблицу имен и адресов функций
	//push_int(str_tab_ptr);  // Указатель на таблицу строк
	push_int(str_ptr);  // Указатель на строковой пул
	// Конец таблицы указателей

    /*
     * Array 1:
     *  type: I
     *  size: x
     *  act_size: n
     *  items: n
     *
     * Array 2:
     *  type: D
     *  size: x
     *  act_size: n
     *  items: n
     */


	for(auto m : imports) 
		push_string(m.GetName());

    for(int i : real_addr_table) {
        push_int(i);
    }

    for(int i : real_array_table) {
        push_int(i);
    }
    for(int i : real_string_table) {
        push_int(i);
    }

	for(auto iter = program_stack.begin(); iter != program_stack.end(); ++iter)
	{
		compiled_exe.push_back(iter->bytes[0]);
		compiled_exe.push_back(iter->bytes[1]);
		compiled_exe.push_back(iter->bytes[2]);
		compiled_exe.push_back(iter->bytes[3]);
		compiled_exe.push_back(iter->bytes[4]);
		compiled_exe.push_back(iter->bytes[5]);
		compiled_exe.push_back(iter->bytes[6]);
		compiled_exe.push_back(iter->bytes[7]);
		compiled_exe.push_back(iter->bytes[8]);
    }
    compiled_exe.insert(compiled_exe.end(), temp.begin(), temp.end());
	for(auto iter = commands.begin(); iter != commands.end(); ++iter)
		compiled_exe.push_back(*iter);
	//for(auto iter = procstack.begin(); iter != procstack.end(); ++iter)
	//	push_int(iter->name.Length()+4);
	for(auto iter = procstack.begin(); iter != procstack.end(); ++iter) {
        if(!iter->_private) {
            auto name = iter->name;
            auto type = iter->returnType;
            push_int(iter->addr);
            for(int index = 0; index < type.length(); index++)
                compiled_exe.push_back(type[index]);
            compiled_exe.push_back(' ');
            for(int index = 0; index < name.length(); index++)
                compiled_exe.push_back(name[index]);
            compiled_exe.push_back(0);
        }
	}
	//for (auto iter = str_stack_table.begin(); iter != str_stack_table.end(); ++iter)
	//	push_int(*iter);
    ofstream ofs(outName, ios::binary | ios::trunc);
	for(auto iter = compiled_exe.begin(); iter != compiled_exe.end(); ++iter)
		ofs << *iter;
	for (auto iter = str_stack.begin(); iter != str_stack.end(); ++iter) {
		ofs << *iter << (byte)0;
	}
	ofs.close();
}

void push_int(int num) {
	memory_unit u;
	u.word = num;
	//cout << "INT: " << u.word << endl;
	compiled_exe.push_back(u.bytes[0]);
	compiled_exe.push_back(u.bytes[1]);
	compiled_exe.push_back(u.bytes[2]);
	compiled_exe.push_back(u.bytes[3]);
}


void push_string(const string &str) {
    for(int i = 0; i < str.length(); i++)
		compiled_exe.push_back(str[i]);
	compiled_exe.push_back(0);
}

int getFuncTabSize() {
	int s = 0;
	for (CompilerItem e : procstack) {
        if(!e._private)
            s += 4 + e.returnType.length() + 1 + e.name.length() + 1;
	}
	return s;
}
