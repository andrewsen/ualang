/*
    uavm -- virtual machine. A part of bilexical programming language gramework
    Copyright (C) 2013-2014 Andrew Senko.

    This file is part of uavm.

    Uavm is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Uavm is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with uavm.  If not, see <http://www.gnu.org/licenses/>.
*/

/*  Written by Andrew Senko <andrewsen@yandex.ru>. */


#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
//#include "commons.h"
#include "runtime.h"
#ifdef WIN32
	#include <Windows.h>
#endif

#include "export.h"
bool tracer(Runtime *rt, Module* m);

int main (int argc, const char* argv[]) {
    cout << "debug\n";
#ifdef WIN32
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
#endif
//#ifndef _DEBUG_

    //auto v = split("Hello|, |Wor|ld|!", '|', false);
    //for(auto s : v) cout << s;
    //cout << endl;
    //cout << argv[0] << endl;

    //String work_dir = getenv("PWD");
    //work_dir += "/";
    //setenv("SVM_PATH", "/usr/share/svm/modules:" + work_dir + "modules/:", 1);
    //cout << getenv("SVM_PATH") << endl;
    //string arg = "";
    //getline(cin, arg);
    //cin >> arg;
    //arg = arg.erase(0,1);

    //const char * args[2];
    //args[0] = argv[0];
    //args[1] = arg.c_str();
    try {

#ifndef _DEBUG_
        if(argc == 1) return 0;
        Runtime* rt;
        if(argc > 2 && !strcmp(argv[1], "-t")) {
            const char * args[2];
            args[0] = argv[0];
            args[1] = argv[2];
            rt = Runtime::Create(2, args);
            rt->EnableTrace();
            rt->SetTraceHandler(tracer);
        }
        else if(argc > 2 && (!strcmp(argv[1], "-l") || !strcmp(argv[1], "--enable-logging"))) {
            const char * args[2];
            args[0] = argv[0];
            args[1] = argv[2];
            Runtime::Logging(true);
            rt = Runtime::Create(2, args);
        }
        else if(argc >= 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
            cout << "Senko's Virtual Machine. Віртуальна машина білексичної мови програмування\n"
                << argv[0] << " [ПАРАМЕТР] <file.sem>" << endl << endl

                << "  -l,  --enable-logging - включити вивід діагностичної інформації\n"

                << "  -h,  --help           - показати довідку\n"
                << "  -v,  --version        - показати версію\n";
            return 0;
        }
        else if(argc >= 2 && (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))) {
            cout << "Senko's Virtual Machine, версія 0.1-beta" << endl;
            return 0;
        }
        else
            rt = Runtime::Create(argc, argv);
#else
        const char * args[2];
        args[0] = argv[0];
        args[1] = "/home/senko/Рабочий стол/hello.sem";
        Runtime *rt = Runtime::Create(2, args);
#endif
        //cout << getenv("11VM_/home/senko/qt/Framework/build-vm-indev-Desktop-Debug/prog1.sky.semPATH") << endl;
        //exit(0);/home/senko/qt/Framework/build-vm-indev-Desktop-Debug/prog1.sky.sem
        rt->Start();
        rt->Shutdown();
        Runtime::Destroy(rt);
    }
    catch (RuntimeException rte) {
        cerr << "\033[0m";
        cerr << "Помилка етапу виконання:" << endl;
        cerr << rte.what() << endl;
        //cerr << rte.what() << endl;
        //cerr << rte.where() << endl;
        //cerr << rte.data() << endl;
        //rt.Handle(rte);
    }
    catch (string e) {
        cerr << "\033[0m";
        cerr << "Помилка:\n" << e << endl;
        //cerr << rte.what() << endl;
        //cerr << rte.where() << endl;
        //cerr << rte.data() << endl;
        //rt.Handle(rte);
    }
    catch (char const* e) {
        cerr << "\033[0m";
        cerr << "Помилка:\n" << e << endl;
        //cerr << rte.what() << endl;
        //cerr << rte.where() << endl;
        //cerr << rte.data() << endl;
        //rt.Handle(rte);
    }
    catch (std::bad_alloc ba) {
        cerr << "\033[0m";
        cerr << "Помилка виділення пам'яті:\n" << ba.what() << endl;
        //cerr << rte.what() << endl;
        //cerr << rte.where() << endl;
        //cerr << rte.data() << endl;
        //rt.Handle(rte);
    }

	if(argc == 1) {
        //cout << "usage: " << argv[0] << " <file.out>";
        //cin.get();
		return 1;
	}
	try {
        //init(argv[1]);
		//cout << "LOG: initialized, ready for execute!\n";
        //execute();
    }
	catch (const char * str) {
		cerr << str;
		//cin.get();
		return 0;
	}
    cerr << "\033[0m";
	//int i = 0;
	return 0;
}

bool tracer(Runtime *rt, Module* m) {
    char actions [255];
    cout << endl << "(trace) ";
    cin >> actions;
    auto strs = split(string(actions), ' ', true);
    if(strs[0] == "pop") {

    }
    else if(strs[0] == "push") {

    }
    else if(strs[0] == "ps") {

    }
    else if(strs[0] == "vminfo") {

    }
    else if(strs[0] == "pstr") {

    }
    else if(strs[0] == "pregs") {

    }
    else if(strs[0] == "reg") {

    }
    else if(strs[0] == "stop") {
        return false;
    }
    return true;
}

int ac;
const char **av;
istream *in;
ostream *out;
ostream *err;
ostream *log;
Runtime *rt;

void VM::PreInit(int argc, const char *argv[]) {
    ac = argc;
    av = argv;
    in = &cin;
    out = &cout;
    cout << "Pre Init" << endl;
    err = &cerr;
    log = &clog;
}

void VM::SetStdIn(istream *i) {
    in = i;
}

void VM::SetStdOut(ostream *o) {
    out = o;
}

void VM::SetStdErr(ostream *o) {
    err = o;
}

void VM::SetStdLog(ostream *o) {
    log = o;
}

void VM::InitVM() {
    cout << "Creating VM" << endl;
    rt = Runtime::Create(ac, av);
}

void VM::SetTraceEnabled(bool e) {
    flags.TF = 1;
}

int VM::Start() {
    rt->Start();
    return vra.word;
}

void VM::SetTraceHandler(trace th) {
    //rt->SetTraceHandler(th);
}

void VM::DestroyVM() {
    delete rt;
}

// Output a string.
/*ostream &operator<<(ostream &stream, String &o)
{
  stream << o.p;
  return stream;
}

// Input a string.
istream &operator>>(istream &stream, String &o)
{
    //stream >> o.p;
    char t[255]; // arbitrary size - change if necessary
  int len;

  stream.getline(t, 255);
  len = strlen(t) + 1;

  //if(len > o.Length()) {
  //}
  o.p = t;
  return stream;
}

String operator+(const char *s, const String &o)
{
  //int len;
  String temp = s;
  temp += o;

  //delete [] temp.p;

  //len = strlen(s) + strlen(o.p) + 1;
  //temp.size = len;
  //try {
  //  temp.p = new char[len];
  //} catch (bad_alloc xa) {
  //  cout << "Allocation error\n";
  //  exit(1);
  //}
  //strcpy(temp.p, s);

  //strcat(temp.p, o.p);

  return temp;
}*/

