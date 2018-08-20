#include "array.hpp"
#include "global.hpp"

namespace cleo
{

Force create_array(const Value *elems, std::uint32_t size)
{
    return create_object(*type::Array, elems, size);
}

std::uint32_t get_array_size(Value v)
{
    return get_object_size(v);
}

Value get_array_elem(Value v, std::uint32_t index)
{
    return index < get_object_size(v) ? get_object_element(v, index) : nil;
}

Force array_seq(Value v)
{
    if (get_array_size(v) == 0)
        return nil;
    return create_object1_1(*type::ArraySeq, 0, v);
}

Value get_array_seq_first(Value s)
{
    return get_array_elem(get_object_element(s, 0), get_object_int(s, 0));
}

Force get_array_seq_next(Value s)
{
    auto v = get_object_element(s, 0);
    auto i = get_object_int(s, 0) + 1;
    if (get_array_size(v) == i)
        return nil;
    return create_object1_1(*type::ArraySeq, i, v);
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

Force transient_array(Value v)
{
    auto size = get_array_size(v);
    auto capacity = (size < 16 ? 32 : (size * 2));
    Root t{create_object(*type::TransientArray, nullptr, capacity + 1)};
    Root tsize{create_int64(size)};
    set_object_element(*t, get_object_size(*t) - 1, *tsize);
    for (std::uint32_t i = 0; i < size; ++i)
        set_object_element(*t, i, get_object_element(v, i));
    return *t;
}

Value get_transient_array_size(Value v)
{
    return get_object_element(v, get_object_size(v) - 1);
}

Value get_transient_array_elem(Value v, std::uint32_t index)
{
    return index < get_int64_value(get_transient_array_size(v)) ? get_object_element(v, index) : nil;
}

Force transient_array_conj(Value v, Value e)
{
    auto capacity = get_object_size(v) - 1;
    auto size = get_int64_value(get_transient_array_size(v));
    Root new_size{create_int64(size + 1)};
    if (size < capacity)
    {
        set_object_element(v, capacity, *new_size);
        set_object_element(v, size, e);
        return v;
    }
    Root t{create_object(*type::TransientArray, nullptr, capacity * 2)};
    set_object_element(*t, capacity * 2 - 1, *new_size);
    for (std::uint32_t i = 0; i < size; ++i)
        set_object_element(*t, i, get_object_element(v, i));
    set_object_element(*t, capacity, e);
    return *t;
}

Force transient_array_persistent(Value v)
{
    set_object_size(v, get_int64_value(get_transient_array_size(v)));
    set_object_type(v, *type::Array);
    return v;
}

}
