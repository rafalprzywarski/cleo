#include "clib.hpp"
#include "global.hpp"
#include "util.hpp"
#include "error.hpp"
#include "array.hpp"
#include <cstring>
#include <sys/mman.h>
#include <dlfcn.h>
#include <algorithm>

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

#ifndef __ARM_ARCH

struct rel_addr
{
    void *abs_addr;
    template <typename T>
    rel_addr(T *p) : abs_addr(reinterpret_cast<void *>(p)) {}
};

void put1(char *& code, rel_addr val)
{
    auto rel_addr = reinterpret_cast<std::uintptr_t>(val.abs_addr) - (reinterpret_cast<std::uintptr_t>(code) + 4);
    assert((rel_addr >> 32) == 0xffffffff || (rel_addr >> 32) == 0);
    std::memcpy(code, &rel_addr, 4);
    code += 4;
}

#endif // !__ARM_ARCH

struct abs_addr
{
    std::uintptr_t addr;
    template <typename T>
    abs_addr(T p) : addr(reinterpret_cast<std::uintptr_t>(p)) {}
};

void put1(char *& code, abs_addr val)
{
    std::memcpy(code, &val.addr, sizeof(val.addr));
    code += sizeof(val.addr);
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

#ifdef __ARM_ARCH

std::vector<std::uint8_t> param_offsets(Value param_types)
{
    auto param_count = get_array_size(param_types);
    std::vector<std::uint8_t> offsets;
    offsets.reserve(param_count);
    std::uint8_t offset = 0;
    for (decltype(param_count) i = 0; i < param_count; ++i)
    {
        if (get_array_elem(param_types, i).is(clib::int64))
        {
            if (offset & 1)
                ++offset;
            offsets.push_back(offset);
            offset += 2;
        }
        else // string
        {
            offsets.push_back(offset);
            ++offset;
        }
    }
    return offsets;
}

std::vector<std::uint8_t> param_sizes(Value param_types)
{
    auto param_count = get_array_size(param_types);
    std::vector<std::uint8_t> sizes;
    sizes.reserve(param_count);
    for (decltype(param_count) i = 0; i < param_count; ++i)
        sizes.push_back(get_array_elem(param_types, i).is(clib::int64) ? 2 : 1);
    return sizes;
}

void generate_code(char *p, void *cfn, Value param_types)
{
    auto param_count = get_array_size(param_types);
    auto code = p;
    put(p, 0x20, 0x48, 0x2d, 0xe9);                         // push	{r5, fp, lr}
    put(p, 0x0d, 0xb0, 0xa0, 0xe1);                         // mov	fp, sp

    auto offsets = param_offsets(param_types);
    auto sizes = param_sizes(param_types);
    auto first_on_stack = std::find(begin(offsets), end(offsets), 4) - begin(offsets);
    if (first_on_stack < param_count)
    {
        auto total = ((offsets.back() - 4) + sizes.back()) * 4;
        put(p, total, 0xd0, 0x4d, 0xe2);                    // sub	sp, sp, total
    }

    for (decltype(param_count) i = first_on_stack; i < param_count; ++i)
    {
        auto stack_offset = (offsets[i] - 4) * 4;
        if (sizes[i] == 2)
        {
            put(p,
                0xd0 + ((i & 1) << 3), 0x20 + (i >> 1), 0xc0, 0xe1,                     // ldrd	r2, [r0,pi]
                0xf0 + (stack_offset & 0xf), 0x20 + (stack_offset >> 4), 0xcd, 0xe1     // strd r2, [sp, pi]
            );
        }
        else
        {
            assert(sizes[i] == 1);
            put(p,
                i * 8, 0x20, 0x90, 0xe5,                                                // ldr	r2, [r0,pi]
                stack_offset, 0x20, 0x8d, 0xe5                                          // str  r2, [sp, pi]
            );
        }
    }

    for (decltype(param_count) k = first_on_stack, i = k - 1; k > 0; --k, --i)
        if (sizes[i] == 1)
            put(p, i * 8, offsets[i] << 4, 0x90, 0xe5);                                 // ldr	rx, [r0,pi]
        else
            put(p, 0xd0 + ((i & 1) << 3), (offsets[i] << 4) + (i >> 1), 0xc0, 0xe1);    // ldrd	rx, [r0,pi]

    put(p,
        first_on_stack < param_count ? 0x10 : 0x0c, 0x50, 0x9f, 0xe5,   // ldr	r5, [pc, #16 or #12]	; cfn
        0x35, 0xff, 0x2f, 0xe1                                          // blx	r5
    );

    if (first_on_stack < param_count)
        put(p, 0x0b, 0xd0, 0xa0, 0xe1);                     // mov	sp, fp

    put(p,
        0x20, 0x48, 0xbd, 0xe8,                         // pop	{r5, fp, lr}
        0x04, 0x20, 0x9f, 0xe5,                         // ldr	r2, [pc, #4]	; create_int64
        0x12, 0xff, 0x2f, 0xe1,                         // bx	r2
        abs_addr(cfn),                                  // .word
        abs_addr(create_int64)                          // .word
    );

    __clear_cache(code, p);
}
#else
void generate_code(char *p, void *cfn, Value param_types)
{
    auto param_count = get_array_size(param_types);
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
}
#endif

Force create_c_fn(void *cfn, Value name, Value ret_type, Value param_types)
{
    check_type("fn name", name, *type::Symbol);
    check_type("parameter types", param_types, *type::Array);
    auto param_count = get_array_size(param_types);
    if (param_count > MAX_ARGS)
        throw_illegal_argument("C functions can take up to " + std::to_string(MAX_ARGS) + ", got " + std::to_string(param_count));
    for (decltype(param_count) i = 0; i < param_count; ++i)
    {
        auto type = get_array_elem(param_types, i);
        if (!type.is(clib::int64) && !type.is(clib::string))
            throw_illegal_argument("Invalid parameter types: " + to_string(param_types));
    }

    const std::size_t max_size = 256;
    auto code = code_alloc(max_size);
    generate_code(code, cfn, param_types);

    enable_execution(code, max_size);
    Root caddr{create_int64(reinterpret_cast<Int64>(code))};
    return create_object3(*type::CFunction, *caddr, name, param_types);
}

std::uint64_t get_arg_bit_value(Value param_types, std::uint8_t i, Value arg)
{
    auto arg_tag = get_value_tag(arg);
    auto expected_type = get_array_elem(param_types, i);
    if (expected_type.is(clib::int64))
    {
        if (arg_tag != tag::INT64)
            throw_arg_type_error(arg, i);
        return static_cast<std::uint64_t>(get_int64_value(arg));
    }
    else
    {
        if (arg_tag != tag::STRING)
            throw_arg_type_error(arg, i);
        return reinterpret_cast<std::uint64_t>(get_string_ptr(arg));
    }
}

Force call_c_function(const Value *args, std::uint8_t num_args)
{
    auto fn = args[0];
    auto addr = get_object_element(fn, 0);
    auto name = get_object_element(fn, 1);
    auto param_types = get_object_element(fn, 2);
    if ((num_args - 1) != get_array_size(param_types))
        throw_arity_error(name, num_args - 1);
    std::uint64_t raw_args[MAX_ARGS];
    for (decltype(num_args) i = 1; i < num_args; ++i)
        raw_args[i - 1] = get_arg_bit_value(param_types, i - 1, args[i]);
    return reinterpret_cast<CFunction>(get_int64_value(addr))(raw_args, num_args - 1);
}

Force import_c_fn(Value libname, Value fnname, Value ret_type, Value param_types)
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
