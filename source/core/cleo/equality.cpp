#include "equality.hpp"
#include <array>

namespace cleo
{

const Value TRUE = create_keyword("true");
const Value EQ = create_symbol("cleo.core", "=");

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
        default:
            return nil;
    }
}

}
