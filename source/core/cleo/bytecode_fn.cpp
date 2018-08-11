#include "bytecode_fn.hpp"
#include "global.hpp"
#include "array.hpp"
#include "multimethod.hpp"

namespace cleo
{

namespace
{

Force bytecode_fn_body_replace_consts(Value b, const Value *consts, Int64 n)
{
    auto old_consts = get_bytecode_fn_body_consts(b);
    auto csize = get_array_size(old_consts);
    std::vector<Value> mconsts(csize);
    for (decltype(csize) j = 0; (j + n) < csize; ++j)
        mconsts[j] = get_array_elem(old_consts, j);
    for (decltype(csize) j = 0; j < n; ++j)
        mconsts[csize - n + j] = consts[j];
    Root mrconsts{create_array(mconsts.data(), mconsts.size())};
    return create_bytecode_fn_body(*mrconsts, get_bytecode_fn_body_vars(b), get_bytecode_fn_body_locals_size(b), get_bytecode_fn_body_bytes(b), get_bytecode_fn_body_bytes_size(b));
}

}

Force create_bytecode_fn_exception_table(const Int64 *entries, const Value *types, std::uint32_t size)
{
    return create_object(*type::BytecodeFnExceptionTable, entries, size * 3, types, size);
}

std::uint32_t get_bytecode_fn_exception_table_size(Value et)
{
    return get_object_size(et);
}

Int64 get_bytecode_fn_exception_table_start_offset(Value et, std::uint32_t i)
{
    return get_object_int(et, i * 3);
}

Int64 get_bytecode_fn_exception_table_end_offset(Value et, std::uint32_t i)
{
    return get_object_int(et, i * 3 + 1);
}

Int64 get_bytecode_fn_exception_table_handler_offset(Value et, std::uint32_t i)
{
    return get_object_int(et, i * 3 + 2);
}

Value get_bytecode_fn_exception_table_type(Value et, std::uint32_t i)
{
    return get_object_element(et, i);
}

Int64 bytecode_fn_find_exception_handler(Value et, Int64 offset, Value type)
{
    auto size = get_bytecode_fn_exception_table_size(et);
    for (decltype(size) i = 0; i < size; ++i)
    {
        auto et_type = get_bytecode_fn_exception_table_type(et, i);
        if (offset >= get_bytecode_fn_exception_table_start_offset(et, i) &&
            offset < get_bytecode_fn_exception_table_end_offset(et, i) &&
            (isa(type, et_type) || et_type.is_nil()))
            return get_bytecode_fn_exception_table_handler_offset(et, i);
    }
    return -1;
}

Force create_bytecode_fn_body(Value consts, Value vars, Int64 locals_size, const vm::Byte *bytes, Int64 bytes_size)
{
    auto bytes_int_size = (bytes_size + sizeof(Int64) - 1) / sizeof(Int64);
    std::vector<Int64> ints(2 + bytes_int_size, 0);
    ints[0] = locals_size;
    ints[1] = bytes_size;
    std::memcpy(&ints[2], bytes, bytes_size);
    std::array<Value, 2> elems{{consts, vars}};
    return create_object(*type::BytecodeFnBody, ints.data(), ints.size(), elems.data(), elems.size());
}

Value get_bytecode_fn_body_consts(Value body)
{
    return get_object_element(body, 0);
}

Value get_bytecode_fn_body_vars(Value body)
{
    return get_object_element(body, 1);
}

Int64 get_bytecode_fn_body_locals_size(Value body)
{
    return get_object_int(body, 0);
}

const vm::Byte *get_bytecode_fn_body_bytes(Value body)
{
    return reinterpret_cast<const vm::Byte *>(get_object_int_ptr(body, 2));
}

Int64 get_bytecode_fn_body_bytes_size(Value body)
{
    return get_object_int(body, 1);
}

Force create_bytecode_fn(Value name, const Int64 *arities, const Value *bodies, std::uint8_t n)
{
    std::vector<Value> elems(1 + n);
    elems[0] = name;
    std::copy_n(bodies, n, begin(elems) + 1);
    return create_object(*type::BytecodeFn, arities, n, elems.data(), elems.size());
}

Value get_bytecode_fn_name(Value fn)
{
    return get_object_element(fn, 0);
}

std::uint8_t get_bytecode_fn_size(Value fn)
{
    return get_object_int_size(fn);
}

Int64 get_bytecode_fn_arity(Value fn, std::uint8_t i)
{
    return get_object_int(fn, i);
}

Value get_bytecode_fn_body(Value fn, std::uint8_t i)
{
    return get_object_element(fn, i + 1);
}

Force bytecode_fn_replace_consts(Value fn, const Value *consts, Int64 n)
{
    if (n == 0)
        return fn;
    auto size = get_bytecode_fn_size(fn);
    std::vector<Value> bodies(size);
    std::vector<Int64> arities(size);
    Roots rbodies(size);
    for (Int64 i = 0; i < size; ++i)
    {
        rbodies.set(i, bytecode_fn_body_replace_consts(get_bytecode_fn_body(fn, i), consts, n));
        bodies[i] = rbodies[i];
        arities[i] = get_bytecode_fn_arity(fn, i);
    }

    return create_bytecode_fn(get_bytecode_fn_name(fn), arities.data(), bodies.data(), bodies.size());
}

}
