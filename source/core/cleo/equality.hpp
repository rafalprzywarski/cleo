#pragma once
#include "value.hpp"

namespace cleo
{

Value are_equal(Value left, Value right);

struct std_equal_to
{
    using result_type = bool;
    using first_argument_type = Value;
    using second_argument_type = Value;

    result_type operator()(first_argument_type left, second_argument_type right) const
    {
        return are_equal(left, right);
    }
};

}
