#include "clib.hpp"
#include "global.hpp"
#include "util.hpp"
#include "error.hpp"
#include "small_vector.hpp"
#include <sys/mman.h>
#include <dlfcn.h>

namespace cleo
{

using CFunction = Value(*)(const Value *args, std::uint8_t num_args, Value& err);

char *code_alloc(std::size_t size)
{
    return static_cast<char *>(mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));
}

void enable_execution(char *code, std::size_t size)
{
    mprotect(code, size, PROT_READ | PROT_EXEC);
}

struct rel_addr
{
    void *abs_addr;
    template <typename T>
    rel_addr(T *p) : abs_addr(reinterpret_cast<void *>(p)) {}
};

struct abs_addr
{
    std::uintptr_t addr;
    template <typename T>
    abs_addr(T p) : addr(reinterpret_cast<std::uintptr_t>(p)) {}
};

void put1(char *& code, rel_addr val)
{
    auto rel_addr = reinterpret_cast<std::uintptr_t>(val.abs_addr) - (reinterpret_cast<std::uintptr_t>(code) + 4);
    assert((rel_addr >> 32) == 0xffffffff || (rel_addr >> 32) == 0);
    std::memcpy(code, &rel_addr, 4);
    code += 4;
}

void put1(char *& code, abs_addr val)
{
    std::memcpy(code, &val.addr, 8);
    code += 8;
}

void put1(char *& code, unsigned char val)
{
    *code++ = val;
}

template <typename T0>
void put(char *& code, T0 val0)
{
    put1(code, val0);
}

template <typename T0, typename... Ts>
void put(char *& code, T0 val0, Ts... vals)
{
    put1(code, val0);
    put(code, vals...);
}

Force create_c_fn(void *cfn, Value name, Value ret_type, Value param_types)
{
    check_type("fn name", name, *type::Symbol);
    check_type("parameter types", param_types, *type::SmallVector);
    const std::size_t max_size = 256;
    auto code = code_alloc(max_size);
    auto p = code;
    if (get_small_vector_size(param_types) == 0)
    {
        put(p,
            0x55,                                       // push   rbp
            0x48, 0x89, 0xe5,                           // mov    rbp,rsp
            0x53,                                       // push   rbx
            0x50,                                       // push   rax
            0x48, 0x89, 0xd3,                           // mov    rbx,rdx
            0x40, 0x84, 0xf6,                           // test   sil,sil
            0x74, 0x1f,                                 // je     +31
            0x48, 0xbf, abs_addr(name.bits()),          // movabs rdi,name
            0x40, 0x0f, 0xb6, 0xf6,                     // movzx  esi,sil
            0xe8, rel_addr(create_arity_error),         // call   create_arity_error
            0x48, 0x89, 0x03,                           // mov    QWORD PTR [rbx],rax
            0x31, 0xc0,                                 // xor    eax,eax
            0x48, 0x83, 0xc4, 0x08,                     // add    rsp,0x8
            0x5b,                                       // pop    rbx
            0x5d,                                       // pop    rbp
            0xc3,                                       // ret
            0xe8, rel_addr(cfn),                        // call   cfn
            0x48, 0x89, 0xc7,                           // mov    rdi,rax
            0x48, 0x83, 0xc4, 0x08,                     // add    rsp,0x8
            0x5b,                                       // pop    rbx
            0x5d,                                       // pop    rbp
            0xe9, rel_addr(create_int64)                // jmp    create_int64
        );
    }
    else
    {
        put(p,
            0x55,                                       // push   rbp
            0x48, 0x89, 0xe5,                           // mov    rbp,rsp
            0x53,                                       // push   rbx
            0x50,                                       // push   rax
            0x48, 0x89, 0xd3,                           // mov    rbx,rdx
            0x40, 0x80, 0xfe, 0x01,                     // cmp    sil,0x1
            0x74, 0x1f,                                 // je     +31
            0x48, 0xbf, abs_addr(name.bits()),          // movabs rdi,name
            0x40, 0x0f, 0xb6, 0xf6,                     // movzx  esi,sil
            0xe8, rel_addr(create_arity_error),         // call   create_arity_error
            0x48, 0x89, 0x03,                           // mov    QWORD PTR [rbx],rax
            0x31, 0xc0,                                 // xor    eax,eax
            0x48, 0x83, 0xc4, 0x08,                     // add    rsp,0x8
            0x5b,                                       // pop    rbx
            0x5d,                                       // pop    rbp
            0xc3,                                       // ret
            0x48, 0x8b, 0x3f,                           // mov    rdi,QWORD PTR [rdi]
            0xe8, rel_addr(get_int64_value),            // call   get_int64_value
            0x48, 0x89, 0xc7,                           // mov    rdi,rax
            0xe8, rel_addr(cfn),                        // call   cfn
            0x48, 0x89, 0xc7,                           // mov    rdi,rax
            0x48, 0x83, 0xc4, 0x08,                     // add    rsp,0x8
            0x5b,                                       // pop    rbx
            0x5d,                                       // pop    rbp
            0xe9, rel_addr(create_int64)                // jmp    create_int64
        );
    }

    enable_execution(code, max_size);
    Root caddr{create_int64(reinterpret_cast<Int64>(code))};
    return create_object1(*type::CFunction, *caddr);
}

Force call_c_function(const Value *args, std::uint8_t num_args)
{
    auto fn = args[0];
    auto addr = get_object_element(fn, 0);
    Value err;
    Value result = reinterpret_cast<CFunction>(get_int64_value(addr))(args + 1, num_args - 1, err);
    if (err)
    {
        Root r{err};
        throw_exception(err);
    }
    return result;
}

Force import_c_fn(Value libname, Value fnname, Value calling_conv, Value ret_type, Value param_types)
{
    check_type("lib-name", libname, *type::String);
    check_type("fn-name", fnname, *type::String);
    std::string slibname{get_string_ptr(libname), get_string_len(libname)};
    std::string sfnname{get_string_ptr(fnname), get_string_len(fnname)};
    auto lib = dlopen(slibname.c_str(), RTLD_NOW);
    if (!lib)
        return nil;
    auto sym = dlsym(lib, sfnname.c_str());
    if (!sym)
        return nil;
    return create_c_fn(sym, create_symbol("so:" + slibname, sfnname), ret_type, param_types);
}

}
