#include "small_set.hpp"
#include "global.hpp"

namespace cleo
{

Force create_small_set()
{
    return create_object0(*type::SmallSet);
}

std::uint32_t get_small_set_size(Value s)
{
    return get_object_size(s);
}

Value get_small_set_elem(Value s, std::uint32_t index)
{
    return index < get_object_size(s) ? get_object_element(s, index) : nil;
}

Value small_set_get(Value s, Value k)
{
    return small_set_contains(s, k) ? k : nil;
}

Force small_set_conj(Value s, Value k)
{
    if (small_set_contains(s, k))
        return s;
    auto size = get_small_set_size(s);
    std::vector<Value> new_elems;
    new_elems.reserve(size + 1);
    for (decltype(size) i = 0; i < size; ++i)
        new_elems.push_back(get_small_set_elem(s, i));
    new_elems.push_back(k);
    return create_object(*type::SmallSet, new_elems.data(), new_elems.size());
}

Value small_set_contains(Value s, Value k)
{
    auto size = get_small_set_size(s);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (are_equal(k, get_object_element(s, i)))
            return TRUE;
    }
    return nil;
}

Force small_set_seq(Value s)
{
    if (get_small_set_size(s) == 0)
        return nil;
    Root index{create_int64(0)};
    return create_object2(*type::SmallSetSeq, s, *index);
}

Value get_small_set_seq_first(Value s)
{
    return get_small_set_elem(get_object_element(s, 0), get_int64_value(get_object_element(s, 1)));
}

Force get_small_set_seq_next(Value s)
{
    auto set = get_object_element(s, 0);
    auto i = get_int64_value(get_object_element(s, 1)) + 1;
    if (get_small_set_size(set) == i)
        return nil;
    Root index{create_int64(i)};
    return create_object2(*type::SmallSetSeq, set, *index);
}

}
