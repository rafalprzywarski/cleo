#include "persistent_hash_map.hpp"
#include "global.hpp"
#include "array.hpp"

namespace cleo
{

// HashMap:
//   size 0: [0 | SENTINEL]
//   size 1: [1 | value key]
//   size>1: [size | collision-node]
//   size>1: [size | array-node]
// CollisionNode:
//   [hash | key0 value0 key1 value1 key2? value2? ...]
// ArrayNode:
//   [(value-map node-map) | key0? value0? key1? value1? ... node2? node1? node0?]
// HashMapSeq:
//   size 1: [[first-key first-value] nil 0 nil]
//   size>1: [[first-key first-value] collision-node 0 #SeqParent[index array-node parent-or-nil]-or-nil]
//   size>1: [[first-key first-value] array-node 0 #SeqParent[index array-node parent-or-nil]-or-nil]

namespace
{

auto popcount(std::uint32_t b)
{
    return __builtin_popcount(b);
}

void copy_object_elements(Value dst, std::uint32_t dst_index, Value src, std::uint32_t src_index, std::uint32_t src_end)
{
    for (auto i = src_index, j = dst_index; i != src_end; ++i, ++j)
        set_dynamic_object_element(dst, j, get_dynamic_object_element(src, i));
}

Force create_collision_node(std::uint32_t hash, Value k0, Value v0, Value k1, Value v1)
{
    assert(static_cast<std::uint32_t>(hash_value(k0)) == static_cast<std::uint32_t>(hash_value(k1)));
    return create_object1_4(*type::PersistentHashMapCollisionNode, hash, k0, v0, k1, v1);
}

Force create_single_value_map(Value k, Value v)
{
    return create_object1_2(*type::PersistentHashMap, 1, v, k);
}

Force create_map(Int64 size, Value elem0)
{
    return create_object1_1(*type::PersistentHashMap, size, elem0);
}

Force create_collision_map(std::uint32_t hash, Value k0, Value v0, Value k1, Value v1)
{
    Root node{create_collision_node(hash, k0, v0, k1, v1)};
    return create_map(2, *node);
}

Value collision_node_get(Value node, std::uint32_t size, Value key, Value def_val)
{
    assert(get_value_type(node).is(*type::PersistentHashMapCollisionNode));
    for (decltype(size) i = 0; i < size; i += 2)
        if (get_dynamic_object_element(node, i) == key)
            return get_dynamic_object_element(node, i + 1);
    return def_val;
}

Value collision_node_get(Value node, Value key, Value def_val)
{
    return collision_node_get(node, get_dynamic_object_size(node), key, def_val);
}

Value collision_node_get(Value node, Value key, std::uint32_t key_hash, Value def_val)
{
    assert(get_value_type(node).is(*type::PersistentHashMapCollisionNode));
    if (key_hash != get_dynamic_object_int(node, 0))
        return def_val;
    return collision_node_get(node, key, def_val);
}

Force create_array_node(std::uint8_t shift, Value key0, std::uint32_t key0_hash, Value val0, std::uint32_t node_hash, Value node);

Force collision_node_assoc(Value node, std::uint8_t shift, Value key, std::uint32_t key_hash, Value val, bool& replaced)
{
    assert(get_value_type(node).is(*type::PersistentHashMapCollisionNode));
    auto node_hash = get_dynamic_object_int(node, 0);
    if (key_hash != node_hash)
        return create_array_node(shift, key, key_hash, val, node_hash, node);
    auto node_size = get_dynamic_object_size(node);
    bool should_replace = !collision_node_get(node, key, *SENTINEL).is(*SENTINEL);
    if (should_replace)
    {
        Root new_node{create_object(*type::PersistentHashMapCollisionNode, &node_hash, 1, nullptr, node_size)};
        for (decltype(node_size) i = 0; i < node_size; i += 2)
        {
            auto ek = get_dynamic_object_element(node, i);
            set_dynamic_object_element(*new_node, i, ek);
            if (ek != key)
                set_dynamic_object_element(*new_node, i + 1, get_dynamic_object_element(node, i + 1));
            else
                set_dynamic_object_element(*new_node, i + 1, val);
        }
        replaced = true;
        return *new_node;
    }
    else
    {
        Root new_node{create_object(*type::PersistentHashMapCollisionNode, &node_hash, 1, nullptr, node_size + 2)};
        copy_object_elements(*new_node, 0, node, 0, node_size);
        set_dynamic_object_element(*new_node, node_size, key);
        set_dynamic_object_element(*new_node, node_size + 1, val);
        replaced = false;
        return *new_node;
    }
}

std::pair<Force, Value> collision_node_dissoc(Value node, Value key, std::uint32_t key_hash)
{
    assert(get_value_type(node).is(*type::PersistentHashMapCollisionNode));
    auto node_hash = get_dynamic_object_int(node, 0);
    if (node_hash != key_hash)
        return {node, *SENTINEL};
    auto node_size = get_dynamic_object_size(node);
    decltype(node_size) index = 0;
    while (index < node_size && get_dynamic_object_element(node, index) != key)
        index += 2;
    if (index >= node_size)
        return {node, *SENTINEL};
    if (node_size == 4) // two KVs
        return {get_dynamic_object_element(node, 3 - index), get_dynamic_object_element(node, 2 - index)};
    Root new_node{create_object(*type::PersistentHashMapCollisionNode, &node_hash, 1, nullptr, node_size - 2)};
    copy_object_elements(*new_node, 0, node, 0, index);
    copy_object_elements(*new_node, index, node, index + 2, node_size);
    return {*new_node, *SENTINEL};
}

Value collision_node_equal(Value left, Value right)
{
    assert(get_value_type(left).is(*type::PersistentHashMapCollisionNode));
    assert(get_value_type(right).is(*type::PersistentHashMapCollisionNode));
    auto size = get_dynamic_object_size(left);
    if (size != get_dynamic_object_size(right))
        return nil;
    if (get_dynamic_object_int(left, 0) != get_dynamic_object_int(right, 0))
        return nil;
    for (decltype(size) i = 0; i < size; i += 2)
        if (collision_node_get(right, size, get_dynamic_object_element(left, i), *SENTINEL) != get_dynamic_object_element(left, i + 1))
            return nil;
    return TRUE;
}

std::uint32_t map_bit(std::uint8_t shift, std::uint32_t hash)
{
    return 1 << ((hash >> shift) & 0x1f);
}

std::uint32_t map_key_index(std::uint32_t value_map, std::uint32_t bit)
{
    return 2 * popcount(value_map & (bit - 1));
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

Force create_array_node(std::uint8_t shift, Value key0, std::uint32_t key0_hash, Value val0, Value key1, std::uint32_t key1_hash, Value val1)
{
    assert(key0_hash != key1_hash);
    auto key0_bit = map_bit(shift, key0_hash);
    auto key1_bit = map_bit(shift, key1_hash);
    if (key0_bit == key1_bit)
    {
        std::uint32_t node_map = key0_bit;
        auto map_val = combine_maps(0, node_map);
        Root child_node{create_array_node(shift + 5, key0, key0_hash, val0, key1, key1_hash, val1)};
        return create_object1_1(*type::PersistentHashMapArrayNode, map_val, *child_node);
    }
    else
    {
        std::uint32_t value_map = key0_bit | key1_bit;
        auto map_val = combine_maps(value_map, 0);
        if (key0_bit < key1_bit)
            return create_object1_4(*type::PersistentHashMapArrayNode, map_val, key0, val0, key1, val1);
        else
            return create_object1_4(*type::PersistentHashMapArrayNode, map_val, key1, val1, key0, val0);
    }
}

Force create_array_node(std::uint8_t shift, Value key0, std::uint32_t key0_hash, Value val0, std::uint32_t node_hash, Value node)
{
    assert(key0_hash != node_hash);
    auto key0_bit = map_bit(shift, key0_hash);
    auto node_bit = map_bit(shift, node_hash);
    if (key0_bit == node_bit)
    {
        std::uint32_t node_map = key0_bit;
        auto map_val = combine_maps(0, node_map);
        Root child_node{create_array_node(shift + 5, key0, key0_hash, val0, node_hash, node)};
        return create_object1_1(*type::PersistentHashMapArrayNode, map_val, *child_node);
    }
    else
    {
        auto map_val = combine_maps(key0_bit, node_bit);
        return create_object1_3(*type::PersistentHashMapArrayNode, map_val, key0, val0, node);
    }
}

Force create_array_node(Int64 map, std::uint32_t node_size)
{
    return create_object(*type::PersistentHashMapArrayNode, &map, 1, nullptr, node_size);
}

Force create_array_map(Value key0, std::uint32_t key0_hash, Value val0, Value key1, std::uint32_t key1_hash, Value val1)
{
    Root node{create_array_node(0, key0, key0_hash, val0, key1, key1_hash, val1)};
    return create_map(2, *node);
}

Value array_node_get(Value node, std::uint8_t shift, Value key, std::uint32_t key_hash, Value def_val)
{
    assert(get_value_type(node).is(*type::PersistentHashMapArrayNode));
    std::uint64_t value_node_map = get_dynamic_object_int(node, 0);
    std::uint32_t value_map{static_cast<std::uint32_t>(value_node_map)};
    std::uint32_t node_map{static_cast<std::uint32_t>(value_node_map >> 32)};
    assert((value_map & node_map) == 0);
    std::uint32_t key_bit = map_bit(shift, key_hash);
    if (value_map & key_bit)
    {
        auto key_index = map_key_index(value_map, key_bit);
        if (get_dynamic_object_element(node, key_index) != key)
            return def_val;
        return get_dynamic_object_element(node, key_index + 1);
    }
    if (node_map & key_bit)
    {
        auto node_index = map_node_index(node_map, get_dynamic_object_size(node), key_bit);
        auto child_node = get_dynamic_object_element(node, node_index);
        if (get_value_type(child_node).is(*type::PersistentHashMapCollisionNode))
            return collision_node_get(child_node, key, key_hash, def_val);
        return array_node_get(child_node, shift + 5, key, key_hash, def_val);
    }
    return def_val;
}

Value array_node_assoc(Value node, std::uint8_t shift, Value key, std::uint32_t key_hash, Value val, bool& replaced)
{
    assert(get_value_type(node).is(*type::PersistentHashMapArrayNode));
    std::uint64_t value_node_map = get_dynamic_object_int(node, 0);
    std::uint32_t value_map{static_cast<std::uint32_t>(value_node_map)};
    std::uint32_t node_map{static_cast<std::uint32_t>(value_node_map >> 32)};
    std::uint32_t key_bit = map_bit(shift, key_hash);
    auto key_index = map_key_index(value_map, key_bit);
    auto node_size = get_dynamic_object_size(node);
    if (value_map & key_bit)
    {
        auto key0 = get_dynamic_object_element(node, key_index);
        if (key0 == key)
        {
            Root new_node{create_array_node(value_node_map, node_size)};

            copy_object_elements(*new_node, 0, node, 0, key_index);
            set_dynamic_object_element(*new_node, key_index, key);
            set_dynamic_object_element(*new_node, key_index + 1, val);
            copy_object_elements(*new_node, key_index + 2, node, key_index + 2, node_size);

            replaced = true;
            return *new_node;
        }
        else
        {
            auto new_value_map = combine_maps(value_map ^ key_bit, node_map ^ key_bit);
            Root new_node{create_array_node(new_value_map, node_size - 1)};
            auto node_index = map_node_index(node_map, node_size, key_bit);
            auto val0 = get_dynamic_object_element(node, key_index + 1);
            std::uint32_t key0_hash = hash_value(key0);
            Root new_child{
                (key0_hash == key_hash) ?
                create_collision_node(key_hash, key0, val0, key, val) :
                create_array_node(shift + 5, key0, key0_hash, val0, key, key_hash, val)};

            copy_object_elements(*new_node, 0, node, 0, key_index);
            copy_object_elements(*new_node, key_index, node, key_index + 2, node_index + 1);
            set_dynamic_object_element(*new_node, node_index - 1, *new_child);
            copy_object_elements(*new_node, node_index, node, node_index + 1, node_size);

            replaced = false;
            return *new_node;
        }
    }
    else if (node_map & key_bit)
    {
        Root new_node{create_array_node(value_node_map, node_size)};
        auto node_index = map_node_index(node_map, node_size, key_bit);
        auto child_node = get_dynamic_object_element(node, node_index);
        Root new_child{
            get_value_type(child_node).is(*type::PersistentHashMapCollisionNode) ?
            collision_node_assoc(child_node, shift + 5, key, key_hash, val, replaced) :
            array_node_assoc(child_node, shift + 5, key, key_hash, val, replaced)};
        copy_object_elements(*new_node, 0, node, 0, node_index);
        set_dynamic_object_element(*new_node, node_index, *new_child);
        copy_object_elements(*new_node, node_index + 1, node, node_index + 1, node_size);
        return *new_node;
    }
    else
    {
        Int64 new_value_map = combine_maps(value_map | key_bit, node_map);
        Root new_node{create_array_node(new_value_map, node_size + 2)};
        copy_object_elements(*new_node, 0, node, 0, key_index);
        set_dynamic_object_element(*new_node, key_index, key);
        set_dynamic_object_element(*new_node, key_index + 1, val);
        copy_object_elements(*new_node, key_index + 2, node, key_index, node_size);

        replaced = false;
        return *new_node;
    }
}

std::pair<Force, Value> array_node_dissoc(Value node, std::uint8_t shift, Value key, std::uint32_t key_hash)
{
    std::uint64_t value_node_map = get_dynamic_object_int(node, 0);
    std::uint32_t value_map{static_cast<std::uint32_t>(value_node_map)};
    std::uint32_t node_map{static_cast<std::uint32_t>(value_node_map >> 32)};
    std::uint32_t key_bit = map_bit(shift, key_hash);
    if (value_map & key_bit)
    {
        auto key_index = map_key_index(value_map, key_bit);
        auto key0 = get_dynamic_object_element(node, key_index);
        if (key0 != key)
            return {node, *SENTINEL};
        auto node_size = get_dynamic_object_size(node);
        auto value_count = popcount(value_map);
        if (value_count == 2 && node_map == 0)
            return {get_dynamic_object_element(node, 3 - key_index), get_dynamic_object_element(node, 2 - key_index)};
        if (value_count == 1 && popcount(node_map) == 1)
        {
            auto other_child_node = get_dynamic_object_element(node, node_size - 1);
            if (get_value_type(other_child_node).is(*type::PersistentHashMapCollisionNode))
                return {other_child_node, *SENTINEL};
        }
        auto new_value_node_map = combine_maps(value_map ^ key_bit, node_map);
        Root new_node{create_array_node(new_value_node_map, node_size - 2)};
        copy_object_elements(*new_node, 0, node, 0, key_index);
        copy_object_elements(*new_node, key_index, node, key_index + 2, node_size);
        return {*new_node, *SENTINEL};
    }
    else if (node_map & key_bit)
    {
        auto node_size = get_dynamic_object_size(node);
        auto node_index = map_node_index(node_map, node_size, key_bit);
        auto child_node = get_dynamic_object_element(node, node_index);
        std::pair<Root, Value> new_child{
            get_value_type(child_node).is(*type::PersistentHashMapCollisionNode) ?
            collision_node_dissoc(child_node, key, key_hash) :
            array_node_dissoc(child_node, shift + 5, key, key_hash)};
        if (*new_child.first == child_node)
            return {node, *SENTINEL};
        if (!new_child.second.is(*SENTINEL))
        {
            if (value_map == 0 && node_map == key_bit)
                return {*new_child.first, new_child.second};
            auto new_value_node_map = combine_maps(value_map ^ key_bit, node_map ^ key_bit);
            Root new_node{create_array_node(new_value_node_map, node_size + 1)};
            auto key_index = map_key_index(value_map, key_bit);
            copy_object_elements(*new_node, 0, node, 0, key_index);
            set_dynamic_object_element(*new_node, key_index, new_child.second);
            set_dynamic_object_element(*new_node, key_index + 1, *new_child.first);
            copy_object_elements(*new_node, key_index + 2, node, key_index, node_index);
            copy_object_elements(*new_node, node_index + 2, node, node_index + 1, node_size);
            return {*new_node, *SENTINEL};
        }
        Root new_node{create_array_node(value_node_map, node_size)};
        copy_object_elements(*new_node, 0, node, 0, node_index);
        set_dynamic_object_element(*new_node, node_index, *new_child.first);
        copy_object_elements(*new_node, node_index + 1, node, node_index + 1, node_size);
        return {*new_node, *SENTINEL};
    }
    else
        return {node, *SENTINEL};
}

Value array_node_equal(Value left, Value right)
{
    assert(get_value_type(left).is(*type::PersistentHashMapArrayNode));
    assert(get_value_type(right).is(*type::PersistentHashMapArrayNode));

    std::uint64_t value_node_map = get_dynamic_object_int(left, 0);
    if (std::uint64_t(get_dynamic_object_int(right, 0)) != value_node_map)
        return nil;
    std::uint32_t value_map{static_cast<std::uint32_t>(value_node_map)};
    auto value_count = popcount(value_map);
    for (std::uint32_t i = 0; i < std::uint32_t(value_count * 2); ++i)
        if (get_dynamic_object_element(left, i) != get_dynamic_object_element(right, i))
            return nil;
    auto node_size = get_dynamic_object_size(left);
    for (std::uint32_t i = value_count * 2; i < node_size; ++i)
    {
        auto left_child = get_dynamic_object_element(left, i);
        auto right_child = get_dynamic_object_element(right, i);
        auto left_child_type = get_object_type(left_child);
        if (!left_child_type.is(get_object_type(right_child)))
            return nil;
        if (left_child_type.is(*type::PersistentHashMapCollisionNode) && !collision_node_equal(get_dynamic_object_element(left, i), get_dynamic_object_element(right, i)))
            return nil;
        if (left_child_type.is(*type::PersistentHashMapArrayNode) && !array_node_equal(get_dynamic_object_element(left, i), get_dynamic_object_element(right, i)))
            return nil;
    }
    return TRUE;
}

Force collision_node_seq(Value node, Value parent)
{
    std::array<Value, 2> kv{{get_dynamic_object_element(node, 0), get_dynamic_object_element(node, 1)}};
    Root entry{create_array(kv.data(), kv.size())};
    return create_static_object(*type::PersistentHashMapSeq, *entry, node, 2, parent);
}

Force array_node_seq(Value node, Value parent)
{
    std::uint64_t value_node_map = get_dynamic_object_int(node, 0);
    std::uint32_t value_map{static_cast<std::uint32_t>(value_node_map)};
    if (value_map != 0)
    {
        std::array<Value, 2> kv{{get_dynamic_object_element(node, 0), get_dynamic_object_element(node, 1)}};
        Root entry{create_array(kv.data(), kv.size())};
        return create_static_object(*type::PersistentHashMapSeq, *entry, node, 2, parent);
    }
    Root child_parent{create_static_object(*type::PersistentHashMapSeqParent, 1, node, parent)};
    auto child = get_dynamic_object_element(node, 0);
    if (get_value_type(child).is(*type::PersistentHashMapCollisionNode))
        return collision_node_seq(child, *child_parent);
    return array_node_seq(child, *child_parent);
}

Force collision_node_next(Value node, Value child, Value parent, Int64 index)
{
    std::array<Value, 2> kv{{get_dynamic_object_element(child, 0), get_dynamic_object_element(child, 1)}};
    Root entry{create_array(kv.data(), kv.size())};
    Root child_parent{create_static_object(*type::PersistentHashMapSeqParent, index + 1, node, parent)};
    return create_static_object(*type::PersistentHashMapSeq, *entry, child, 2, *child_parent);
}

Force array_node_next(Value node, Value child, Value parent, Int64 index)
{
    Root child_parent{create_static_object(*type::PersistentHashMapSeqParent, index + 1, node, parent)};

    std::uint64_t value_node_map = get_dynamic_object_int(child, 0);
    std::uint32_t value_map{static_cast<std::uint32_t>(value_node_map)};
    if (value_map != 0)
    {
        std::array<Value, 2> kv{{get_dynamic_object_element(child, 0), get_dynamic_object_element(child, 1)}};
        Root entry{create_array(kv.data(), kv.size())};
        return create_static_object(*type::PersistentHashMapSeq, *entry, child, 2, *child_parent);
    }

    return array_node_seq(child, *child_parent);
}

}

Force create_persistent_hash_map()
{
    return create_map(0, *SENTINEL);
}

Int64 get_persistent_hash_map_size(Value m)
{
    return get_dynamic_object_int(m, 0);
}

Value persistent_hash_map_get(Value m, Value k)
{
    return persistent_hash_map_get(m, k, nil);
}

Value persistent_hash_map_get(Value map, Value key, Value def_val)
{
    auto node_or_val = get_dynamic_object_element(map, 0);
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
        return array_node_get(node_or_val, 0, key, key_hash, def_val);
    }
    return get_dynamic_object_element(map, 1) == key ? node_or_val : def_val;
}

Force persistent_hash_map_assoc(Value map, Value key, Value val)
{
    auto node_or_val = get_dynamic_object_element(map, 0);
    if (node_or_val.is(*SENTINEL))
        return create_single_value_map(key, val);

    auto node_or_val_type = get_value_type(node_or_val);
    if (node_or_val_type.is(*type::PersistentHashMapCollisionNode))
    {
        std::uint32_t key_hash = hash_value(key);
        auto replaced = false;
        Root new_node{collision_node_assoc(node_or_val, 0, key, key_hash, val, replaced)};
        auto size = get_persistent_hash_map_size(map);
        return create_map(replaced ? size : (size + 1), *new_node);
    }
    if (node_or_val_type.is(*type::PersistentHashMapArrayNode))
    {
        std::uint32_t key_hash = hash_value(key);
        auto replaced = false;
        Root new_node{array_node_assoc(node_or_val, 0, key, key_hash, val, replaced)};
        auto size = get_persistent_hash_map_size(map);
        return create_map(replaced ? size : (size + 1), *new_node);
    }

    Value key0 = get_dynamic_object_element(map, 1);
    if (key0 == key)
        return create_single_value_map(key, val);
    auto val0 = get_dynamic_object_element(map, 0);
    std::uint32_t key_hash = hash_value(key);
    std::uint32_t key0_hash = hash_value(key0);
    if (key_hash == key0_hash)
        return create_collision_map(key_hash, key0, val0, key, val);
    return create_array_map(key0, key0_hash, val0, key, key_hash, val);
}

Force persistent_hash_map_dissoc(Value map, Value key)
{
    auto node_or_val = get_dynamic_object_element(map, 0);
    if (node_or_val.is(*SENTINEL))
        return map;
    auto node_or_val_type = get_value_type(node_or_val);
    if (node_or_val_type.is(*type::PersistentHashMapCollisionNode))
    {
        std::uint32_t key_hash = hash_value(key);
        std::pair<Root, Value> new_node{collision_node_dissoc(node_or_val, key, key_hash)};
        if (new_node.first->is(node_or_val))
            return map;
        if (!new_node.second.is(*SENTINEL))
            return create_single_value_map(new_node.second, *new_node.first);
        auto size = get_persistent_hash_map_size(map);
        return create_map(size - 1, *new_node.first);
    }
    if (node_or_val_type.is(*type::PersistentHashMapArrayNode))
    {
        std::uint32_t key_hash = hash_value(key);
        std::pair<Root, Value> new_node{array_node_dissoc(node_or_val, 0, key, key_hash)};
        if (new_node.first->is(node_or_val))
            return map;
        if (!new_node.second.is(*SENTINEL))
            return create_single_value_map(new_node.second, *new_node.first);
        auto size = get_persistent_hash_map_size(map);
        return create_map(size - 1, *new_node.first);
    }
    Value key0 = get_dynamic_object_element(map, 1);
    if (key0 == key)
        return *EMPTY_HASH_MAP;
    return map;
}

Value persistent_hash_map_contains(Value m, Value k)
{
    auto val = persistent_hash_map_get(m, k, *SENTINEL);
    return val.is(*SENTINEL) ? nil : TRUE;
}

Value are_persistent_hash_maps_equal(Value left, Value right)
{
    if (get_persistent_hash_map_size(left) != get_persistent_hash_map_size(right))
        return nil;
    auto left_node_or_val = get_dynamic_object_element(left, 0);
    auto right_node_or_val = get_dynamic_object_element(right, 0);
    if (left_node_or_val.is(*SENTINEL) || right_node_or_val.is(*SENTINEL))
        return left_node_or_val.is(right_node_or_val) ? TRUE : nil;
    auto left_type = get_value_type(left_node_or_val);
    auto right_type = get_value_type(right_node_or_val);
    if (left_type.is(*type::PersistentHashMapCollisionNode) || right_type.is(*type::PersistentHashMapCollisionNode))
    {
        if (!left_type.is(right_type))
            return nil;
        return collision_node_equal(left_node_or_val, right_node_or_val);
    }
    if (left_type.is(*type::PersistentHashMapArrayNode) || right_type.is(*type::PersistentHashMapArrayNode))
    {
        if (!left_type.is(right_type))
            return nil;
        return array_node_equal(left_node_or_val, right_node_or_val);
    }
    auto left_key = get_dynamic_object_element(left, 1);
    auto right_key = get_dynamic_object_element(right, 1);
    return (left_node_or_val == right_node_or_val && left_key == right_key) ? TRUE : nil;
}

Force persistent_hash_map_seq(Value m)
{
    auto node_or_val = get_dynamic_object_element(m, 0);
    if (node_or_val.is(*SENTINEL))
        return nil;
    auto node_type = get_value_type(node_or_val);
    if (node_type.is(*type::PersistentHashMapCollisionNode))
        return collision_node_seq(node_or_val, nil);
    if (node_type.is(*type::PersistentHashMapArrayNode))
        return array_node_seq(node_or_val, nil);
    std::array<Value, 2> kv{{get_dynamic_object_element(m, 1), node_or_val}};
    Root entry{create_array(kv.data(), kv.size())};
    return create_static_object(*type::PersistentHashMapSeq, *entry, nil, 0, nil);
}

Value get_persistent_hash_map_seq_first(Value s)
{
    return get_static_object_element(s, 0);
}

Force get_persistent_hash_map_seq_next(Value s)
{
    auto node = get_static_object_element(s, 1);
    if (node.is_nil())
        return nil;
    auto index = get_static_object_int(s, 2);
    auto parent = get_static_object_element(s, 3);
    while (index == get_dynamic_object_size(node))
    {
        if (!parent)
            return nil;
        index = get_static_object_int(parent, 0);
        node = get_static_object_element(parent, 1);
        parent = get_static_object_element(parent, 2);
    }
    auto child = get_dynamic_object_element(node, index);
    if (get_value_type(child).is(*type::PersistentHashMapCollisionNode))
        return collision_node_next(node, child, parent, index);
    if (get_value_type(child).is(*type::PersistentHashMapArrayNode))
        return array_node_next(node, child, parent, index);
    std::array<Value, 2> kv{{child, get_dynamic_object_element(node, index + 1)}};
    Root entry{create_array(kv.data(), kv.size())};
    return create_static_object(*type::PersistentHashMapSeq, *entry, node, index + 2, parent);
}

}
