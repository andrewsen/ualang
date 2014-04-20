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

#ifndef EXPORT_H
#define EXPORT_H
#include <string>
#include <vector>
#include "basic_structs.h"

namespace VM {

using namespace std;

typedef int (*trace) (TraceData td);
extern "C" {

void PreInit(int argc, const char* argv[]);
void SetStdIn(istream *i);
void SetStdOut(ostream *o);
void SetStdErr(ostream *o);
void SetStdLog(ostream *o);
void SetTraceEnabled(bool e);
void SetTraceHandler(trace th);
void InitVM();
void* GetRuntime();
int Start();
void DestroyVM();
}
}
#endif // EXPORT_H
