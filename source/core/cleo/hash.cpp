#include "hash.hpp"
#include "global.hpp"
#include "multimethod.hpp"

namespace cleo
{

Int64 hash_value(Value val)
{
    switch (get_value_tag(val))
    {
        case tag::NATIVE_FUNCTION:
            return std::hash<Value>{}(val);
        case tag::SYMBOL:
            return Int64(hash_value(get_symbol_namespace(val))) * 31 + hash_value(get_symbol_name(val));
        case tag::KEYWORD:
            return Int64(hash_value(get_keyword_namespace(val))) * 31 + hash_value(get_keyword_name(val));
        case tag::INT64:
            return std::hash<Int64>{}(get_int64_value(val));
        case tag::FLOAT64:
            return std::hash<Float64>{}(get_float64_value(val));
        case tag::STRING:
            return std::hash<std::string>{}({get_string_ptr(val), get_string_len(val)});
        case tag::OBJECT_TYPE:
            return hash_value(get_object_type_name(val));
        case tag::OBJECT:
        {
            if (val.is_nil())
                return 0;
            Root h{call_multimethod1(*rt::hash_obj, val)};
            return get_int64_value(*h);
        }
    }
    return 0;
}

Force hash(Value val)
{
    return create_int64(hash_value(val));
}

}
