TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++11 -O3

SOURCES += main.cpp \
    runtime.cpp \
    module.cpp

HEADERS += \
    runtime.h \
    module.h \
    common.h \
    opcodes.h

