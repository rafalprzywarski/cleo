#include "equality.hpp"
#include "small_vector.hpp"
#include "global.hpp"
#include <array>

namespace cleo
{

Value are_small_vectors_equal(Value left, Value right)
{
    auto size = get_small_vector_size(left);
    if (size != get_small_vector_size(right))
        return nil;
    for (decltype(size) i = 0; i < size; ++i)
        if (!are_equal(get_small_vector_elem(left, i), get_small_vector_elem(right, i)))
            return nil;
    return TRUE;
}

Value are_equal(Value left, Value right)
{
    if (left == right)
        return TRUE;

    auto left_tag = get_value_tag(left);
    auto right_tag = get_value_tag(right);

    if (left_tag != right_tag)
        return nil;

    switch (left_tag)
    {
        case tag::INT64:
            return get_int64_value(left) == get_int64_value(right);
        case tag::FLOAT64:
            return get_float64_value(left) == get_float64_value(right);
        case tag::STRING:
            return
                get_string_len(left) == get_string_len(right) &&
                std::memcmp(get_string_ptr(left), get_string_ptr(right), get_string_len(left)) == 0;
        case tag::OBJECT:
            if (get_object_type(left) == type::SMALL_VECTOR && get_object_type(right) == type::SMALL_VECTOR)
                return are_small_vectors_equal(left, right);
            return nil;
        default:
            return nil;
    }
}

}
