/*
          __________    _______     ___
         /  _____   /  /  __   /   /  /
        /  /    /  /  /  /__/ /   /  /
       /  /    /  /  /  _____/   /  /
      /  /    /  /  /  /        /  /
     /  /    /  /  /  /        /  /
    /__/    /__/  /__/        /____

   _________    _______    __     ___    ______     _____
  |   ___   |  |   __  |  |  |   /   |  |   __ \
  |  |   |  |  |  |__| |  |  |  /    |  |  |__| |   |  |___
  |  |   |  |  |   ____|  |  | /  /  |  |   ___ \   |   ___
  |  |   |  |  |  |       |  |/  /|  |  |  |   | |  |   ___|
  |  |   |  |  |  |       |  |  / |  |  |  |___| |  |  |___
  |__|   |__|  |__|       |____/  |__|  |________/  |______|





 */

#include <cstring>
#include "runtime.h"

Module::Module()
{
    headers.reserve(3);
}

void Module::Load(string file)
{
    this->file = file;

    ifstream ifs(file, ios::in | ios::binary);

    uint mg;

    ifs.read((char*)&mg, 4);
    //if(mg != MOD_MAGIC)
    //    throw InvalidMagicException(mg);

    uint headers_end;
    ifs.read((char*)&headers_end, 4);
    ifs.read((char*)&mflags, sizeof(ModuleFlags));

    readSegmentHeaders(ifs, headers_end);
    readSegments(ifs);

    if(!mflags.no_globals_bit)
    {
        if(__global_constructor__ == nullptr) rt->rtThrow(Runtime::MissingGlobalConstructor);
        rt->execFunction(__global_constructor__, nullptr, nullptr);
    }

    ifs.close();
}

void Module::readSegmentHeaders(ifstream &ifs, uint hend) {
    while(ifs.tellg() != hend) {
        //cout << ifs.tellg() << endl;
        Header h;

        char segName[80];
        char* ptr = &segName[0];
        while(true) {
            *ptr = ifs.get();
            if(*ptr == '\0') break;
            ptr++;
        }
        h.type = (Header::Type)(byte)ifs.get();
        ifs.read((char*)&h.begin, 4);
        ifs.read((char*)&h.end, 4);

        h.name = segName;
        this->headers.push_back(h);
    }
}

void Module::readSegments(ifstream &ifs) {
    for(uint i = 0; i < headers.size(); ++i) {
        Header &h = headers[i];
        //cout << "Found header: " << h.name << endl;
        switch (h.type) {
            case Header::Functions:
                {
                    ifs.seekg(h.begin);
                    //uint fcount;
                    ifs.read((char*)&this->func_count, 4);

                    this->functions = new Function*[this->func_count];

                    uint fc = 0;

                    while (ifs.tellg() != h.end) {
                        this->functions[fc] = new Function;
                        Function* fun = this->functions[fc];

                        fun->ret = (Type)ifs.get();
                        //cout << "ifs.tellg() = " << hex << ifs.tellg() << endl;
                        ifs.get(fun->sign, SIGN_LENGTH, '\0').ignore();
                        //cout << "ifs.tellg() = " << hex << ifs.tellg() << dec << endl;
                        char ch = ifs.get();
                        uint arg_addr = 0;
                        while(ch != '\0') {
                            fun->args[fun->argc].type = (Type)ch;
                            fun->args[fun->argc].addr = arg_addr;
                            arg_addr += Runtime::Sizeof(fun->args[fun->argc].type) + 1;
                            ++fun->argc;
                            ch = ifs.get();
                        }
                        fun->args_size = arg_addr;

                        fun->isPrivate = (bool)ifs.get();
                        bool imported = (bool)ifs.get();
                        if(imported) {
                            char mod[128];
                            ifs.get(mod, 128, '\0').ignore();
                            if(!strcmp(mod, "::vm.internal")) {
                                fun->internal = true;
                            }
                            else
                            {
                                for(Module* m : rt->imported)
                                    if(m != nullptr && !strcmp(mod, m->file.c_str()))
                                    {
                                        for(uint t = 0; t < m->func_count; t++)
                                        {
                                            Function* f = m->functions[t];
                                            if(!strcmp(fun->sign, f->sign) && fun->argc == f->argc)
                                            {
                                                for(uint i = 0; i < f->argc; ++i)
                                                {
                                                    if(fun->args[i].type != f->args[i].type)
                                                        continue;
                                                    fun = f;

                                                }
                                            }
                                        }
                                        goto loop_out;
                                    }
                                Module* m = new Module;
                                m->Load(mod);
                                rt->imported.push_back(m);
                                for(uint t = 0; t < m->func_count; t++)
                                {
                                    Function* f = m->functions[t];
                                    if(!strcmp(fun->sign, f->sign) && fun->argc == f->argc)
                                    {
                                        for(uint i = 0; i < f->argc; ++i)
                                        {
                                            if(fun->args[i].type != f->args[i].type)
                                                continue;
                                            fun = f;
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            //uint size;
                            ifs.read((char*)&fun->locals_size, 4);
                            if(fun->locals_size > 0) {
                                fun->locals = new LocalVar[fun->locals_size];

                                //uint local_mem_size;
                                ifs.read((char*)&fun->local_mem_size, 4);
                                //fun->locals_mem = new byte[local_mem_size];

                                uint cur_addr = 0;
                                for(uint i = 0; i < fun->locals_size; ++i) {
                                    fun->locals[i].addr = cur_addr;
                                    fun->locals[i].type = (Type)ifs.get();
                                    cur_addr += Runtime::Sizeof(fun->locals[i].type);
                                }
                                //ifs.read((char*)fun->locals, fun->locals_size);
                            }
                            ifs.read((char*)&fun->bc_size, 4);
                            fun->bytecode = new OpCode[fun->bc_size];
                            ifs.read((char*)fun->bytecode, fun->bc_size);
                        }
                        if(!strcmp(fun->sign, "__global_constructor__") && fun->argc == 0)
                            this->__global_constructor__ = fun;
                        fun->module = this;
loop_out:

                        ++fc;
                    }
                }
                break;
            case Header::Strings:
                {
                    //cout << "Strings size: " << h.end - h.begin << endl;
                    uint size = h.end - h.begin;
                    this->strings = new char[size];
                    ifs.seekg(h.begin);
                    ifs.read(this->strings, size);
                }
                break;
            case Header::Globals:
                {
                    ifs.seekg(h.begin);
                    //uint gcount;
                    ifs.read((char*)&this->globals_count, 4);
                    if(this->globals_count == 0) {
                        this->globals = nullptr;
                        continue;
                    }
                    this->globals = new GlobalVar[this->globals_count];
                    GlobalVar* gptr = globals;
                    while(ifs.tellg() != h.end) {
                        gptr->type = (Type)ifs.get();
                        gptr->isPrivate = (bool)ifs.get();
                        ifs.get(gptr->name, SIGN_LENGTH, '\0').ignore();

                        gptr->addr = rt->globals_ptr;
                        uint newsize = Runtime::Sizeof(gptr->type) + 1;
                        uint sz = size_t(rt->globals_ptr - rt->global_var_mem);
                        if(sz + newsize > rt->global_size)
                            rt->allocGlobalMem();
                        rt->globals_ptr += newsize;
                        ++gptr;
                    }
                }
                break;
            default:
                break;
        }
    }
}
