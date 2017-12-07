#include "hash.hpp"
#include <functional>

namespace cleo
{

Int64 hash_value(Value val)
{
    switch (get_value_tag(val))
    {
        case tag::NATIVE_FUNCTION:
        case tag::SYMBOL:
        case tag::KEYWORD:
            return std::hash<Value>{}(val);
        case tag::INT64:
            return std::hash<Int64>{}(get_int64_value(val));
        case tag::FLOAT64:
            return std::hash<Float64>{}(get_float64_value(val));
        case tag::STRING:
            return std::hash<std::string>{}({get_string_ptr(val), get_string_len(val)});
    }
    return 0;
}

Force hash(Value val)
{
    return create_int64(hash_value(val));
}

}
