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

}
