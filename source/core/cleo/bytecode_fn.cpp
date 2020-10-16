#include "bytecode_fn.hpp"
#include "global.hpp"
#include "array.hpp"
#include "multimethod.hpp"
#include "persistent_hash_set.hpp"
#include "eval.hpp"
#include <cstring>
#include <algorithm>

namespace cleo
{

namespace
{

Force create_bytecode_fn(Value type, Value name, const Value *bodies, std::uint8_t n, Value ast)
{
    std::vector<Int64> arities(n);
    std::transform(bodies, bodies + n, begin(arities), get_bytecode_fn_body_arity);
    std::vector<Value> elems(3 + n);
    elems[0] = name;
    elems[1] = ast;
    std::copy_n(bodies, n, begin(elems) + 3);
    return create_object(type, arities.data(), arities.size(), elems.data(), elems.size());
}

Force bytecode_fn_body_set_closed_vals(Value b, Value vals)
{
    return create_bytecode_fn_body(get_bytecode_fn_body_arity(b),
                                   get_bytecode_fn_body_consts(b),
                                   get_bytecode_fn_body_vars(b),
                                   vals,
                                   get_bytecode_fn_body_exception_table(b),
                                   get_bytecode_fn_body_locals_size(b),
                                   get_bytecode_fn_body_bytes(b),
                                   get_bytecode_fn_body_bytes_size(b));
}

}

Force create_bytecode_fn_exception_table(const Int64 *entries, const Value *types, std::uint32_t size)
{
    return create_object(*type::BytecodeFnExceptionTable, entries, size * 4, types, size);
}

std::uint32_t get_bytecode_fn_exception_table_size(Value et)
{
    return get_dynamic_object_size(et);
}

Int64 get_bytecode_fn_exception_table_start_offset(Value et, std::uint32_t i)
{
    return get_dynamic_object_int(et, i * 4);
}

Int64 get_bytecode_fn_exception_table_end_offset(Value et, std::uint32_t i)
{
    return get_dynamic_object_int(et, i * 4 + 1);
}

Int64 get_bytecode_fn_exception_table_handler_offset(Value et, std::uint32_t i)
{
    return get_dynamic_object_int(et, i * 4 + 2);
}

Int64 get_bytecode_fn_exception_table_stack_size(Value et, std::uint32_t i)
{
    return get_dynamic_object_int(et, i * 4 + 3);
}

Value get_bytecode_fn_exception_table_type(Value et, std::uint32_t i)
{
    return get_dynamic_object_element(et, i);
}

bytecode_fn_exception_handler bytecode_fn_find_exception_handler(Value et, Int64 offset, Value type)
{
    auto size = get_bytecode_fn_exception_table_size(et);
    for (decltype(size) i = 0; i < size; ++i)
    {
        auto et_type = get_bytecode_fn_exception_table_type(et, i);
        if (offset >= get_bytecode_fn_exception_table_start_offset(et, i) &&
            offset < get_bytecode_fn_exception_table_end_offset(et, i) &&
            (isa(type, et_type) || et_type.is_nil()))
            return {get_bytecode_fn_exception_table_handler_offset(et, i),
                    get_bytecode_fn_exception_table_stack_size(et, i)};
    }
    return {-1, -1};
}

Force create_bytecode_fn_body(Int64 arity, Value consts, Value vars, Value closed_vals, Value exception_table, Int64 locals_size, const vm::Byte *bytes, Int64 bytes_size)
{
    auto bytes_int_size = (bytes_size + sizeof(Int64) - 1) / sizeof(Int64);
    std::vector<Int64> ints(3 + bytes_int_size, 0);
    ints[0] = arity;
    ints[1] = locals_size;
    ints[2] = bytes_size;
    std::memcpy(&ints[3], bytes, bytes_size);
    std::array<Value, 4> elems{{consts, vars, closed_vals, exception_table}};
    return create_object(*type::BytecodeFnBody, ints.data(), ints.size(), elems.data(), elems.size());
}

Int64 get_bytecode_fn_body_arity(Value body)
{
    return get_dynamic_object_int(body, 0);
}

Value get_bytecode_fn_body_consts(Value body)
{
    return get_dynamic_object_element(body, 0);
}

Value get_bytecode_fn_body_vars(Value body)
{
    return get_dynamic_object_element(body, 1);
}

Value get_bytecode_fn_body_closed_vals(Value body)
{
    return get_dynamic_object_element(body, 2);
}

Value get_bytecode_fn_body_exception_table(Value body)
{
    return get_dynamic_object_element(body, 3);
}

Int64 get_bytecode_fn_body_locals_size(Value body)
{
    return get_dynamic_object_int(body, 1);
}

const vm::Byte *get_bytecode_fn_body_bytes(Value body)
{
    return reinterpret_cast<const vm::Byte *>(get_dynamic_object_int_ptr(body, 3));
}

Int64 get_bytecode_fn_body_bytes_size(Value body)
{
    return get_dynamic_object_int(body, 2);
}

Force create_bytecode_fn(Value name, const Value *bodies, std::uint8_t n, Value ast)
{
    return create_bytecode_fn(*type::BytecodeFn, name, bodies, n, ast);
}

Force create_open_bytecode_fn(Value name, const Value *bodies, std::uint8_t n, Value ast)
{
    return create_bytecode_fn(*type::OpenBytecodeFn, name, bodies, n, ast);
}

Value get_bytecode_fn_name(Value fn)
{
    return get_dynamic_object_element(fn, 0);
}

std::uint8_t get_bytecode_fn_size(Value fn)
{
    return get_dynamic_object_int_size(fn);
}

Int64 get_bytecode_fn_arity(Value fn, std::uint8_t i)
{
    return get_dynamic_object_int(fn, i);
}

Value get_bytecode_fn_body(Value fn, std::uint8_t i)
{
    return get_dynamic_object_element(fn, i + 3);
}

Force bytecode_fn_set_closed_vals(Value fn, Value vals)
{
    if (vals.is_nil())
        return fn;

    auto size = get_bytecode_fn_size(fn);
    std::vector<Value> bodies(size);
    Roots rbodies(size);
    for (Int64 i = 0; i < size; ++i)
    {
        rbodies.set(i, bytecode_fn_body_set_closed_vals(get_bytecode_fn_body(fn, i), vals));
        bodies[i] = rbodies[i];
    }

    return create_bytecode_fn(get_bytecode_fn_name(fn), bodies.data(), bodies.size(), nil);
}

std::pair<Value, Int64> bytecode_fn_find_body(Value fn, std::uint8_t arity)
{
    auto n = get_bytecode_fn_size(fn);
    for (decltype(n) i = 0; i < n; ++i)
        if (get_bytecode_fn_arity(fn, i) == arity)
            return {get_bytecode_fn_body(fn, i), arity};
    if (n > 0)
    {
        auto va_arity = get_bytecode_fn_arity(fn, n - 1);
        if (va_arity < 0 && ~va_arity <= arity)
            return {get_bytecode_fn_body(fn, n - 1), va_arity};
    }
    return {};
}

Value get_bytecode_fn_ast(Value fn)
{
    return get_dynamic_object_element(fn, 1);
}

void bytecode_fn_update_bodies(Value fn, Value src_fn)
{
    assert(get_dynamic_object_size(fn) == get_dynamic_object_size(src_fn));

    for (std::uint32_t size = get_dynamic_object_size(fn), i = 3; i != size; ++i)
        set_dynamic_object_element(fn, i, get_dynamic_object_element(src_fn, i));

    recompile_bytecode_fns(get_dynamic_object_element(fn, 2));
}

void recompile_bytecode_fn(Value fn)
{
    std::array<Value, 2> compile{{*rt::compile_fn_ast, get_bytecode_fn_ast(fn)}};
    Root fresh_fn{call(compile.data(), compile.size())};
    bytecode_fn_update_bodies(fn, *fresh_fn);
}

void recompile_bytecode_fns(Value fn_set)
{
    if (fn_set.is_nil())
        return;
    assert(get_value_type(fn_set).is(*type::PersistentHashSet));
    for (Root s{persistent_hash_set_seq(fn_set)}; *s; s = get_persistent_hash_set_seq_next(*s))
        recompile_bytecode_fn(get_persistent_hash_set_seq_first(*s));
}

Value add_bytecode_fn_fn_dep(Value fn, Value dep)
{
    auto deps = get_dynamic_object_element(fn, 2);
    if (deps.is_nil())
        deps = *EMPTY_HASH_SET;

    Root new_deps{persistent_hash_set_conj(deps, dep)};
    set_dynamic_object_element(fn, 2, *new_deps);

    return nil;
}

}
