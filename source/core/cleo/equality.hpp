#pragma once
#include "value.hpp"

namespace cleo
{

Value are_seqables_equal(Value left, Value right);
Value are_array_sets_equal(Value left, Value right);
Value are_array_maps_equal(Value left, Value right);
Value are_maps_equal(Value left, Value right);
Value are_equal(Value left, Value right);

inline bool operator==(Value left, Value right)
{
    return bool(are_equal(left, right));
}

inline bool operator!=(Value left, Value right)
{
    return !(left == right);
}

struct StdIs
{
    using result_type = bool;
    using first_argument_type = Value;
    using second_argument_type = Value;

    result_type operator()(first_argument_type left, second_argument_type right) const
    {
        return left.is(right);
    }
};

}
