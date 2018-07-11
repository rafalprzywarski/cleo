#include "bytecode_fn.hpp"
#include "global.hpp"

namespace cleo
{

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

}
