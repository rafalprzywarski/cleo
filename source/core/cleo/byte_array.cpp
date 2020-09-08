#include "byte_array.hpp"
#include "global.hpp"
#include "util.hpp"
#include "hash.hpp"

namespace cleo
{

namespace
{
std::uint32_t int_size(Int64 bsize)
{
    return (bsize + (sizeof(Int64) - 1)) / sizeof(Int64);
}

Int64 check_int64(Value val)
{
    if (get_value_tag(val) != tag::INT64)
        throw_illegal_argument("Can only put Int64's in a byte array");
    return get_int64_value(val);
}

}

Force create_byte_array(const Value *elems, Int64 size)
{
    Root a{create_object(*type::ByteArray, nullptr, 1 + int_size(size), nullptr, 0)};
    set_dynamic_object_int(*a, 0, size);
    for (Int64 i = 0; i < size; ++i)
        set_dynamic_object_int_byte(*a, i + sizeof(Int64), check_int64(elems[i]));
    return *a;
}

Force byte_array_seq(Value v)
{
    if (get_byte_array_size(v) == 0)
        return nil;
    return create_static_object(*type::ByteArraySeq, v, 0);
}

Value get_byte_array_seq_first(Value s)
{
    return get_byte_array_elem(get_static_object_element(s, 0), get_static_object_int(s, 1));
}

Force get_byte_array_seq_next(Value s)
{
    auto v = get_static_object_element(s, 0);
    auto i = get_static_object_int(s, 1) + 1;
    if (get_byte_array_size(v) == i)
        return nil;
    return create_static_object(*type::ByteArraySeq, v, i);
}

Force byte_array_conj(Value v, Value e)
{
    auto size = get_byte_array_size(v);
    Root a{create_object(*type::ByteArray, nullptr, 1 + int_size(size + 1), nullptr, 0)};
    set_dynamic_object_int(*a, 0, size + 1);
    std::memcpy(get_dynamic_object_mut_int_ptr(*a, 1), get_dynamic_object_int_ptr(v, 1), size);
    set_dynamic_object_int_byte(*a, sizeof(Int64) + size, check_int64(e));
    return *a;
}

Force byte_array_pop(Value v)
{
    auto size = get_byte_array_size(v);
    if (size == 0)
    {
        Root msg{create_string("Can't pop an empty array")};
        throw_exception(new_illegal_state(*msg));
    }
    if (size == 1)
        return *EMPTY_BYTE_ARRAY;

    Root a{create_object(*type::ByteArray, nullptr, 1 + int_size(size - 1), nullptr, 0)};
    set_dynamic_object_int(*a, 0, size - 1);
    std::memcpy(get_dynamic_object_mut_int_ptr(*a, 1), get_dynamic_object_int_ptr(v, 1), size - 1);
    return *a;
}

Force byte_array_hash(Value v)
{
    std::uint64_t h = 0;
    auto size = get_byte_array_size(v);
    for (Int64 i = 0; i < size; ++i)
        h = h * 31 + hash_value(get_byte_array_elem(v, i));
    return create_int64(h * 31 + size);
}

Force transient_byte_array(Value v)
{
    Int64 size = get_byte_array_size(v);
    auto capacity = (size < 16 ? 32 : (size * 2));
    Root t{create_object(*type::TransientByteArray, nullptr, 1 + int_size(capacity), nullptr, 0)};
    std::memcpy(get_dynamic_object_mut_int_ptr(*t, 0), get_dynamic_object_int_ptr(v, 0), sizeof(Int64) + size);
    return *t;
}

Value get_transient_byte_array_elem(Value v, Int64 index)
{
    return index >= 0 && index < get_transient_byte_array_size(v) ? create_int48(get_dynamic_object_int_byte(v, sizeof(Int64) + index)) : nil;
}

Force transient_byte_array_conj(Value v, Value e)
{
    auto capacity = (get_dynamic_object_int_size(v) - 1) * Int64(sizeof(Int64));
    auto size = get_transient_byte_array_size(v);
    auto new_size = size + 1;
    if (size < capacity)
    {
        set_dynamic_object_int(v, 0, new_size);
        set_dynamic_object_int_byte(v, sizeof(Int64) + size, check_int64(e));
        return v;
    }
    Root t{create_object(*type::TransientByteArray, nullptr, 1 + int_size(capacity * 2), nullptr, 0)};
    std::memcpy(get_dynamic_object_mut_int_ptr(*t, 0), get_dynamic_object_int_ptr(v, 0), sizeof(Int64) + size);
    set_dynamic_object_int(*t, 0, new_size);
    set_dynamic_object_int_byte(*t, sizeof(Int64) + size, check_int64(e));
    return *t;
}

Force transient_byte_array_pop(Value v)
{
    auto size = get_transient_byte_array_size(v);
    if (size == 0)
    {
        Root msg{create_string("Can't pop an empty array")};
        throw_exception(new_illegal_state(*msg));
    }
    set_dynamic_object_int(v, 0, size - 1);
    return v;
}

Force transient_byte_array_assoc_elem(Value v, Int64 index, Value e)
{
    auto size = get_transient_byte_array_size(v);
    if (index > size)
        throw_index_out_of_bounds();
    if (index == size)
        return transient_byte_array_conj(v, e);
    set_dynamic_object_int_byte(v, sizeof(Int64) + index, check_int64(e));
    return v;
}

Force transient_byte_array_persistent(Value v)
{
    set_object_type(v, *type::ByteArray);
    return v;
}

}
