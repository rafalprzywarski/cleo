#include "persistent_hash_map.hpp"
#include "global.hpp"

namespace cleo
{

// HashMap:
//   size 0: [0 SENTINEL]
//   size 1: [1 value key]
//   size>1: [size collision-node]
//   size>1: [size array-node]
// CollisionNode:
//   [hash key0 value0 key1 value1 key2? value2? ...]
// ArrayNode:
//   [(value-map node-map) key0 value0 ...  node0]

namespace
{

auto popcount(std::uint32_t b)
{
    return __builtin_popcount(b);
}

void copy_object_elements(Value dst, std::uint32_t dst_index, Value src, std::uint32_t src_index, std::uint32_t src_end)
{
    for (auto i = src_index, j = dst_index; i != src_end; ++i, ++j)
        set_object_element(dst, j, get_object_element(src, i));
}

Force create_collision_node(std::uint32_t hash, Value k0, Value v0, Value k1, Value v1)
{
    Root h{create_int64(hash)};
    return create_object5(*type::PersistentHashMapCollisionNode, *h, k0, v0, k1, v1);
}

Force create_single_value_map(Value k, Value v)
{
    return create_object3(*type::PersistentHashMap, *ONE, v, k);
}

Force create_map(Value size, Value elem0)
{
    return create_object2(*type::PersistentHashMap, size, elem0);
}

Force create_map(Int64 size, Value elem0)
{
    Root s{create_int64(size)};
    return create_object2(*type::PersistentHashMap, *s, elem0);
}

Force create_collision_map(std::uint32_t hash, Value k0, Value v0, Value k1, Value v1)
{
    Root node{create_collision_node(hash, k0, v0, k1, v1)};
    return create_map(*TWO, *node);
}

Value collision_node_get(Value node, Value key, Value def_val)
{
    auto size = get_object_size(node);
    for (decltype(size) i = 1; i < size; i += 2)
        if (get_object_element(node, i) == key)
            return get_object_element(node, i + 1);
    return def_val;
}

Value collision_node_get(Value node, Value key, std::uint32_t key_hash, Value def_val)
{
    if (key_hash != get_int64_value(get_object_element(node, 0)))
        return def_val;
    return collision_node_get(node, key, def_val);
}

Value collision_node_assoc(Value node, Value k, Value v, bool& replaced)
{
    auto node_size = get_object_size(node);
    bool should_replace = !collision_node_get(node, k, *SENTINEL).is(*SENTINEL);
    if (should_replace)
    {
        Root new_node{create_object(*type::PersistentHashMapCollisionNode, nullptr, node_size)};
        set_object_element(*new_node, 0, get_object_element(node, 0));
        for (decltype(node_size) i = 1; i < node_size; i += 2)
        {
            auto ek = get_object_element(node, i);
            set_object_element(*new_node, i, ek);
            if (ek != k)
                set_object_element(*new_node, i + 1, get_object_element(node, i + 1));
            else
                set_object_element(*new_node, i + 1, v);
        }
        replaced = true;
        return *new_node;
    }
    else
    {
        Root new_node{create_object(*type::PersistentHashMapCollisionNode, nullptr, node_size + 2)};
        set_object_element(*new_node, 0, get_object_element(node, 0));
        copy_object_elements(*new_node, 1, node, 1, node_size);
        set_object_element(*new_node, node_size, k);
        set_object_element(*new_node, node_size + 1, v);
        replaced = false;
        return *new_node;
    }
}

Force create_array_node(Value key0, std::uint32_t key0_hash, Value val0, Value key1, std::uint32_t key1_hash, Value val1)
{
    std::uint32_t data_map = (1 << (key0_hash & 0x1f)) | (1 << (key1_hash & 0x1f));
    Root data_map_val{create_int64(data_map)};
    return create_object5(*type::PersistentHashMapArrayNode, *data_map_val, key0, val0, key1, val1);
}

Force create_array_map(Value key0, std::uint32_t key0_hash, Value val0, Value key1, std::uint32_t key1_hash, Value val1)
{
    Root node{create_array_node(key0, key0_hash, val0, key1, key1_hash, val1)};
    return create_map(*TWO, *node);
}

std::uint32_t map_bit(std::uint32_t hash)
{
    return 1 << (hash & 0x1f);
}

std::uint32_t map_key_index(std::uint32_t value_map, std::uint32_t bit)
{
    return 1 + 2 * popcount(value_map & (bit - 1));
}

std::uint32_t map_node_index(std::uint32_t node_map, std::uint32_t node_size, std::uint32_t bit)
{
    return node_size - popcount(node_map & (bit - 1)) - 1;
}

Int64 combine_maps(std::uint32_t value_map, std::uint32_t node_map)
{
    assert((value_map & node_map) == 0);
    return value_map | (std::uint64_t(node_map) << 32);
}

Value array_node_get(Value node, Value key, std::uint32_t key_hash, Value def_val)
{
    std::uint64_t value_node_map = get_int64_value(get_object_element(node, 0));
    std::uint32_t value_map{static_cast<std::uint32_t>(value_node_map)};
    std::uint32_t node_map{static_cast<std::uint32_t>(value_node_map >> 32)};
    assert((value_map & node_map) == 0);
    std::uint32_t key_bit = map_bit(key_hash);
    if (value_map & key_bit)
    {
        auto key_index = map_key_index(value_map, key_bit);
        if (get_object_element(node, key_index) != key)
            return def_val;
        return get_object_element(node, key_index + 1);
    }
    if (node_map & key_bit)
    {
        auto node_index = map_node_index(node_map, get_object_size(node), key_bit);
        return collision_node_get(get_object_element(node, node_index), key, key_hash, def_val);
    }
    return def_val;
}

Value array_node_assoc(Value node, Value key, std::uint32_t key_hash, Value val, bool& replaced)
{
    std::uint64_t value_node_map = get_int64_value(get_object_element(node, 0));
    std::uint32_t value_map{static_cast<std::uint32_t>(value_node_map)};
    std::uint32_t node_map{static_cast<std::uint32_t>(value_node_map >> 32)};
    std::uint32_t key_bit = map_bit(key_hash);
    auto key_index = map_key_index(value_map, key_bit);
    auto node_size = get_object_size(node);
    if (value_map & key_bit)
    {
        if (get_object_element(node, key_index) == key)
        {
            Root new_node{create_object(*type::PersistentHashMapArrayNode, nullptr, node_size)};

            copy_object_elements(*new_node, 0, node, 0, key_index);
            set_object_element(*new_node, key_index, key);
            set_object_element(*new_node, key_index + 1, val);
            copy_object_elements(*new_node, key_index + 2, node, key_index + 2, node_size);

            replaced = true;
            return *new_node;
        }
        else
        {
            Root new_node{create_object(*type::PersistentHashMapArrayNode, nullptr, node_size - 1)};
            Root new_value_map{create_int64(combine_maps(value_map ^ key_bit, node_map ^ key_bit))};
            auto node_index = map_node_index(node_map, node_size, key_bit);
            auto key0 = get_object_element(node, key_index);
            auto val0 = get_object_element(node, key_index + 1);
            Root collision{create_collision_node(key_hash, key0, val0, key, val)};

            set_object_element(*new_node, 0, *new_value_map);
            copy_object_elements(*new_node, 1, node, 1, key_index);
            copy_object_elements(*new_node, key_index, node, key_index + 2, node_index + 1);
            set_object_element(*new_node, node_index - 1, *collision);
            copy_object_elements(*new_node, node_index, node, node_index + 1, node_size);

            replaced = false;
            return *new_node;
        }
    }
    else if (node_map & key_bit)
    {
        Root new_node{create_object(*type::PersistentHashMapArrayNode, nullptr, node_size)};
        auto node_index = map_node_index(node_map, node_size, key_bit);
        Root collision{collision_node_assoc(get_object_element(node, node_index), key, val, replaced)};

        copy_object_elements(*new_node, 0, node, 0, node_index);
        set_object_element(*new_node, node_index, *collision);
        copy_object_elements(*new_node, node_index + 1, node, node_index + 1, node_size);

        return *new_node;
    }
    else
    {
        Root new_node{create_object(*type::PersistentHashMapArrayNode, nullptr, node_size + 2)};
        Root new_value_map{create_int64(combine_maps(value_map | key_bit, node_map))};
        set_object_element(*new_node, 0, *new_value_map);
        copy_object_elements(*new_node, 1, node, 1, key_index);
        set_object_element(*new_node, key_index, key);
        set_object_element(*new_node, key_index + 1, val);
        copy_object_elements(*new_node, key_index + 2, node, key_index, node_size);

        replaced = false;
        return *new_node;
    }
}

}

Force create_persistent_hash_map()
{
    return create_map(*ZERO, *SENTINEL);
}

Int64 get_persistent_hash_map_size(Value m)
{
    return get_int64_value(get_object_element(m, 0));
}

Value persistent_hash_map_get(Value m, Value k)
{
    return persistent_hash_map_get(m, k, nil);
}

Value persistent_hash_map_get(Value map, Value key, Value def_val)
{
    auto node_or_val = get_object_element(map, 1);
    if (node_or_val.is(*SENTINEL))
        return def_val;
    auto node_or_val_type = get_value_type(node_or_val);
    if (node_or_val_type.is(*type::PersistentHashMapCollisionNode))
    {
        std::uint32_t key_hash = hash_value(key);
        return collision_node_get(node_or_val, key, key_hash, def_val);
    }
    if (node_or_val_type.is(*type::PersistentHashMapArrayNode))
    {
        std::uint32_t key_hash = hash_value(key);
        return array_node_get(node_or_val, key, key_hash, def_val);
    }
    return get_object_element(map, 2) == key ? node_or_val : def_val;
}

Force persistent_hash_map_assoc(Value map, Value key, Value val)
{
    auto node_or_val = get_object_element(map, 1);
    if (node_or_val.is(*SENTINEL))
        return create_single_value_map(key, val);

    auto node_or_val_type = get_value_type(node_or_val);
    if (node_or_val_type.is(*type::PersistentHashMapCollisionNode))
    {
        auto replaced = false;
        Root new_node{collision_node_assoc(node_or_val, key, val, replaced)};
        auto size = get_persistent_hash_map_size(map);
        return create_map(replaced ? size : (size + 1), *new_node);
    }
    if (node_or_val_type.is(*type::PersistentHashMapArrayNode))
    {
        std::uint32_t key_hash = hash_value(key);
        auto replaced = false;
        Root new_node{array_node_assoc(node_or_val, key, key_hash, val, replaced)};
        auto size = get_persistent_hash_map_size(map);
        return create_map(replaced ? size : (size + 1), *new_node);
    }

    Value key0 = get_object_element(map, 2);
    if (key0 == key)
        return create_single_value_map(key, val);
    auto val0 = get_object_element(map, 1);
    auto key_hash = hash_value(key);
    auto key0_hash = hash_value(key0);
    if (key_hash == key0_hash)
        return create_collision_map(key_hash, key0, val0, key, val);
    return create_array_map(key0, key0_hash, val0, key, key_hash, val);
}

Value persistent_hash_map_contains(Value m, Value k)
{
    auto val = persistent_hash_map_get(m, k, *SENTINEL);
    return val.is(*SENTINEL) ? nil : TRUE;
}

}
