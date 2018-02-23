#include "small_map.hpp"
#include "global.hpp"
#include "equality.hpp"
#include "small_vector.hpp"

namespace cleo
{

Force create_small_map()
{
    return create_object(*type::SmallMap, nullptr, 0);
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
        if (k == get_object_element(m, i * 2))
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
        if (ck == k)
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
    return create_object(*type::SmallMap, kvs.data(), kvs.size());
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
        if (k == get_object_element(m, i * 2))
            return TRUE;
    }
    return nil;
}

Force small_map_seq(Value m)
{
    if (get_small_map_size(m) == 0)
        return nil;
    std::array<Value, 2> kv{{get_small_map_key(m, 0), get_small_map_val(m, 0)}};
    Root kvv{create_small_vector(kv.data(), kv.size())};
    return create_object3(*type::SmallMapSeq, *kvv, m, *ONE);
}

Value get_small_map_seq_first(Value s)
{
    return get_object_element(s, 0);
}

Force get_small_map_seq_next(Value s)
{
    auto index = get_int64_value(get_object_element(s, 2));
    auto m = get_object_element(s, 1);
    if (index == get_small_map_size(m))
        return nil;
    std::array<Value, 2> kv{{get_small_map_key(m, index), get_small_map_val(m, index)}};
    Root kvv{create_small_vector(kv.data(), kv.size())};
    Root new_index{create_int64(index + 1)};
    return create_object3(*type::SmallMapSeq, *kvv, m, *new_index);
}

}
