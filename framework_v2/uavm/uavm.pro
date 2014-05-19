#-------------------------------------------------
#
# Project created by QtCreator 2014-05-06T14:25:34
#
#-------------------------------------------------

QT       -= core gui

TARGET = uavm
TEMPLATE = lib

DEFINES += UAVM_LIBRARY

SOURCES += vm.cpp

HEADERS += vm.h\
        uavm_global.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
