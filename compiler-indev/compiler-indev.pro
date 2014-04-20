#    uac -- bilexical compiler. A part of bilexical programming language gramework
#    Copyright (C) 2013-2014 Andrew Senko.
#
#    This file is part of uac.
#
#    uac is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    uac is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with uac.  If not, see <http://www.gnu.org/licenses/>.
#
#    Written by Andrew Senko <andrewsen@yandex.ru>.

TEMPLATE = app

CONFIG += console
CONFIG -= app_bundle

#CONFIG -= qt

QT       += core
QT       -= gui
#QMAKE_CXXFLAGS += -m32
QMAKE_CXXFLAGS += -std=c++11

SOURCES += \
    TheCore.cpp \
    script_asm.cpp \
    Module.cpp \
    getToken.cpp \
    code_writer.cpp \
    modulefile.cpp

HEADERS += \
    Module.h \
    asm_keywords.h \
    parser_common.h \
    modulefile.h

