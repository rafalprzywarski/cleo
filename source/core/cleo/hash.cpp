#include "hash.hpp"
#include "global.hpp"
#include "multimethod.hpp"

namespace cleo
{

namespace
{

template <std::uint32_t get_hash(Value), void set_hash(Value, std::uint32_t), std::uint32_t hash(Value)>
std::uint32_t hash_memoized(Value val)
{
    auto h = get_hash(val);
    if (h == 0)
    {
        h = hash(val);
        set_hash(val, h);
    }
    return h;
}

std::uint32_t hash_string(Value s)
{
    return std::uint32_t(std::hash<std::string>{}({get_string_ptr(s), get_string_len(s)}));
}

std::uint32_t hash_symbol(Value s)
{
    return std::uint32_t(hash_value(get_symbol_namespace(s)) * 31 + hash_value(get_symbol_name(s)));
};

std::uint32_t hash_keyword(Value k)
{
    return std::uint32_t(hash_value(get_keyword_namespace(k)) * 31 + hash_value(get_keyword_name(k)));
}

}

Int64 hash_value(Value val)
{
    switch (get_value_tag(val))
    {
        case tag::NATIVE_FUNCTION:
            return std::hash<Value>{}(val);
        case tag::SYMBOL:
            return hash_memoized<get_symbol_hash, set_symbol_hash, hash_symbol>(val);
        case tag::KEYWORD:
            return hash_memoized<get_keyword_hash, set_keyword_hash, hash_keyword>(val);
        case tag::INT64:
            return std::hash<Int64>{}(get_int64_value(val));
        case tag::FLOAT64:
            return std::hash<Float64>{}(get_float64_value(val));
        case tag::STRING:
            return hash_memoized<get_string_hash, set_string_hash, hash_string>(val);
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
