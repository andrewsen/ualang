#ifndef COMMON_H
#define COMMON_H

#include <string>
//#include "opcodes.h"

typedef unsigned char byte;

class Exception {
protected:
    std::string what;

public:
    virtual std::string What() {
        return what;
    }
};

#endif // COMMON_H
