#include "module.h"

Module::Module() : name("") {
}

Module::Module(string name)
{
    this->name = name;
}

string Module::GetName() {
    return name;
}
