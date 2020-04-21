#include "array.hpp"
#include "global.hpp"
#include "util.hpp"
#include "hash.hpp"

namespace cleo
{

Force create_array(const Value *elems, std::uint32_t size)
{
    return create_object(*type::Array, elems, size);
}

Force array_seq(Value v)
{
    if (get_array_size(v) == 0)
        return nil;
    return create_static_object(*type::ArraySeq, v, 0);
}

Value get_array_seq_first(Value s)
{
    return get_array_elem(get_static_object_element(s, 0), get_static_object_int(s, 1));
}

Force get_array_seq_next(Value s)
{
    auto v = get_static_object_element(s, 0);
    auto i = get_static_object_int(s, 1) + 1;
    if (get_array_size(v) == i)
        return nil;
    return create_static_object(*type::ArraySeq, v, i);
}

Force array_conj(Value v, Value e)
{
    auto size = get_array_size(v);
    std::vector<Value> new_elems;
    new_elems.reserve(size + 1);
    for (decltype(size) i = 0; i < size; ++i)
        new_elems.push_back(get_array_elem(v, i));
    new_elems.push_back(e);
    return create_array(&new_elems.front(), new_elems.size());
}

Force array_pop(Value v)
{
    auto size = get_array_size(v);
    if (size == 0)
    {
        Root msg{create_string("Can't pop an empty array")};
        throw_exception(new_illegal_state(*msg));
    }
    if (size == 1)
        return *EMPTY_VECTOR;
    std::vector<Value> popped;
    popped.reserve(size - 1);
    for (decltype(size) i = 0; i < (size - 1); ++i)
        popped.push_back(get_array_elem(v, i));
    return create_array(popped.data(), popped.size());
}

Force array_hash(Value v)
{
    std::uint64_t h = 0;
    auto size = get_array_size(v);
    for (Int64 i = 0; i < size; ++i)
        h = h * 31 + hash_value(get_array_elem(v, i));
    return create_int64(h * 31 + size);
}

Force transient_array(Value v)
{
    Int64 size = get_array_size(v);
    auto capacity = (size < 16 ? 32 : (size * 2));
    Root t{create_object(*type::TransientArray, &size, 1, nullptr, capacity)};
    for (std::uint32_t i = 0; i < size; ++i)
        set_dynamic_object_element(*t, i, get_dynamic_object_element(v, i));
    return *t;
}

Value get_transient_array_elem(Value v, std::uint32_t index)
{
    return index < get_transient_array_size(v) ? get_dynamic_object_element(v, index) : nil;
}

Force transient_array_conj(Value v, Value e)
{
    auto capacity = get_dynamic_object_size(v);
    auto size = get_transient_array_size(v);
    auto new_size = size + 1;
    if (size < capacity)
    {
        set_dynamic_object_int(v, 0, new_size);
        set_dynamic_object_element(v, size, e);
        return v;
    }
    Root t{create_object(*type::TransientArray, &new_size, 1, nullptr, capacity * 2)};
    for (std::uint32_t i = 0; i < size; ++i)
        set_dynamic_object_element(*t, i, get_dynamic_object_element(v, i));
    set_dynamic_object_element(*t, size, e);
    return *t;
}

Force transient_array_pop(Value v)
{
    auto size = get_transient_array_size(v);
    if (size == 0)
    {
        Root msg{create_string("Can't pop an empty array")};
        throw_exception(new_illegal_state(*msg));
    }
    set_dynamic_object_element(v, size - 1, nil); // GC
    set_dynamic_object_int(v, 0, size - 1);
    return v;
}

Force transient_array_assoc_elem(Value v, std::uint32_t index, Value e)
{
    auto size = get_transient_array_size(v);
    if (index > size)
        throw_index_out_of_bounds();
    if (index == size)
        return transient_array_conj(v, e);
    set_dynamic_object_element(v, index, e);
    return v;
}

Force transient_array_persistent(Value v)
{
    set_dynamic_object_size(v, get_transient_array_size(v));
    set_object_type(v, *type::Array);
    return v;
}

}
