#ifndef RUNTIME_H
#define RUNTIME_H

#include "module.h"
#include <cstring>

class Runtime
{
    //byte * code;
    byte * stack;
    byte * memory;
    byte * mem_first_free;

    byte * stack_ptr;

    byte * global_var_mem;
    byte * globals_ptr;
    //byte * code_reg;

    uint current_stack_size = 0x40000; // 256Kb
    uint max_stack_size = 0x800000; //8Mb

    uint current_mem_size = 0x100000; // 1Mb
    uint mem_chunk_size = 0x100000; // 1Mb
    uint max_mem_size = 0x80000000; //2Gb

    uint global_size =  0x4000;
    uint global_max_size =  0x100000;

    string file;
    Module main_module;

    vector<Module*> imported;
public:
    friend class Module;

    enum RtExType {
        StackOverflow, GlobalMemOverflow, NotImplemented, MissingGlobalConstructor, CantExecute
    };

    Function main;

    Runtime();

    void Create(string file);
    void Load();
    void Start();
    void Start(uint entry, int argc=0, char*argv[]=nullptr);
    void Unload();

    uint FindFunctionId(const char* name) {
        ///TODO: arg types!!!
        for(uint i = 0; i < main_module.func_count; ++i)
            if(!strcmp(main_module.functions[i]->sign, name))
                return i;
        return -1;
    }

    Function *FindFunctionId(const char* name, Type args...) {
        return nullptr;
    }

    bool HasReturnCode();
    int GetReturnCode();

    inline static uint Sizeof(Type t) {
        switch (t) {
            case Type::BOOL:
            case Type::UI8:
                return 1;
            case Type::I16:
                return 2;
            case Type::I32:
            case Type::UI32:
            case Type::UTF8:
                return 4;
            case Type::DOUBLE:
            case Type::I64:
            case Type::UI64:
                return 8;
            default:
                return 0;
        }
    }

private:
    void stackalloc(uint need);
    void allocGlobalMem();
    void allocMem(uint need);
    void rtThrow(RtExType t);
    void execFunction(Function *f, byte *fargs, byte *local_table);

    inline void ldarg(uint idx, Function *f, byte *fargs);
    inline void ldfld(uint idx, Function *f);
    inline void ldloc(uint idx, Function *f, byte* locals);

    inline void starg(uint idx, Function *f, byte* fargs);
    inline void stfld(uint idx, Function *f);
    inline void stloc(uint idx, Function *f, byte *locals);
    void internalPrint(Function *f, byte *fargs);
    void internalRead(Function *f, Type ret);
    void conv_ui8();
    void conv_i16();
    void conv_i32();
    void conv_ui32();
    void conv_i64();
    void conv_ui64();
};

#endif // RUNTIME_H
