#include "small_vector.hpp"
#include "global.hpp"

namespace cleo
{

Value create_small_vector(const Value *elems, std::uint32_t size)
{
    return create_object(type::SMALL_VECTOR, elems, size);
}

std::uint32_t get_small_vector_size(Value v)
{
    return get_object_size(v);
}

Value get_small_vector_elem(Value v, std::uint32_t index)
{
    return index < get_object_size(v) ? get_object_element(v, index) : nil;
}

Value small_vector_seq(Value v)
{
    return get_small_vector_size(v) == 0 ?
        nil
        : create_object2(type::SMALL_VECTOR_SEQ, v, create_int64(0));
}

Value get_small_vector_seq_first(Value s)
{
    return get_small_vector_elem(get_object_element(s, 0), get_int64_value(get_object_element(s, 1)));
}

Value get_small_vector_seq_next(Value s)
{
    auto v = get_object_element(s, 0);
    auto i = get_int64_value(get_object_element(s, 1)) + 1;

    return get_small_vector_size(v) == i ?
        nil :
        create_object2(type::SMALL_VECTOR_SEQ, v, create_int64(i));
}

}
