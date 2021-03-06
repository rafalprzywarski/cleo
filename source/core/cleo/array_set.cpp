#include "array_set.hpp"
#include "global.hpp"

namespace cleo
{

Force create_array_set()
{
    return create_object0(*type::ArraySet);
}

std::uint32_t get_array_set_size(Value s)
{
    return get_dynamic_object_size(s);
}

Value get_array_set_elem(Value s, std::uint32_t index)
{
    return index < get_dynamic_object_size(s) ? get_dynamic_object_element(s, index) : nil;
}

Value array_set_get(Value s, Value k)
{
    return array_set_get(s, k, nil);
}

Value array_set_get(Value s, Value k, Value def_v)
{
    return array_set_contains(s, k) ? k : def_v;
}

Force array_set_conj(Value s, Value k)
{
    if (array_set_contains(s, k))
        return s;
    auto size = get_array_set_size(s);
    std::vector<Value> new_elems;
    new_elems.reserve(size + 1);
    for (decltype(size) i = 0; i < size; ++i)
        new_elems.push_back(get_array_set_elem(s, i));
    new_elems.push_back(k);
    return create_object(*type::ArraySet, new_elems.data(), new_elems.size());
}

Value array_set_contains(Value s, Value k)
{
    auto size = get_array_set_size(s);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (k == get_dynamic_object_element(s, i))
            return TRUE;
    }
    return nil;
}

Force array_set_seq(Value s)
{
    if (get_array_set_size(s) == 0)
        return nil;
    return create_static_object(*type::ArraySetSeq, s, 0);
}

Value get_array_set_seq_first(Value s)
{
    return get_array_set_elem(get_static_object_element(s, 0), get_static_object_int(s, 1));
}

Force get_array_set_seq_next(Value s)
{
    auto set = get_static_object_element(s, 0);
    auto i = get_static_object_int(s, 1) + 1;
    if (get_array_set_size(set) == i)
        return nil;
    return create_static_object(*type::ArraySetSeq, set, i);
}

}
