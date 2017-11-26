#include "hash.hpp"
#include <functional>

namespace cleo
{

Value hash(Value val)
{
    switch (get_value_tag(val))
    {
        case tag::NATIVE_FUNCTION:
        case tag::SYMBOL:
        case tag::KEYWORD:
            return create_int64(std::hash<Value>{}(val));
        case tag::INT64:
            return create_int64(std::hash<Int64>{}(get_int64_value(val)));
        case tag::FLOAT64:
            return create_int64(std::hash<Float64>{}(get_float64_value(val)));
        case tag::STRING:
            return create_int64(std::hash<std::string>{}({get_string_ptr(val), get_string_len(val)}));
    }
    return create_int64(0);
}

}
