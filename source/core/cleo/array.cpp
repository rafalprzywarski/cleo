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
    Root index{create_int64(0)};
    return create_object2(*type::ArraySeq, v, *index);
}

Value get_array_seq_first(Value s)
{
    return get_array_elem(get_object_element(s, 0), get_int64_value(get_object_element(s, 1)));
}

Force get_array_seq_next(Value s)
{
    auto v = get_object_element(s, 0);
    auto i = get_int64_value(get_object_element(s, 1)) + 1;
    if (get_array_size(v) == i)
        return nil;
    Root index{create_int64(i)};
    return create_object2(*type::ArraySeq, v, *index);
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

}
