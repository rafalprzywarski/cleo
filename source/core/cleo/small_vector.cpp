#include "small_vector.hpp"
#include "global.hpp"

namespace cleo
{

Force create_small_vector(const Value *elems, std::uint32_t size)
{
    return create_object(*type::SmallVector, elems, size);
}

std::uint32_t get_small_vector_size(Value v)
{
    return get_object_size(v);
}

Value get_small_vector_elem(Value v, std::uint32_t index)
{
    return index < get_object_size(v) ? get_object_element(v, index) : nil;
}

Force small_vector_seq(Value v)
{
    if (get_small_vector_size(v) == 0)
        return nil;
    Root index{create_int64(0)};
    return create_object2(*type::SmallVectorSeq, v, *index);
}

Value get_small_vector_seq_first(Value s)
{
    return get_small_vector_elem(get_object_element(s, 0), get_int64_value(get_object_element(s, 1)));
}

Force get_small_vector_seq_next(Value s)
{
    auto v = get_object_element(s, 0);
    auto i = get_int64_value(get_object_element(s, 1)) + 1;
    if (get_small_vector_size(v) == i)
        return nil;
    Root index{create_int64(i)};
    return create_object2(*type::SmallVectorSeq, v, *index);
}

Force small_vector_conj(Value v, Value e)
{
    auto size = get_small_vector_size(v);
    std::vector<Value> new_elems;
    new_elems.reserve(size + 1);
    for (decltype(size) i = 0; i < size; ++i)
        new_elems.push_back(get_small_vector_elem(v, i));
    new_elems.push_back(e);
    return create_small_vector(&new_elems.front(), new_elems.size());
}

}
