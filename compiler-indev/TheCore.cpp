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


#include <iostream>
#ifdef WIN32
    #include <conio.h>
    #include <Windows.h>
#endif
//#include "String.h"
#include "parser_common.h"
#include <iomanip>
#include <math.h>
#include <locale>
#include <getopt.h>
#include <cstring>
#include <cstdio>



//#include 

using namespace std;
void print(string &file);
void compileProject(string proj);
int compileFile(string sfile);
void readPath(string pathFile);
string outName;
string moduleName;
extern string bindingLib;
extern CodeLocale curLocale;
bool isOutSpecified = false;

_flags modFlags;

vector<ProjectItem> proj_constants;
vector<string> proj_files;
vector<string> namespaces;
vector<string> current_ns;

extern bool newFormat;

void cleanAll();

int main(int argc, char* argv[])
{
    //cin.get();
    //QCoreApplication a(argc, argv);

    //return a.exec();
	//_flags f;
	//f.canExecute = 2;
	//cout << "f:" << hex << (int)f.value << endl;
#ifdef WIN32
    //SetConsoleCP(65001);
    //SetConsoleOutputCP(65001);
    //system("chcp 65001");
#endif
    ///ifs.imbue(locale(".866"));
    //cout << "Senko's Virtual Assembler v0.7-alpha1. UTF-8: включен" << endl;
	modFlags.canExecute = true;
	modFlags.canBeLinked = true;
	modFlags.isBinding = false;
	modFlags.isKernelModule = false;
	modFlags.useOsAPI = false;
	modFlags.reserved = 0;
	
    //cout << sizeof(modFlags) << endl;
	 
	entryFunc.name = "";
    entryFunc.addr = -1;
    //fcin.get();
    path.push_back("/usr/share/simple-rt/modules/");
    path.push_back(getenv("PWD") + string("/"));
#ifdef _DEBUG
    //system("cd /home/senko/qt/Framework/build-compiler-indev-Desktop-Debug");
    path.push_back("/home/senko/qt/Framework/build-compiler-indev-Desktop-Debug/modules/");
    string s;
    //cin.get();
    getline(cin, s);
    compileFile(s.erase(0,1));
	//compileProject(s);
    //compileFile("C:\\Users\\User\\Desktop\\mod.sky");
    //compileFile("C:\\Users\\User\\Desktop\\test.sky");
	cin.get();
	return 0;
#else
    //compileFile("test.asm");
#endif
        //return 0;
	if(argc == 1) {
        cout << " file.sky [-o out]";
		//cin.get();
		cout << endl;
		return 1;
    }
    const char* short_options = "o:p:hv";

    const struct option long_options[] = {
        {"help",no_argument,NULL,'h'},
        {"version",no_argument,NULL,'v'},
        {"out",required_argument,NULL,'o'},
        {"proj",no_argument,NULL,'p'},
        {"entry",required_argument,NULL,'e'},
        {"locale",required_argument,NULL,'l'},
        {"format",required_argument,NULL,'f'},
        {NULL,0,NULL,0}
    };

    int rez;
    int option_index;

    while ((rez = getopt_long(argc, argv, short_options,
        long_options, &option_index)) !=- 1){

        switch (rez)
        {
            case 'h': {
                cout << "Senko's Assembler v0.1-beta. UTF-8: включен" << endl << endl;
                cout << argv[0] << " [ПАРАМЕТР] <file.sky>" << endl << endl;

                printf("  %-2s, %-10s %s", "-h", "--help", "показати довідку\n");
                printf("  %-2s, %-10s %s", "-o", "--out", "вказати вихідний файл\n");
                printf("  %-2s, %-10s %s", "-v", "--version", "показати версію\n");
                //printf("  %-2s, %-10s %s", "-p", "--proj", "compile binary from project\n");
                printf("  %-2s  %-10s %s", "", "--locale", "переключити синтаксис. Можливі параметри: eng, ukr або укр\n");
                return 0;
            }
            case 'o': {
                isOutSpecified = true;
                outName = optarg;
                break;
            }
            case 'v': {
                cout << "Senko's Assembler v0.1-beta\n";
                return 0;
            }
            case 'p': {
                compileProject(optarg);
                return 0;
            }
            case 'f': {
                string a = optarg;
                if(a == "2.0" || a == "segmented")
                    newFormat = true;
                else if(a == "1.0" || a == "flat")
                    newFormat = false;
                else {
                    cerr << "Unknown format " << a << endl;
                    exit(EXIT_FAILURE);
                    return EXIT_FAILURE;
                }
            }

            case 'l': {
                if(string(optarg) == "eng") curLocale = CodeLocale::English;
                else if(string(optarg) == "ukr" || string(optarg) == "укр")  curLocale = CodeLocale::Ukrainian;
                else if(string(optarg) == "latin")  curLocale = CodeLocale::Latin;
                break;
            }
            case '?':
            default: {
                printf("found unknown option\n");
                return 1;
            }
        }
    }

    readPath("path.txt");//lang/bin/modules/
                            //and/bin/std/

	string source = argv[1];
        cout << argv[argc-1] << endl;
	if(argc > 1 && !strcmp(argv[1], "-p")) {
		//cout << "PRINT!" << endl;
		print(source);
		//cin.get();
		cout << endl;
		return 0;
	}
	
	//cout << argv[1] << endl;
    else if(argc > 1 && !strcmp(argv[1], "--proj")) {
		//cout << "PROJECT!" << endl;
		compileProject(argv[2]);
	}
    else {
        char ** files = argv + optind;
        int fcount = argc - optind;
        //cout << "FILE!" << endl;
        for(int i = 0; i < fcount; ++i) {
            compileFile(files[i]);
            cleanAll();
        }
	}

	//cin.get();
		cout << endl;
	return 0;
}

void compileProject(string proj) {
	try {
		cout << proj << endl;
		ifstream ifs;
		ifs.open(proj);
		if(!ifs) {
			cout << "Very, very bad project file! :'(";
			//cin.get();
                cout << endl;
			return;
		}
		string str = "", sproj = proj;
		
        //if(!sproj.Contains("/")) return;

		string outPath = "";
		int last;
        for(int i = 0; i < sproj.length(); i++) {
            if(sproj[i] == '/' || sproj[i] == '/') last = i;
		}

		for(int i = 0; i < last; i++) {
            outPath += sproj[i];
		}

        outPath += "/";

		bool isSources = false;
		while (!ifs.eof())
		{
            ifs >> str;
            str = trim(str);
            Token tok = str;
			//if(str.StartsWith("$")) {
			//	ProjectItem e;
			//	Token tok = str;
			//	e.key = tok.GetNextToken();
			//	if(tok.GetNextToken() != "=") throw "'=' Excepted";
			//	e.value = tok.GetTail().Trim();
			//	proj_constants.push_back(e);
			//	string str;
			//	str.replace(" ", "s ");
			//	//proj_constants;
			//}
            string com = tok.GetNextToken();

			if (com == "}") {
				if(isSources) isSources = false;
			}

			//if(isSources) {
			//		proj_files.push_back(com + tok.GetTail());
			//}

			if(isSources) {
                if(str.find_last_of("\r")) str = str.erase(str.length()-1, str.length());
                proj_files.push_back(outPath + str);
			}
			else if (com == "out") {
				if(tok.GetNextToken() != "=") throw "'=' Excepted in line " + str;
				outName = tok.GetTail();
			}
			else if (com == "sources")
			{
				if(tok.GetNextToken() != "=") throw "'=' Excepted in line " + str;
				if(tok.GetNextToken() != "{") throw "'{' Excepted in line " + str;

				isSources = true;
			}
			else if (com == "type")
			{
				if(tok.GetNextToken() != "=") throw "'=' Excepted in line " + str;
                string type = tok.GetNextToken();
				if(type == "executable") modFlags.canExecute = true;
				else if(type == "library") modFlags.canExecute = false;
				else throw "Unknown type!";
			}
			else if (com == "binding")
			{
				modFlags.isBinding = true;
				modFlags.canExecute = false;
				if(tok.GetNextToken() != "=") throw "'=' Excepted in line " + str;
				bindingLib = tok.GetNextToken();
				if(tok.GetTokenType() != Token::STRING)
                    throw CompilerException("string with dll name expected!");
				str_stack.push_back(bindingLib);
			}
			else if (com == "module")
			{
				if(tok.GetNextToken() != "=") throw "'=' Excepted in line " + str;
				//moduleName = tok.GetTail();			
				string modName;
				string t = tok.GetNextToken();
				bool addExt = true;
				if(tok.GetTokenType() == Token::STRING)
					modName = t;
				else if(tok.GetTokenType() == Token::IDENTIFIER)
				{
					modName = t;
					string tmp = tok.GetNextToken();
					while (tok.GetTokenType() != Token::EOF)
					{
						if(tok.GetCurrentToken() != ".") 
                            throw CompilerException(".(dot) expected!");
						modName += ".";
						modName += tok.GetNextToken();
						if(tok.GetTokenType() != Token::IDENTIFIER) 
                            throw CompilerException("Bad identifier: " + tok.GetCurrentToken());
						tok.GetNextToken();
					}
					if(tok.GetCurrentToken() == ".") 
                        throw CompilerException("Unexpected dot!");
					addExt = false;
				}
				outName = outPath + modName;
				moduleName = outName;
			}
			else if (com == "entry")
			{
				if(entryFunc.name != "") throw "Entry point was already defined! In line " + str;
				if(tok.GetNextToken() != "=") throw "'=' Excepted in line " + str;
                entryFunc.name = trim(tok.GetTail());
				if(tok.GetTokenType() != Token::IDENTIFIER) throw "Not Identifier at entry in line " + str;
			}
			
		}//vasmc -proj p.vaproj
		ifs.close();// __builtin_alignof __newslot __noop __nounwind __novtordisp __ptr32 __value __typeof __restrict __resume;
		parserEntry("", true);
		write();

		cout << endl << "------" << endl;
		int i = 0;
        for(vector<unsigned char>::iterator iter = commands.begin(); iter != commands.end(); ++iter) {
			cout << setw(4)  << (int)*iter << " ";
			i++;
			if(i == 16) {
				i = 0;
				cout << endl;
			}
		}
	} catch (const char* str) {
		cout << str << endl;
	} catch (string str) {
		cout << str << endl;
	} catch (CompilerException e) {
		cout << e.getMessage() << endl;
	}
}

void cleanAll() {
    commands.clear();
    program_stack.clear();
    str_stack.clear();
    outName;
    moduleName;
    bindingLib;
    curLocale;
    isOutSpecified = false;

    proj_constants.clear();
    proj_files.clear();
    namespaces.clear();
    current_ns.clear();
    path.clear();

    modFlags.canExecute = true;
    modFlags.canBeLinked = true;
    modFlags.isBinding = false;
    modFlags.isKernelModule = false;
    modFlags.useOsAPI = false;
    modFlags.reserved = 0;

    entryFunc.name = "";
    entryFunc.addr = -1;
    path.push_back("/usr/share/simple-rt/modules/");
    path.push_back(getenv("PWD") + string("/"));

    outName = "";
    moduleName = "";

    globals.clear();
    calls.clear();
    jumps.clear();
    str_stack_table.clear();
    cstack.clear();
    procstack.clear();
    jmpstack.clear();
    imports.clear();
    current_ns.clear();
    arrays.clear();
    proj_files.clear();
}

int compileFile(string sfile) {
    //cout << file << endl;
	try {
        //long long long int it;
        //string sfile = file;

        //outName = moduleName;
        if(sfile.find('/') != string::npos) {
            int slashp = sfile.find_last_of('/');
            string fpath = "";

            for(int i = 0; i < slashp; ++i) {
                fpath += sfile[i];
            }
            fpath += '/';
            path.push_back(fpath);
            cout << "PATH TO FILE " << sfile << " IS " << fpath << endl;
        }
        else
            cout << "PATH TO FILE " << sfile << " IS " << getenv("PWD") << endl;
        parserEntry(sfile, false);
        if(entryFunc.addr == -1 && modFlags.canExecute) throw CompilerException("Entry function '" + entryFunc.name + "' was not defined!",
			sfile, -1);
        if(sfile.find_last_of(".sky") == sfile.length() - 1 || sfile.find_last_of(".asm") == sfile.length() - 1)  {
            sfile.erase(sfile.length()-4);
        }
        moduleName = sfile;
        write();
        //cout << "outName: " << sfile << endl;

		cout << endl << "------" << endl;
        cout << "Кількість рядків: " << str_stack.size() << endl;
        cout << "Байткод (в ./out.bc):" << hex << endl;
		int i = 0;
        ofstream bc("out.bc");
        for(vector<unsigned char>::iterator  iter = commands.begin(); iter != commands.end(); ++iter) {
            cout << setw(3) << (int)*iter << " ";
            bc.put((int)*iter);
			i++;
			if(i == 16) {
				i = 0;
				cout << endl;
			}
		}
    bc.close();
	} catch (CompilerException cx) {
        cout << dec;
        cout << "Компіляція перервана!" << endl;
		//return 1;
        cout << "Помилка компіляциї: " << cx.getMessage() << endl
             << "\tу файлі " << cx.getSource() << " в рядку " << cx.getLine() << endl;
		return 1;
	} catch (LinkerException lx) {
        cout << dec;
        cout << "Помілка лінкування: " << lx.getMessage() << endl
             << "\tу файлі" << lx.getSource()
             << " позийія в сегменті коду 0x " << hex << lx.getLine() << dec << endl;
		return 1;
	} catch (Exception x) {
        cout << dec;
		//cout << "Compilation terminated!" << endl;
		//return 1;
        cout << "Помилка: " << x.getMessage() << endl
             << "\tу файлі " << x.getSource() << endl;// << " at line " << cx.getLine() << endl;
		return 1;
	} catch (const char* str) {
		cout << str;
		return 1;
    } catch (char* str) {
		cout << str;
		return 1;
	} catch (string str) {
		cout << str;
		return 1;
	} catch (...) {
		cout << "Unknown fault!\n";
		return 1;
	}
	cout << endl << "Str_tab:" << endl;
    for (vector<int>::iterator  iter = str_stack_table.begin(); iter != str_stack_table.end(); ++iter)
		cout << *iter << endl;
	return 0;
}

void print(string &file) {
    ifstream ifs(file, ios::binary | ios::in);
	if(!ifs) {
		cout << "Very, very bad file! :'(";
		cout << endl;
		exit(1);
		//cin.get();
	}
	int i = 0, l = 0;
	while (!ifs.eof()) {
        char ch;
		ifs.get(ch);
		cout << setw(3) << dec << (int)ch << " ";
		i++;
		l++;
		if(i == 16) {
			i = 0;
			cout << "|" << dec << l << endl;
		}
	}
}

void readPath(string pathFile) {
    ifstream p(pathFile);
	if(!p) {
        //cout << "PATH file will be created" << endl;
        ofstream op(pathFile);
		op << "lang/bin/modules/" << endl << "lang/bin/std/";
		op.close();
		
        p.open(pathFile);
	}

	while (!p.eof())
	{
		string item;
		p >> item;
        cout << "PATH: " << item << endl;
		
		path.push_back(item);
	}

	p.close();
}
