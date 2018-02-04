#include "persistent_hash_map.hpp"
#include "global.hpp"

namespace cleo
{

Force create_persistent_hash_map()
{
    return create_object3(*type::PersistentHashMap, *ZERO, nil, nil);
}

Int64 get_persistent_hash_map_size(Value m)
{
    return get_int64_value(get_object_element(m, 0));
}

Value persistent_hash_map_get(Value m, Value k)
{
    return get_object_element(m, 1) == k ? get_object_element(m, 2) : nil;
}

Force persistent_hash_map_assoc(Value m, Value k, Value v)
{
    return create_object3(*type::PersistentHashMap, *ONE, k, v);
}

Value persistent_hash_map_contains(Value m, Value k)
{
    return get_object_element(m, 1) == k ? TRUE : nil;
}

}
