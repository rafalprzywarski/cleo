#include "array_map.hpp"
#include "global.hpp"
#include "equality.hpp"
#include "array.hpp"

namespace cleo
{

Force create_array_map()
{
    return create_object(*type::ArrayMap, nullptr, 0);
}

std::uint32_t get_array_map_size(Value m)
{
    return get_dynamic_object_size(m) / 2;
}

Value get_array_map_key(Value m, std::uint32_t index)
{
    return get_dynamic_object_element(m, index * 2);
}

Value get_array_map_val(Value m, std::uint32_t index)
{
    return get_dynamic_object_element(m, index * 2 + 1);
}

Value array_map_get(Value m, Value k)
{
    return array_map_get(m, k, nil);
}

Value array_map_get(Value m, Value k, Value def_v)
{
    auto size = get_array_map_size(m);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (k == get_dynamic_object_element(m, i * 2))
            return get_dynamic_object_element(m, i * 2 + 1);
    }
    return def_v;
}

Force array_map_assoc(Value m, Value k, Value v)
{
    auto size = get_array_map_size(m);
    std::vector<Value> kvs;
    kvs.reserve((size + 1) * 2);
    bool replaced = false;
    for (decltype(size) i = 0; i != size; ++i)
    {
        auto ck = get_dynamic_object_element(m, i * 2);
        kvs.push_back(ck);
        if (ck == k)
        {
            kvs.push_back(v);
            replaced = true;
        }
        else
            kvs.push_back(get_dynamic_object_element(m, i * 2 + 1));
    }
    if (!replaced)
    {
        kvs.push_back(k);
        kvs.push_back(v);
    }
    return create_object(*type::ArrayMap, kvs.data(), kvs.size());
}

Force array_map_dissoc(Value m, Value k)
{
    if (!array_map_contains(m, k))
        return m;
    auto size = get_array_map_size(m);
    std::vector<Value> kvs;
    kvs.reserve((size - 1) * 2);
    for (decltype(size) i = 0; i != size; ++i)
    {
        auto ck = get_dynamic_object_element(m, i * 2);
        if (k != ck)
        {
            kvs.push_back(ck);
            kvs.push_back(get_dynamic_object_element(m, i * 2 + 1));
        }
    }
    return create_object(*type::ArrayMap, kvs.data(), kvs.size());
}

Force array_map_merge(Value l, Value r)
{
    auto l_size = get_array_map_size(l);
    auto r_size = get_array_map_size(r);
    std::vector<Value> kvs;
    kvs.reserve(l_size + r_size);
    for (decltype(r_size) i = 0; i != r_size; ++i)
    {
        kvs.push_back(get_array_map_key(r, i));
        kvs.push_back(get_array_map_val(r, i));
    }
    for (decltype(l_size) i = 0; i != l_size; ++i)
    {
        auto key = get_array_map_key(l, i);
        if (array_map_contains(r, key))
            continue;

        kvs.push_back(key);
        kvs.push_back(get_array_map_val(l, i));
    }
    return create_object(*type::ArrayMap, kvs.data(), kvs.size());
}

Value array_map_contains(Value m, Value k)
{
    auto size = get_array_map_size(m);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (k == get_dynamic_object_element(m, i * 2))
            return TRUE;
    }
    return nil;
}

Force array_map_seq(Value m)
{
    if (get_array_map_size(m) == 0)
        return nil;
    std::array<Value, 2> kv{{get_array_map_key(m, 0), get_array_map_val(m, 0)}};
    Root kvv{create_array(kv.data(), kv.size())};
    return create_object3(*type::ArrayMapSeq, *kvv, m, *ONE);
}

Value get_array_map_seq_first(Value s)
{
    return get_static_object_element(s, 0);
}

Force get_array_map_seq_next(Value s)
{
    auto index = get_static_object_int(s, 2);
    auto m = get_static_object_element(s, 1);
    if (index == get_array_map_size(m))
        return nil;
    std::array<Value, 2> kv{{get_array_map_key(m, index), get_array_map_val(m, index)}};
    Root kvv{create_array(kv.data(), kv.size())};
    return create_static_object(*type::ArrayMapSeq, *kvv, m, index + 1);
}

}
