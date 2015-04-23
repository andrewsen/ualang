#include "runtime.h"
#include <cstring>
//#include <unistd.h>

Runtime::Runtime() : imported(5, nullptr)
{
}

void Runtime::Create(string file)
{
    this->file = file;
    stack = new byte[current_stack_size];
    stack_ptr = stack;
    memory = new byte[current_mem_size];
    mem_first_free = memory;
    global_var_mem = new byte[global_size];
    globals_ptr = global_var_mem;

    //signal(SIGSEGV, sig_handler)
}

void Runtime::Load()
{
    main_module.rt = this;
    main_module.Load(file);

    //cout << "Modules loaded!" << endl;
}

void Runtime::Start(uint entryId, int argc, char *argv[])
{
    Function * entry = main_module.functions[entryId];
    union
    {
        byte b[4];
        uint addr;
    } conv;

    Function nativeMain;

    strcpy(nativeMain.sign, "__NativeMain__");

    nativeMain.module = entry->module;
    conv.addr = entryId;

    byte nativeBytecode[] = {
        (byte)OpCode::CALL, conv.b[0], conv.b[1], conv.b[2], conv.b[3], (byte)OpCode::RET
    };

    nativeMain.bytecode = (OpCode*)nativeBytecode;

    if(!entry->module->mflags.executable_bit)
        rtThrow(Runtime::CantExecute);
    execFunction(&nativeMain, nullptr, nullptr);
    //execFunction(entry, nullptr, nullptr, nullptr);
}

void Runtime::Unload()
{

}

bool Runtime::HasReturnCode()
{
    //FIXME:::
    return false;
}

int Runtime::GetReturnCode()
{

}

void Runtime::stackalloc(uint need) {
    if(((size_t)stack_ptr - (size_t)stack + need) < current_stack_size) return;

    uint old_size = current_stack_size;

    if(old_size >= max_stack_size)
    {
        //cin.get();
        rtThrow(StackOverflow);
    }

    size_t ptr = (size_t)stack_ptr - (size_t)stack;

    current_stack_size *= 2;
    byte * newstack = new byte[current_stack_size];
    memcpy(newstack, stack, old_size);
    delete [] stack;
    stack = newstack;
    stack_ptr = newstack + ptr;
}

void Runtime::allocGlobalMem()
{
    uint old_size = global_size;
    if(old_size >= global_max_size) rtThrow(GlobalMemOverflow);
    global_size *= 2;

    byte * new_globals = new byte [global_size];
    memcpy(new_globals, global_var_mem, old_size);
    delete [] global_var_mem;
    global_var_mem = new_globals;
}

void Runtime::allocMem(uint need)
{
    if(((size_t)this->mem_first_free - (size_t)this->memory) < this->max_mem_size) return;

    size_t used = (size_t)this->mem_first_free - (size_t)this->memory;

    uint old_size = this->current_mem_size;

    if(old_size >= max_mem_size) rtThrow(StackOverflow);

    current_mem_size += mem_chunk_size;
    byte * newmem = new byte[current_mem_size];
    memcpy(newmem, memory, old_size);
    delete [] memory;
    memory = newmem;
    mem_first_free = memory + used;
}

void Runtime::rtThrow(Runtime::RtExType t)
{
    throw t;
}

void Runtime::execFunction(Function *f, byte* fargs, byte* local_table)
{
    //cout << "Executing function " << f->sign << endl;
    OpCode* code = f->bytecode;
    while(true) {
        switch (*code) {
            case OpCode::ADD: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 + a2;
                    stack_ptr += 4; //sizeof(int) + 1 byte for type
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(__uint64_t*)(stack_ptr) = a1 + a2;
                    stack_ptr += 8; //sizeof(int) + 1 byte for type
                }
                //0x89ABCDEF: [EF|CD|AB|89];
            }
                break;
            case OpCode::ADDF: {
                double a2 = *(double*)(stack_ptr -= 8);
                double a1 = *(double*)(stack_ptr -= 9);

                *(double*)(stack_ptr) = a1 + a2;
                stack_ptr += 8;
            }
                break;
            case OpCode::AND: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 & a2;
                    stack_ptr += 4; //sizeof(int) + 1 byte for type
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(__uint64_t*)(stack_ptr) = a1 & a2;
                    stack_ptr += 8; //sizeof(int) + 1 byte for type
                }
            }
                break;
            case OpCode::CALL:
            {
            /*
             * [122|...|42|1|23.3|0x43|34]<-
             * call f(int, double, string, int)
             * [122|...|42]<-
             */
                uint faddr = *(uint*)(++code);
                code += 3;
                Function* next = f->module->functions[faddr];

                //uint args_size = 0;
                //uint args_tab[Function::ARGS_LENGTH];

                /*for(uint i = 0; i < next->argc; ++i) {
                    args_size += Runtime::Sizeof(next->args[i]) + 1;
                    args_tab[i] = args_size - 1;
                }*/
                byte* args = nullptr;
                if(next->args_size != 0)
                {
                    args = new byte[next->args_size];

                    stack_ptr -= next->args_size-1;
                    memcpy(args, stack_ptr, next->args_size);
                }

                if(!next->internal)
                {
                    byte* locs = new byte [next->local_mem_size];
                    execFunction(next, args, locs);
                    delete [] locs;
                }
                else
                {
                    if(!strcmp(next->sign, "print"))
                    {
                        internalPrint(next, args);
                    }
                    else if(!strcmp(next->sign, "reads"))
                    {
                        internalRead(f, Type::UTF8);
                    }
                    else if(!strcmp(next->sign, "readi"))
                    {
                        internalRead(f, Type::I32);
                    }
                    else if(!strcmp(next->sign, "readd"))
                    {
                        internalRead(f, Type::DOUBLE);
                    }
                }
                delete [] args;
            }
                break;
            case OpCode::CONV_F:
            {
                //stackalloc(4);
                switch((Type)*stack_ptr){
                    case Type::UI8:
                    case Type::UI32: {
                        uint arg = *(uint*)(stack_ptr -= 4);
                        *(double*)stack_ptr = (double)arg;
                        stack_ptr += 8;
                        break;
                    }
                    case Type::I16:
                    case Type::I32: {
                        int arg = *(int*)(stack_ptr -= 4);
                        *(double*)stack_ptr = (double)arg;
                        stack_ptr += 8;
                        break;
                    }
                    case Type::I64:{
                        __int64_t arg = *(__int64_t*)(stack_ptr -= 8);
                        *(double*)stack_ptr = (double)arg;
                        stack_ptr += 8;
                        break;
                    }
                    case Type::UI64:{
                        __uint64_t arg = *(__uint64_t*)(stack_ptr -= 8);
                        *(double*)stack_ptr = (double)arg;
                        stack_ptr += 8;
                        break;
                    }
                    default:
                        rtThrow(Runtime::NotImplemented);
                        break;
                }
                *stack_ptr = (byte)Type::DOUBLE;
            }
                break;
            /*:TODO:*/
            case OpCode::CONV_I16:
                conv_i16();
                break;
            case OpCode::CONV_I32:
                conv_i32();
                break;
            case OpCode::CONV_I64:
                conv_i64();
                break;
            case OpCode::CONV_UI8:
                conv_ui8();
                break;
            case OpCode::CONV_UI32:
                conv_ui32();
                break;
            case OpCode::CONV_UI64:
                conv_ui64();
                break;
            case OpCode::DEC:
            {
                switch((Type)*stack_ptr){
                    case Type::UI8:
                    case Type::UI32:
                    case Type::I16:
                    case Type::I32: {
                        --*(uint*)(stack_ptr - 4);
                        ++code;
                        continue;
                    }
                    case Type::I64:
                    case Type::UI64:{
                        --*(__uint64_t*)(stack_ptr - 8);
                        ++code;
                        continue;
                    }
                    case Type::DOUBLE:{
                        --*(double*)(stack_ptr - 8);
                        ++code;
                        continue;
                    }
                    default:
                        rtThrow(Runtime::NotImplemented);
                        break;
                }
            }
                break;
            case OpCode::DIV: {
                Type type = (Type)*stack_ptr;
                switch (type) {
                    case Type::I64:
                    case Type::UI64:
                    {
                        __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                        __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                        *(__uint64_t*)(stack_ptr) = a1 / a2;
                        stack_ptr += 8; //sizeof(int) + 1 byte for type
                        ++code;
                        continue;
                    }
                    default:
                    {
                        uint a2 = *(uint*)(stack_ptr -= 4);
                        uint a1 = *(uint*)(stack_ptr -= 5);

                        *(uint*)(stack_ptr) = uint(a1 / a2);
                        stack_ptr += 4; //sizeof(int) + 1 byte for type
                        ++code;
                        continue;
                    }
                }
                //0x89ABCDEF: [EF|CD|AB|89];
            }
                break;
            case OpCode::DIVF: {
                double a2 = *(double*)(stack_ptr -= 8);
                double a1 = *(double*)(stack_ptr -= 9);

                *(double*)(stack_ptr) = a1 / a2;
                stack_ptr += 8;
            }
                break;

            case OpCode::EQ: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64 && type != Type::UTF8) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 == a2;
                }
                else if(type == Type::DOUBLE) {
                    double a2 = *(double*)(stack_ptr -= 8);
                    double a1 = *(double*)(stack_ptr -= 9);

                    *(uint*)(stack_ptr) = a1 == a2;
                }
                else if(type == Type::UTF8){
                    char* str1 = (char*)*(size_t*)(stack_ptr -= 4);
                    char* str2 = (char*)*(size_t*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = (uint)(bool)strcmp(str1, str2);
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(uint*)(stack_ptr) = a1 == a2;
                }
                stack_ptr += 4;
                *stack_ptr = (byte)Type::BOOL;
            }
                break;
            /*case OpCode::FREELOC:
            {
                :TODO:
            }*/

            case OpCode::GT: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 != a2;
                }
                else if(type == Type::DOUBLE) {
                    double a2 = *(double*)(stack_ptr -= 8);
                    double a1 = *(double*)(stack_ptr -= 9);

                    *(uint*)(stack_ptr) = a1 != a2;
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(uint*)(stack_ptr) = a1 > a2;
                }
                stack_ptr += 4;
                *stack_ptr = (byte)Type::BOOL;
            }
                break;
            case OpCode::GTE: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 != a2;
                }
                else if(type == Type::DOUBLE) {
                    double a2 = *(double*)(stack_ptr -= 8);
                    double a1 = *(double*)(stack_ptr -= 9);

                    *(uint*)(stack_ptr) = a1 != a2;
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(uint*)(stack_ptr) = a1 >= a2;
                }
                stack_ptr += 4;
                *stack_ptr = (byte)Type::BOOL;
            }
                break;
            case OpCode::INC:
            {
                switch((Type)*stack_ptr){
                    case Type::UI8:
                    case Type::UI32:
                    case Type::I16:
                    case Type::I32: {
                        ++*(uint*)(stack_ptr - 4);
                        ++code;
                        continue;
                    }
                    case Type::I64:
                    case Type::UI64:{
                        ++*(__uint64_t*)(stack_ptr - 8);
                        ++code;
                        continue;
                    }
                    case Type::DOUBLE:{
                        ++*(double*)(stack_ptr - 8);
                        ++code;
                        continue;
                    }
                    default:
                        rtThrow(Runtime::NotImplemented);
                        break;
                }
            }
                break;

            case OpCode::INV: {
                *(uint*)(stack_ptr - 4) = (uint)~*(bool*)(stack_ptr - 4);
                switch((Type)*stack_ptr){
                    case Type::UI8:
                        *(byte*)(stack_ptr - 1) = ~*(byte*)(stack_ptr - 1);
                        break;
                    case Type::UI32:
                        *(uint*)(stack_ptr - 4) = ~*(uint*)(stack_ptr - 4);
                        break;
                    case Type::I16:
                        *(short*)(stack_ptr - 2) = ~*(short*)(stack_ptr - 2);
                        break;
                    case Type::I32:
                        *(int*)(stack_ptr - 4) = ~*(int*)(stack_ptr - 4);
                        break;
                    case Type::I64:
                        *(__int64_t*)(stack_ptr - 8) = ~*(__int64_t*)(stack_ptr - 8);
                        break;
                    case Type::UI64:
                        *(__uint64_t*)(stack_ptr - 8) = ~*(__uint64_t*)(stack_ptr - 8);
                        break;
                    default:
                        break;
                }
            }
                break;
            case OpCode::JMP:
            {
                code = f->bytecode + (*(uint*)++code);
            }
                continue;
            case OpCode::JF:
            case OpCode::JZ:
            {
                if(!*(uint*)(stack_ptr -= 4))
                    code = f->bytecode + (*(uint*)++code);
                else
                    code += 4;
                --stack_ptr;
            }
                continue;
            case OpCode::JT:
            case OpCode::JNZ:
            {
                if(*(uint*)(stack_ptr -= 4))
                    code = f->bytecode + (*(uint*)++code);
                else
                    code += 4;
                --stack_ptr;
            }
                continue;
            case OpCode::JNULL:
            case OpCode::JNNULL:
            {
                rtThrow(NotImplemented);
            }
                break;

            case OpCode::LDARG: {
                ldarg(*(uint*)(++code), f, fargs);
                code += 3; //FIXME: May fail
            }
                break;
            case OpCode::LDARG_0:
                ldarg(0, f, fargs);
                break;
            case OpCode::LDARG_1:
                ldarg(1, f, fargs);
                break;
            case OpCode::LDARG_2:
                ldarg(2, f, fargs);
                break;

            case OpCode::LDFLD: {
                ldfld(*(uint*)(++code), f);
                code += 3;
            }
                break;
            case OpCode::LDFLD_0:
                ldfld(0, f);
                break;
            case OpCode::LDFLD_1:
                ldfld(1, f);
                break;
            case OpCode::LDFLD_2:
                ldfld(2, f);
                break;

            case OpCode::LDLOC: {
                ldloc(*(uint*)(++code), f, local_table);
                code += 3;
            }
                break;
            case OpCode::LDLOC_0:
                ldloc(0, f, local_table);
                break;
            case OpCode::LDLOC_1:
                ldloc(1, f, local_table);
                break;
            case OpCode::LDLOC_2:
                ldloc(2, f, local_table);
                break;

            case OpCode::LD_0: {
                //stackalloc(5);
                *(int*)(++stack_ptr) = 0;
                *(stack_ptr += 4) = (byte)Type::I32;
            }
                break;
            case OpCode::LD_1: {
                //stackalloc(5);
                *(int*)(++stack_ptr) = 1;
                *(stack_ptr += 4) = (byte)Type::I32;
            }
                break;
            case OpCode::LD_2: {
                //stackalloc(5);
                *(int*)(++stack_ptr) = 2;
                *(stack_ptr += 4) = (byte)Type::I32;
            }
                break;

            case OpCode::LD_F:
            {
                //stackalloc(9);
                *(double*)(++stack_ptr) = *(double*)(++code);
                code += 3;
                *(stack_ptr += 8) = (byte)Type::DOUBLE;
            }
                break;
            case OpCode::LD_FALSE:
            {
                //stackalloc(5);
                *(uint*)(++stack_ptr) = 0;
                *(stack_ptr += 4) = (byte)Type::BOOL;
            }
                break;
            case OpCode::LD_TRUE:
            {
                //stackalloc(5);
                //stackalloc(5);
                *(uint*)(++stack_ptr) = 1;
                *(stack_ptr += 4) = (byte)Type::BOOL;
            }
                break;

            case OpCode::LD_UI8:
            {
                //stackalloc(5);
                *(byte*)(++stack_ptr) = *(byte*)(++code);
                code += 3;
                *(stack_ptr += 4) = (byte)Type::UI8;
                break;
            }

            case OpCode::LD_I16:
            {
                //stackalloc(5);
                *(short*)(++stack_ptr) = *(short*)(++code);
                code += 3;
                *(stack_ptr += 4) = (byte)Type::I16;
                break;
            }

            case OpCode::LD_I32:{
                //stackalloc(5);
                *(int*)(++stack_ptr) = *(int*)(++code);
                code += 3;
                *(stack_ptr += 4) = (byte)Type::I32;
            }
                break;
            case OpCode::LD_UI32:
            {
                //stackalloc(5);
                *(uint*)(++stack_ptr) = *(uint*)(++code);
                code += 3;
                *(stack_ptr += 4) = (byte)Type::UI32;
                break;
            }

            case OpCode::LD_I64:
            {
                //stackalloc(9);
                *(__int64_t*)(++stack_ptr) = *(__int64_t*)(++code);
                code += 7;
                *(stack_ptr += 8) = (byte)Type::I64;
                break;
            }

            case OpCode::LD_UI64:
            {
                //stackalloc(9);
                *(__uint64_t*)(++stack_ptr) = *(__uint64_t*)(++code);
                code += 7;
                *(stack_ptr += 8) = (byte)Type::UI64;
                break;
            }
            case OpCode::LD_NULL: {
                rtThrow(NotImplemented);
            }
                break;
            case OpCode::LD_STR: {
                char* ptr = f->module->strings + *(uint*)(++code);
                uint len = strlen(ptr)+1;

                allocMem(len);

                strcpy((char*)mem_first_free, ptr);

                //stackalloc(5);
                //++stack_ptr;
                *(size_t*)++stack_ptr = (size_t)(char*)mem_first_free;

                //cout << "STR ADDR: " << hex << (size_t)(char*)mem_first_free << dec << endl;

                mem_first_free += len;

                *(stack_ptr += 4) = (byte)Type::UTF8;
                code += 3;
                //f->module->strings
            }
                break;

            case OpCode::LT: {
                Type type = (Type)*stack_ptr;

                if(type != Type::UI64 && type != Type::I64) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 != a2;
                }
                else if(type == Type::DOUBLE) {
                    double a2 = *(double*)(stack_ptr -= 8);
                    double a1 = *(double*)(stack_ptr -= 9);

                    *(uint*)(stack_ptr) = a1 != a2;
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(uint*)(stack_ptr) = a1 < a2;
                }
                *(stack_ptr += 4) = (byte)Type::BOOL;
            }
                break;
            case OpCode::LTE: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 != a2;
                }
                else if(type == Type::DOUBLE) {
                    double a2 = *(double*)(stack_ptr -= 8);
                    double a1 = *(double*)(stack_ptr -= 9);

                    *(uint*)(stack_ptr) = a1 != a2;
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(uint*)(stack_ptr) = a1 <= a2;
                }
                *(stack_ptr += 4) = (byte)Type::BOOL;
            }
                break;
            case OpCode::MUL: {
                Type type = (Type)*stack_ptr;
                switch (type) {
                    case Type::I64:
                    case Type::UI64:
                    {
                        __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                        __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                        *(__uint64_t*)(stack_ptr) = a1 * a2;
                        stack_ptr += 8; //sizeof(int) + 1 byte for type
                        ++code;
                        continue;
                    }
                    default:
                    {
                        //uint a2 = *(uint*)(stack_ptr -= 4);
                        //uint a1 = *(uint*)(stack_ptr -= 5);

                        *(uint*)(stack_ptr) = *(stack_ptr -= 4) * *(stack_ptr -= 5);
                        stack_ptr += 4; //sizeof(int) + 1 byte for type
                        ++code;
                        continue;
                    }
                }
                /*if(type != Type::UI64 && type != Type::I64)
                {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 * a2;
                    stack_ptr += 4; //sizeof(int) + 1 byte for type
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(__uint64_t*)(stack_ptr) = a1 * a2;
                    stack_ptr += 8; //sizeof(int) + 1 byte for type
                }*/
                //0x89ABCDEF: [EF|CD|AB|89];
            }
                break;
            case OpCode::MULF: {
                double a2 = *(double*)(stack_ptr -= 8);
                double a1 = *(double*)(stack_ptr -= 9);

                *(double*)(stack_ptr) = a1 * a2;
                stack_ptr += 8;
            }
                break;
            case OpCode::NEG: {
                switch((Type)*stack_ptr){
                    case Type::UI8:
                    case Type::UI32:
                    case Type::I16:
                    case Type::I32: {
                        *(uint*)(stack_ptr - 4) = -*(uint*)(stack_ptr - 4);
                        break;
                    }
                    case Type::I64:
                    case Type::UI64:{
                        *(__uint64_t*)(stack_ptr - 8) = -*(__uint64_t*)(stack_ptr - 8);
                        break;
                    }
                    case Type::DOUBLE:{
                        *(double*)(stack_ptr - 8) = -*(double*)(stack_ptr - 8);
                        break;
                    }
                    default:
                        rtThrow(Runtime::NotImplemented);
                        break;
                }
            }
                break;
            case OpCode::NEQ: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64 && type != Type::UTF8) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 != a2;
                }
                else if(type == Type::DOUBLE) {
                    double a2 = *(double*)(stack_ptr -= 8);
                    double a1 = *(double*)(stack_ptr -= 9);

                    *(uint*)(stack_ptr) = a1 != a2;
                }
                else if(type == Type::UTF8) {
                    char* str1 = (char*)*(size_t*)(stack_ptr -= 4);
                    char* str2 = (char*)*(size_t*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = (bool)strcmp(str1, str2);
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(uint*)(stack_ptr) = a1 != a2;
                }
                *(stack_ptr += 4) = (byte)Type::BOOL;
            }
                break;
            /*case OpCode::NEWLOC: {
                Type t = *(Type*)(++code);
                switch (t) {
                    case Type::DOUBLE:
                    case Type::I64:
                    case Type::UI64:
                        allocMem(9);
                        break;
                    default:
                        allocMem(5);
                        break;
                }
            }
                break;*/
            case OpCode::NOP: {
                //Just as planned
            }
                break;
            case OpCode::NOT: {
                *(uint*)(stack_ptr - 4) = (uint)!*(bool*)(stack_ptr - 4);
            }
                break;
            case OpCode::OR: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 | a2;
                    stack_ptr += 4; //sizeof(int) + 1 byte for type
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(__uint64_t*)(stack_ptr) = a1 | a2;
                    stack_ptr += 8; //sizeof(int) + 1 byte for type
                }
            }
                break;
            case OpCode::POP: {
                Type type = (Type)*stack_ptr;
                if(type < Type::UI64)
                    stack_ptr -= 4;
                else
                    stack_ptr -= 8;
            }
                break;
            case OpCode::POS: {
                rtThrow(Runtime::NotImplemented);
            }
                break;
            case OpCode::REM: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 % a2;
                    stack_ptr += 4; //sizeof(int) + 1 byte for type
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(__uint64_t*)(stack_ptr) = a1 % a2;
                    stack_ptr += 8; //sizeof(int) + 1 byte for type
                }
            }
                break;
            case OpCode::REMF: {
                rtThrow(Runtime::NotImplemented);
            }
                break;
            case OpCode::RET: {
                return;
            }
            case OpCode::SHL: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 << a2;
                    stack_ptr += 4; //sizeof(int) + 1 byte for type
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(__uint64_t*)(stack_ptr) = a1 << a2;
                    stack_ptr += 8; //sizeof(int) + 1 byte for type
                }
            }
                break;
            case OpCode::SHR: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 >> a2;
                    stack_ptr += 4; //sizeof(int) + 1 byte for type
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(__uint64_t*)(stack_ptr) = a1 >> a2;
                    stack_ptr += 8; //sizeof(int) + 1 byte for type
                }
            }
                break;
            case OpCode::SIZEOF: {
                Type t = *(Type*)(++code);
                switch (t) {
                    case Type::DOUBLE:
                    case Type::I64:
                    case Type::UI64:
                        stack_ptr -= 8;
                        break;
                    default:
                        stack_ptr -= 4;
                        break;
                }
                *(uint*)stack_ptr = Runtime::Sizeof(t);
                *(stack_ptr += 4) = (byte)Type::I32;
            }
                break;
            case OpCode::STARG: {
                starg(*(uint*)(++code), f, fargs);
                code += 3; //FIXME: May fail
            }
                break;
            case OpCode::STARG_0: {
                starg(0, f, fargs);
            }
                break;
            case OpCode::STARG_1: {
                starg(1, f, fargs);
            }
                break;
            case OpCode::STARG_2: {
                starg(2, f, fargs);
            }
                break;
            case OpCode::STFLD: {
                stfld(*(uint*)(++code), f);
                code += 3; //FIXME: May fail
            }
                break;
            case OpCode::STFLD_0: {
                stfld(0, f);
            }
                break;
            case OpCode::STFLD_1: {
                stfld(1, f);
            }
                break;
            case OpCode::STFLD_2: {
                stfld(2, f);
            }
                break;
            case OpCode::STLOC: {
                stloc(*(uint*)(++code), f, local_table);
                code += 3; //FIXME: May fail
            }
                break;
            case OpCode::STLOC_0: {
                stloc(0, f, local_table);
            }
                break;
            case OpCode::STLOC_1: {
                stloc(1, f, local_table);
            }
                break;
            case OpCode::STLOC_2: {
                stloc(2, f, local_table);
            }
                break;
            case OpCode::SUB: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 - a2;
                    stack_ptr += 4;
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(__uint64_t*)(stack_ptr) = a1 - a2;
                    stack_ptr += 8;
                }
            }
                break;
            case OpCode::SUBF: {
                double a2 = *(double*)(stack_ptr -= 8);
                double a1 = *(double*)(stack_ptr -= 9);

                *(double*)(stack_ptr) = a1 - a2;
                stack_ptr += 8;
            }
                break;
            case OpCode::XOR: {
                Type type = (Type)*stack_ptr;
                if(type != Type::UI64 && type != Type::I64) {
                    uint a2 = *(uint*)(stack_ptr -= 4);
                    uint a1 = *(uint*)(stack_ptr -= 5);

                    *(uint*)(stack_ptr) = a1 ^ a2;
                    stack_ptr += 4; //sizeof(int) + 1 byte for type
                }
                else {
                    __uint64_t a2 = *(__uint64_t*)(stack_ptr -= 8);
                    __uint64_t a1 = *(__uint64_t*)(stack_ptr -= 9);

                    *(__uint64_t*)(stack_ptr) = a1 ^ a2;
                    stack_ptr += 8; //sizeof(int) + 1 byte for type
                }
            }

                break;
            default:
                break;
        }
        ++code;
    }
}

void Runtime::ldarg(uint idx, Function* f, byte* fargs)
{
    LocalVar* arg = &f->args[idx];
    uint sz = Sizeof(arg->type);
    memcpy(++stack_ptr, fargs + arg->addr, sz);
    *(stack_ptr += sz) = (byte)arg->type;
}


void Runtime::ldfld(uint idx, Function *f)
{
    GlobalVar* gv = &f->module->globals[idx];
    uint sz = Sizeof(gv->type);
    memcpy(++stack_ptr, gv->addr, sz);
    *(stack_ptr += sz) = (byte)gv->type;
}


void Runtime::ldloc(uint idx, Function *f, byte* locals)
{
    LocalVar* lv = &f->locals[idx];
    uint sz = Sizeof(lv->type);
    memcpy(++stack_ptr, locals + lv->addr, sz);
    *(stack_ptr += sz) = (byte)lv->type;
}

void Runtime::starg(uint idx, Function *f, byte* fargs)
{
    LocalVar* arg = &f->args[idx];
    uint sz = Sizeof(arg->type);
    stack_ptr -= sz;
    memcpy(fargs + arg->addr, stack_ptr, sz);
    --stack_ptr;
}

void Runtime::stfld(uint idx, Function *f)
{
    GlobalVar* gv = &f->module->globals[idx];
    uint sz = Sizeof(gv->type);
    stack_ptr -= sz;
    memcpy(gv->addr, stack_ptr, sz);
    --stack_ptr;
}

void Runtime::stloc(uint idx, Function *f, byte* locals)
{
    LocalVar* lv = &f->locals[idx];
    uint sz = Sizeof(lv->type);
    stack_ptr -= sz;
    memcpy(locals + lv->addr, stack_ptr, sz);
    --stack_ptr;
}

void Runtime::internalPrint(Function *f, byte* fargs)
{
    Type arg = f->args[f->argc-1].type;
    switch (arg) {
        case Type::UTF8:
            {
                char* str = (char*)*(size_t*)fargs;
                cout << str;
            }
            break;
        case Type::I32:
            cout << *(int*)fargs;
            break;
        case Type::DOUBLE:
            cout << *(double*)fargs;
            break;
        default:
            break;
    }
}
void Runtime::internalRead(Function *f, Type ret)
{
    //allocMem(len);

    //strcpy((char*)mem_first_free, ptr);

    //stackalloc(5);
    switch (ret) {
        case Type::UTF8:
            *(size_t*)++stack_ptr = (size_t)(char*)mem_first_free;

            cin >> (char*)mem_first_free;

            mem_first_free += strlen((char*)mem_first_free) + 1;

            stack_ptr += 4;
            *stack_ptr = (byte)Type::UTF8;
            break;
        case Type::I32:
            cin >> *(int*)++stack_ptr;

            stack_ptr += 4;
            *stack_ptr = (byte)Type::I32;
            break;
        case Type::DOUBLE:
            cin >> *(double*)++stack_ptr;

            stack_ptr += 8;
            *stack_ptr = (byte)Type::DOUBLE;
            break;
        default:
            break;
    }
}

void Runtime::conv_ui8()
{
    //stackalloc(4);
    switch((Type)*stack_ptr){
        case Type::UI32: {
            uint arg = *(uint*)(stack_ptr -= 4);
            *(uint*)stack_ptr = (byte)arg;
            stack_ptr += 4;
            break;
        }
        case Type::I16:
        case Type::I32: {
            int arg = *(int*)(stack_ptr -= 4);
            *(uint*)stack_ptr = (byte)arg;
            stack_ptr += 4;
            break;
        }
        case Type::I64:{
            __int64_t arg = *(__int64_t*)(stack_ptr -= 8);
            *(uint*)stack_ptr = (byte)arg;
            stack_ptr += 4;
            break;
        }
        case Type::UI64:{
            __uint64_t arg = *(__uint64_t*)(stack_ptr -= 8);
            *(uint*)stack_ptr = (byte)arg;
            stack_ptr += 4;
            break;
        }
        case Type::DOUBLE:{
            double arg = *(double*)(stack_ptr -= 8);
            *(uint*)stack_ptr = (byte)arg;
            stack_ptr += 4;
            break;
        }
        default:
            rtThrow(Runtime::NotImplemented);
            break;
    }
    *stack_ptr = (byte)Type::UI8;
}

void Runtime::conv_i16()
{
    //stackalloc(4);
    switch((Type)*stack_ptr){
        case Type::UI8:
        case Type::UI32: {
            uint arg = *(uint*)(stack_ptr -= 4);
            *(int*)stack_ptr = (short)arg;
            stack_ptr += 4;
            break;
        }
        case Type::I32: {
            int arg = *(int*)(stack_ptr -= 4);
            *(int*)stack_ptr = (short)arg;
            stack_ptr += 4;
            break;
        }
        case Type::I64:{
            __int64_t arg = *(__int64_t*)(stack_ptr -= 8);
            *(int*)stack_ptr = (short)arg;
            stack_ptr += 4;
            break;
        }
        case Type::UI64:{
            __uint64_t arg = *(__uint64_t*)(stack_ptr -= 8);
            *(int*)stack_ptr = (short)arg;
            stack_ptr += 4;
            break;
        }
        case Type::DOUBLE:{
            double arg = *(double*)(stack_ptr -= 8);
            *(int*)stack_ptr = (short)arg;
            stack_ptr += 4;
            break;
        }
        default:
            rtThrow(Runtime::NotImplemented);
            break;
    }
    *stack_ptr = (byte)Type::I16;
}

void Runtime::conv_i32()
{
    //stackalloc(4);
    switch((Type)*stack_ptr){
        case Type::UI8:
        case Type::UI32: {
            uint arg = *(uint*)(stack_ptr -= 4);
            *(int*)stack_ptr = (int)arg;
            stack_ptr += 4;
            break;
        }
        case Type::I16: {
            short arg = *(short*)(stack_ptr -= 4);
            *(int*)stack_ptr = (int)arg;
            stack_ptr += 4;
            break;
        }
        case Type::I64:{
            __int64_t arg = *(__int64_t*)(stack_ptr -= 8);
            *(int*)stack_ptr = (int)arg;
            stack_ptr += 4;
            break;
        }
        case Type::UI64:{
            __uint64_t arg = *(__uint64_t*)(stack_ptr -= 8);
            *(int*)stack_ptr = (int)arg;
            stack_ptr += 4;
            break;
        }
        case Type::DOUBLE:{
            double arg = *(double*)(stack_ptr -= 8);
            *(int*)stack_ptr = (int)arg;
            stack_ptr += 4;
            break;
        }
        default:
            rtThrow(Runtime::NotImplemented);
            break;
    }
    *stack_ptr = (byte)Type::I32;
}

void Runtime::conv_ui32()
{
    //stackalloc(4);
    switch((Type)*stack_ptr){
        case Type::UI8: {
            byte arg = *(uint*)(stack_ptr -= 4);
            *(uint*)stack_ptr = (uint)arg;
            stack_ptr += 4;
            break;
        }
        case Type::I16:
        case Type::I32: {
            short arg = *(short*)(stack_ptr -= 4);
            *(uint*)stack_ptr = (uint)arg;
            stack_ptr += 4;
            break;
        }
        case Type::I64:{
            __int64_t arg = *(__int64_t*)(stack_ptr -= 8);
            *(uint*)stack_ptr = (uint)arg;
            stack_ptr += 4;
            break;
        }
        case Type::UI64:{
            __uint64_t arg = *(__uint64_t*)(stack_ptr -= 8);
            *(uint*)stack_ptr = (uint)arg;
            stack_ptr += 4;
            break;
        }
        case Type::DOUBLE:{
            double arg = *(double*)(stack_ptr -= 8);
            *(uint*)stack_ptr = (uint)arg;
            stack_ptr += 4;
            break;
        }
        default:
            rtThrow(Runtime::NotImplemented);
            break;
    }
    *stack_ptr = (byte)Type::UI32;
}

void Runtime::conv_i64()
{
    //stackalloc(4);
    switch((Type)*stack_ptr) {
        case Type::UI8:
        case Type::UI32: {
            uint arg = *(uint*)(stack_ptr -= 4);
            *(__int64_t*)stack_ptr = (__int64_t)arg;
            stack_ptr += 8;
            break;
        }
        case Type::I16:
        case Type::I32: {
            short arg = *(short*)(stack_ptr -= 4);
            *(__int64_t*)stack_ptr = (__int64_t)arg;
            stack_ptr += 8;
            break;
        }
        case Type::UI64:{
            __uint64_t arg = *(__uint64_t*)(stack_ptr -= 8);
            *(__int64_t*)stack_ptr = (__int64_t)arg;
            stack_ptr += 8;
            break;
        }
        case Type::DOUBLE:{
            double arg = *(double*)(stack_ptr -= 8);
            *(__int64_t*)stack_ptr = (__int64_t)arg;
            stack_ptr += 8;
            break;
        }
        default:
            rtThrow(Runtime::NotImplemented);
            break;
    }
    *stack_ptr = (byte)Type::I64;
}

void Runtime::conv_ui64()
{
    //stackalloc(4);
    switch((Type)*stack_ptr) {
        case Type::UI8:
        case Type::UI32: {
            uint arg = *(uint*)(stack_ptr -= 4);
            *(__uint64_t*)stack_ptr = (__uint64_t)arg;
            stack_ptr += 8;
            break;
        }
        case Type::I16:
        case Type::I32: {
            short arg = *(short*)(stack_ptr -= 4);
            *(__uint64_t*)stack_ptr = (__uint64_t)arg;
            stack_ptr += 8;
            break;
        }
        case Type::I64:{
            __int64_t arg = *(__int64_t*)(stack_ptr -= 8);
            *(__uint64_t*)stack_ptr = (__uint64_t)arg;
            stack_ptr += 8;
            break;
        }
        case Type::DOUBLE:{
            double arg = *(double*)(stack_ptr -= 8);
            *(__uint64_t*)stack_ptr = (__uint64_t)arg;
            stack_ptr += 8;
            break;
        }
        default:
            rtThrow(Runtime::NotImplemented);
            break;
    }
    *stack_ptr = (byte)Type::UI64;
}
