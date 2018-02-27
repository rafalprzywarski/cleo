#include "clib.hpp"
#include "global.hpp"
#include "util.hpp"
#include "error.hpp"
#include "small_vector.hpp"
#include <sys/mman.h>
#include <dlfcn.h>

namespace cleo
{

constexpr std::uint8_t MAX_ARGS = 16;
using CFunction = Value(*)(const std::uint64_t *args, std::uint8_t num_args);

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
    int expand[] = {(put1(code, vals), 0)...};
    (void)expand;
}

Force create_c_fn(void *cfn, Value name, Value ret_type, Value param_types)
{
    check_type("fn name", name, *type::Symbol);
    check_type("parameter types", param_types, *type::SmallVector);
    const std::size_t max_size = 128;
    auto code = code_alloc(max_size);
    auto p = code;
    auto param_count = get_small_vector_size(param_types);
    if (param_count > MAX_ARGS)
        throw_illegal_argument("C functions can take up to " + std::to_string(MAX_ARGS) + ", got " + std::to_string(param_count));
    put(p,
        0x55,                                           // push   rbp
        0x48, 0x89, 0xe5                                // mov    rbp,rsp
    );
    if (param_count == 1)
        put(p, 0x48, 0x8b, 0x3f);                       // mov    rdi,QWORD PTR [rdi]
    if (param_count >= 2)
        put(p,
            0x48, 0x89, 0xf8,                           // mov    rax,rdi
            0x48, 0x8b, 0x38,                           // mov    rdi,QWORD PTR [rax]
            0x48, 0x8b, 0x70, 0x08                      // mov    rsi,QWORD PTR [rax+0x8]
        );
    if (param_count >= 3)
        put(p, 0x48, 0x8b, 0x50, 0x10);                 // mov    rdx,QWORD PTR [rax+0x10]
    if (param_count >= 4)
        put(p, 0x48, 0x8b, 0x48, 0x18);                 // mov    rcx,QWORD PTR [rax+0x18]
    if (param_count >= 5)
        put(p, 0x4c, 0x8b, 0x40, 0x20);                 // mov    r8,QWORD PTR [rax+0x20]
    if (param_count >= 6)
        put(p, 0x4c, 0x8b, 0x48, 0x28);                 // mov    r9,QWORD PTR [rax+0x28]
    if (param_count > 6)
    {
        if (param_count & 1)
            put(p, 0x48, 0x83, 0xec, 0x08);             // sub    rsp,0x8

        for (decltype(param_count) i = param_count - 1; i >= 6; --i)
            put(p, 0xff, 0x70, i * 8);                  // push   QWORD PTR [rax+i*8]
    }
    put(p,
        0xe8, rel_addr(cfn)                             // call   cfn
    );
    if (param_count > 6)
    {
        auto on_stack = ((param_count - 6) + 1) / 2 * 2;
        put(p,
            0x48, 0x83, 0xc4, on_stack * sizeof(Int64)  // add    rsp,on_stack*sizeof(Int64)
        );
    }
    put(p,
        0x48, 0x89, 0xc7,                               // mov    rdi,rax
        0x5d,                                           // pop    rbp
        0xe9, rel_addr(create_int64)                    // jmp    create_int64
    );

    enable_execution(code, max_size);
    Root caddr{create_int64(reinterpret_cast<Int64>(code))};
    return create_object3(*type::CFunction, *caddr, name, param_types);
}

Force call_c_function(const Value *args, std::uint8_t num_args)
{
    auto fn = args[0];
    auto addr = get_object_element(fn, 0);
    auto name = get_object_element(fn, 1);
    auto param_types = get_object_element(fn, 2);
    if ((num_args - 1) != get_small_vector_size(param_types))
        throw_arity_error(name, num_args - 1);
    std::uint64_t raw_args[MAX_ARGS];
    for (decltype(num_args) i = 1; i < num_args; ++i)
    {
        if (get_value_tag(args[i]) != tag::INT64)
            throw_arg_type_error(args[i], i - 1);
        raw_args[i - 1] = static_cast<std::uint64_t>(get_int64_value(args[i]));
    }
    return reinterpret_cast<CFunction>(get_int64_value(addr))(raw_args, num_args - 1);
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
