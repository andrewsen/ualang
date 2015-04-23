TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp \
    assembler.cpp \
    module.cpp \
    importedmodule.cpp

HEADERS += \
    assembler.h \
    module.h \
    common.h \
    opcodes.h \
    token.h \
    importedmodule.h

