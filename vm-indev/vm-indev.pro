#    uavm -- virtual machine. A part of bilexical programming language gramework
#    Copyright (C) 2013-2014 Andrew Senko.
#
#    This file is part of uavm.
#
#    uavm is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    uavm is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with uavm.  If not, see <http://www.gnu.org/licenses/>.
#
#    Written by Andrew Senko <andrewsen@yandex.ru>.

TEMPLATE = app

CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

#TEMPLATE = lib
QMAKE_CXXFLAGS += -std=c++11 -O3

SOURCES += \
    #vm.cpp \
    #limits.cpp \
    #commands_exec.cpp \
    entry.cpp \
    module.cpp \
    runtime.cpp \
    exec.cpp

HEADERS += \
    #commons.h \
    #commands_exec.h \
    asm_keywords.h \
    module.h \
    runtime.h\
    export.h \
    basic_structs.h
