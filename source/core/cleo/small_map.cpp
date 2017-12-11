#include "small_map.hpp"
#include "global.hpp"
#include "equality.hpp"

namespace cleo
{

Force create_small_map()
{
    return create_object(type::SMALL_MAP, nullptr, 0);
}

std::uint32_t get_small_map_size(Value m)
{
    return get_object_size(m) / 2;
}

Value get_small_map_key(Value m, std::uint32_t index)
{
    return get_object_element(m, index * 2);
}

Value get_small_map_val(Value m, std::uint32_t index)
{
    return get_object_element(m, index * 2 + 1);
}

Value small_map_get(Value m, Value k)
{
    auto size = get_small_map_size(m);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (are_equal(k, get_object_element(m, i * 2)))
            return get_object_element(m, i * 2 + 1);
    }
    return nil;
}

Force small_map_assoc(Value m, Value k, Value v)
{
    auto size = get_small_map_size(m);
    std::vector<Value> kvs;
    kvs.reserve((size + 1) * 2);
    bool replaced = false;
    for (decltype(size) i = 0; i != size; ++i)
    {
        auto ck = get_object_element(m, i * 2);
        kvs.push_back(ck);
        if (are_equal(ck, k))
        {
            kvs.push_back(v);
            replaced = true;
        }
        else
            kvs.push_back(get_object_element(m, i * 2 + 1));
    }
    if (!replaced)
    {
        kvs.push_back(k);
        kvs.push_back(v);
    }
    return create_object(type::SMALL_MAP, kvs.data(), kvs.size());
}

Force small_map_merge(Value l, Value r)
{
    Root m{l};
    auto size = get_small_map_size(r);
    for (decltype(size) i = 0; i != size; ++i)
        m = small_map_assoc(*m, get_object_element(r, i * 2), get_object_element(r, i * 2 + 1));
    return *m;
}

Value small_map_contains(Value m, Value k)
{
    auto size = get_small_map_size(m);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (are_equal(k, get_object_element(m, i * 2)))
            return TRUE;
    }
    return nil;
}

}
