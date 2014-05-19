TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++11

SOURCES += \
    assembler.cpp \
    main.cpp \
    module.cpp

HEADERS += \
    assembler.h \
    common.h \
    OpCodes.h \
    module.h \
    token.h

