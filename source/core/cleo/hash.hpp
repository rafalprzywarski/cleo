#pragma once
#include "value.hpp"

namespace cleo
{

Int64 hash_value(Value val);
Force hash(Value val);

struct StdHash
{
    using argument_type = Value;
    using result_type = std::size_t;

    result_type operator()(argument_type val) const
    {
        return hash_value(val);
    }
};

}
