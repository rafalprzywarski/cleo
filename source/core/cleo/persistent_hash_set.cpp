#include "persistent_hash_set.hpp"
#include "global.hpp"
#include "array.hpp"

namespace cleo
{

// HashSet:
//   size 0: [0 SENTINEL]
//   size 1: [1 key]
//   size>1: [size collision-node]
//   size>1: [size array-node]
// CollisionNode:
//   [hash | key0 value0 key1 value1 key2? value2? ...]
// ArrayNode:
//   [(value-map node-map) | key0? value0? key1? value1? ... node2? node1? node0?]
// HashSetSeq:
//   size 1: [first-key nil 0 nil]
//   size>1: [first-key collision-node 0 #SeqParent[index array-node parent-or-nil]-or-nil]
//   size>1: [first-key array-node 0 #SeqParent[index array-node parent-or-nil]-or-nil]

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

Force create_collision_node(std::uint32_t hash, Value k0, Value k1)
{
    assert(static_cast<std::uint32_t>(hash_value(k0)) == static_cast<std::uint32_t>(hash_value(k1)));
    return create_object1_4(*type::PersistentHashSetCollisionNode, hash, k0, nil, k1, nil);
}

Force create_set(Int64 size, Value elem0)
{
    return create_static_object(*type::PersistentHashSet, size, elem0);
}

Force create_single_value_set(Value k)
{
    return create_set(1, k);
}

Force create_collision_set(std::uint32_t hash, Value k0, Value k1)
{
    Root node{create_collision_node(hash, k0, k1)};
    return create_set(2, *node);
}

Value collision_node_get(Value node, std::uint32_t size, Value key, Value def_val)
{
    assert(get_value_type(node).is(*type::PersistentHashSetCollisionNode));
    for (decltype(size) i = 0; i < size; i += 2)
        if (get_dynamic_object_element(node, i) == key)
            return key;
    return def_val;
}

Value collision_node_get(Value node, Value key, Value def_val)
{
    return collision_node_get(node, get_dynamic_object_size(node), key, def_val);
}

Value collision_node_get(Value node, Value key, std::uint32_t key_hash, Value def_val)
{
    assert(get_value_type(node).is(*type::PersistentHashSetCollisionNode));
    if (key_hash != get_dynamic_object_int(node, 0))
        return def_val;
    return collision_node_get(node, key, def_val);
}

Force create_array_node(std::uint8_t shift, Value key0, std::uint32_t key0_hash, std::uint32_t node_hash, Value node);

Force collision_node_conj(Value node, std::uint8_t shift, Value key, std::uint32_t key_hash, bool& added)
{
    assert(get_value_type(node).is(*type::PersistentHashSetCollisionNode));
    auto node_hash = get_dynamic_object_int(node, 0);
    if (key_hash != node_hash)
        return create_array_node(shift, key, key_hash, node_hash, node);
    auto node_size = get_dynamic_object_size(node);
    if (!collision_node_get(node, key, *SENTINEL).is(*SENTINEL))
    {
        added = false;
        return node;
    }

    Root new_node{create_object(*type::PersistentHashSetCollisionNode, &node_hash, 1, nullptr, node_size + 2)};
    copy_object_elements(*new_node, 0, node, 0, node_size);
    set_dynamic_object_element(*new_node, node_size, key);
    set_dynamic_object_element(*new_node, node_size + 1, nil);
    return *new_node;
}

std::pair<Force, Value> collision_node_disj(Value node, Value key, std::uint32_t key_hash)
{
    assert(get_value_type(node).is(*type::PersistentHashSetCollisionNode));
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
    Root new_node{create_object(*type::PersistentHashSetCollisionNode, &node_hash, 1, nullptr, node_size - 2)};
    copy_object_elements(*new_node, 0, node, 0, index);
    copy_object_elements(*new_node, index, node, index + 2, node_size);
    return {*new_node, *SENTINEL};
}

Value collision_node_equal(Value left, Value right)
{
    assert(get_value_type(left).is(*type::PersistentHashSetCollisionNode));
    assert(get_value_type(right).is(*type::PersistentHashSetCollisionNode));
    auto size = get_dynamic_object_size(left);
    if (size != get_dynamic_object_size(right))
        return nil;
    if (get_dynamic_object_int(left, 0) != get_dynamic_object_int(right, 0))
        return nil;
    for (decltype(size) i = 0; i < size; i += 2)
    {
        auto key = get_dynamic_object_element(left, i);
        if (collision_node_get(right, size, key, *SENTINEL).is(*SENTINEL))
            return nil;
    }
    return TRUE;
}

std::uint32_t map_bit(std::uint8_t shift, std::uint32_t hash)
{
    return 1 << ((hash >> shift) & 0x1f);
}

std::uint32_t map_key_index(std::uint32_t value_set, std::uint32_t bit)
{
    return 2 * popcount(value_set & (bit - 1));
}

std::uint32_t map_node_index(std::uint32_t node_set, std::uint32_t node_size, std::uint32_t bit)
{
    return node_size - popcount(node_set & (bit - 1)) - 1;
}

Int64 combine_maps(std::uint32_t value_set, std::uint32_t node_set)
{
    assert((value_set & node_set) == 0);
    return value_set | (std::uint64_t(node_set) << 32);
}

Force create_array_node(std::uint8_t shift, Value key0, std::uint32_t key0_hash, Value key1, std::uint32_t key1_hash)
{
    assert(key0_hash != key1_hash);
    auto key0_bit = map_bit(shift, key0_hash);
    auto key1_bit = map_bit(shift, key1_hash);
    if (key0_bit == key1_bit)
    {
        std::uint32_t node_set = key0_bit;
        auto map_val = combine_maps(0, node_set);
        Root child_node{create_array_node(shift + 5, key0, key0_hash, key1, key1_hash)};
        return create_object1_1(*type::PersistentHashSetArrayNode, map_val, *child_node);
    }
    else
    {
        std::uint32_t value_set = key0_bit | key1_bit;
        auto map_val = combine_maps(value_set, 0);
        if (key0_bit < key1_bit)
            return create_object1_4(*type::PersistentHashSetArrayNode, map_val, key0, nil, key1, nil);
        else
            return create_object1_4(*type::PersistentHashSetArrayNode, map_val, key1, nil, key0, nil);
    }
}

Force create_array_node(std::uint8_t shift, Value key0, std::uint32_t key0_hash, std::uint32_t node_hash, Value node)
{
    assert(key0_hash != node_hash);
    auto key0_bit = map_bit(shift, key0_hash);
    auto node_bit = map_bit(shift, node_hash);
    if (key0_bit == node_bit)
    {
        std::uint32_t node_set = key0_bit;
        auto map_val = combine_maps(0, node_set);
        Root child_node{create_array_node(shift + 5, key0, key0_hash, node_hash, node)};
        return create_object1_1(*type::PersistentHashSetArrayNode, map_val, *child_node);
    }
    else
    {
        auto map_val = combine_maps(key0_bit, node_bit);
        return create_object1_3(*type::PersistentHashSetArrayNode, map_val, key0, nil, node);
    }
}

Force create_array_node(Int64 map, std::uint32_t node_size)
{
    return create_object(*type::PersistentHashSetArrayNode, &map, 1, nullptr, node_size);
}

Force create_array_set(Value key0, std::uint32_t key0_hash, Value key1, std::uint32_t key1_hash)
{
    Root node{create_array_node(0, key0, key0_hash, key1, key1_hash)};
    return create_set(2, *node);
}

Value array_node_get(Value node, std::uint8_t shift, Value key, std::uint32_t key_hash, Value def_val)
{
    assert(get_value_type(node).is(*type::PersistentHashSetArrayNode));
    std::uint64_t value_node_set = get_dynamic_object_int(node, 0);
    std::uint32_t value_set{static_cast<std::uint32_t>(value_node_set)};
    std::uint32_t node_set{static_cast<std::uint32_t>(value_node_set >> 32)};
    assert((value_set & node_set) == 0);
    std::uint32_t key_bit = map_bit(shift, key_hash);
    if (value_set & key_bit)
    {
        auto key_index = map_key_index(value_set, key_bit);
        if (get_dynamic_object_element(node, key_index) != key)
            return def_val;
        return key;
    }
    if (node_set & key_bit)
    {
        auto node_index = map_node_index(node_set, get_dynamic_object_size(node), key_bit);
        auto child_node = get_dynamic_object_element(node, node_index);
        if (get_value_type(child_node).is(*type::PersistentHashSetCollisionNode))
            return collision_node_get(child_node, key, key_hash, def_val);
        return array_node_get(child_node, shift + 5, key, key_hash, def_val);
    }
    return def_val;
}

Value array_node_conj(Value node, std::uint8_t shift, Value key, std::uint32_t key_hash, bool& added)
{
    assert(get_value_type(node).is(*type::PersistentHashSetArrayNode));
    std::uint64_t value_node_set = get_dynamic_object_int(node, 0);
    std::uint32_t value_set{static_cast<std::uint32_t>(value_node_set)};
    std::uint32_t node_set{static_cast<std::uint32_t>(value_node_set >> 32)};
    std::uint32_t key_bit = map_bit(shift, key_hash);
    auto key_index = map_key_index(value_set, key_bit);
    auto node_size = get_dynamic_object_size(node);
    if (value_set & key_bit)
    {
        auto key0 = get_dynamic_object_element(node, key_index);
        if (key0 == key)
        {
            added = false;
            return node;
        }
        auto new_value_set = combine_maps(value_set ^ key_bit, node_set ^ key_bit);
        Root new_node{create_array_node(new_value_set, node_size - 1)};
        auto node_index = map_node_index(node_set, node_size, key_bit);
        std::uint32_t key0_hash = hash_value(key0);
        Root new_child{
            (key0_hash == key_hash) ?
            create_collision_node(key_hash, key0, key) :
            create_array_node(shift + 5, key0, key0_hash, key, key_hash)};

        copy_object_elements(*new_node, 0, node, 0, key_index);
        copy_object_elements(*new_node, key_index, node, key_index + 2, node_index + 1);
        set_dynamic_object_element(*new_node, node_index - 1, *new_child);
        copy_object_elements(*new_node, node_index, node, node_index + 1, node_size);

        return *new_node;
    }
    else if (node_set & key_bit)
    {
        Root new_node{create_array_node(value_node_set, node_size)};
        auto node_index = map_node_index(node_set, node_size, key_bit);
        auto child_node = get_dynamic_object_element(node, node_index);
        Root new_child{
            get_value_type(child_node).is(*type::PersistentHashSetCollisionNode) ?
            collision_node_conj(child_node, shift + 5, key, key_hash, added) :
            array_node_conj(child_node, shift + 5, key, key_hash, added)};
        copy_object_elements(*new_node, 0, node, 0, node_index);
        set_dynamic_object_element(*new_node, node_index, *new_child);
        copy_object_elements(*new_node, node_index + 1, node, node_index + 1, node_size);
        return *new_node;
    }
    else
    {
        Int64 new_value_set = combine_maps(value_set | key_bit, node_set);
        Root new_node{create_array_node(new_value_set, node_size + 2)};
        copy_object_elements(*new_node, 0, node, 0, key_index);
        set_dynamic_object_element(*new_node, key_index, key);
        set_dynamic_object_element(*new_node, key_index + 1, key);
        copy_object_elements(*new_node, key_index + 2, node, key_index, node_size);

        return *new_node;
    }
}

std::pair<Force, Value> array_node_disj(Value node, std::uint8_t shift, Value key, std::uint32_t key_hash)
{
    std::uint64_t value_node_set = get_dynamic_object_int(node, 0);
    std::uint32_t value_set{static_cast<std::uint32_t>(value_node_set)};
    std::uint32_t node_set{static_cast<std::uint32_t>(value_node_set >> 32)};
    std::uint32_t key_bit = map_bit(shift, key_hash);
    if (value_set & key_bit)
    {
        auto key_index = map_key_index(value_set, key_bit);
        auto key0 = get_dynamic_object_element(node, key_index);
        if (key0 != key)
            return {node, *SENTINEL};
        auto node_size = get_dynamic_object_size(node);
        auto value_count = popcount(value_set);
        if (value_count == 2 && node_set == 0)
            return {get_dynamic_object_element(node, 3 - key_index), get_dynamic_object_element(node, 2 - key_index)};
        if (value_count == 1 && popcount(node_set) == 1)
        {
            auto other_child_node = get_dynamic_object_element(node, node_size - 1);
            if (get_value_type(other_child_node).is(*type::PersistentHashSetCollisionNode))
                return {other_child_node, *SENTINEL};
        }
        auto new_value_node_set = combine_maps(value_set ^ key_bit, node_set);
        Root new_node{create_array_node(new_value_node_set, node_size - 2)};
        copy_object_elements(*new_node, 0, node, 0, key_index);
        copy_object_elements(*new_node, key_index, node, key_index + 2, node_size);
        return {*new_node, *SENTINEL};
    }
    else if (node_set & key_bit)
    {
        auto node_size = get_dynamic_object_size(node);
        auto node_index = map_node_index(node_set, node_size, key_bit);
        auto child_node = get_dynamic_object_element(node, node_index);
        std::pair<Root, Value> new_child{
            get_value_type(child_node).is(*type::PersistentHashSetCollisionNode) ?
            collision_node_disj(child_node, key, key_hash) :
            array_node_disj(child_node, shift + 5, key, key_hash)};
        if (*new_child.first == child_node)
            return {node, *SENTINEL};
        if (!new_child.second.is(*SENTINEL))
        {
            if (value_set == 0 && node_set == key_bit)
                return {*new_child.first, new_child.second};
            auto new_value_node_set = combine_maps(value_set ^ key_bit, node_set ^ key_bit);
            Root new_node{create_array_node(new_value_node_set, node_size + 1)};
            auto key_index = map_key_index(value_set, key_bit);
            copy_object_elements(*new_node, 0, node, 0, key_index);
            set_dynamic_object_element(*new_node, key_index, new_child.second);
            set_dynamic_object_element(*new_node, key_index + 1, *new_child.first);
            copy_object_elements(*new_node, key_index + 2, node, key_index, node_index);
            copy_object_elements(*new_node, node_index + 2, node, node_index + 1, node_size);
            return {*new_node, *SENTINEL};
        }
        Root new_node{create_array_node(value_node_set, node_size)};
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
    assert(get_value_type(left).is(*type::PersistentHashSetArrayNode));
    assert(get_value_type(right).is(*type::PersistentHashSetArrayNode));

    std::uint64_t value_node_set = get_dynamic_object_int(left, 0);
    if (std::uint64_t(get_dynamic_object_int(right, 0)) != value_node_set)
        return nil;
    std::uint32_t value_set{static_cast<std::uint32_t>(value_node_set)};
    auto value_count = popcount(value_set);
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
        if (left_child_type.is(*type::PersistentHashSetCollisionNode) && !collision_node_equal(get_dynamic_object_element(left, i), get_dynamic_object_element(right, i)))
            return nil;
        if (left_child_type.is(*type::PersistentHashSetArrayNode) && !array_node_equal(get_dynamic_object_element(left, i), get_dynamic_object_element(right, i)))
            return nil;
    }
    return TRUE;
}

Force collision_node_seq(Value node, Value parent)
{
    auto k = get_dynamic_object_element(node, 0);
    return create_static_object(*type::PersistentHashSetSeq, k, node, 2, parent);
}

Force array_node_seq(Value node, Value parent)
{
    std::uint64_t value_node_set = get_dynamic_object_int(node, 0);
    std::uint32_t value_set{static_cast<std::uint32_t>(value_node_set)};
    if (value_set != 0)
    {
        auto k = get_dynamic_object_element(node, 0);
        return create_static_object(*type::PersistentHashSetSeq, k, node, 2, parent);
    }
    Root child_parent{create_static_object(*type::PersistentHashSetSeqParent, 1, node, parent)};
    auto child = get_dynamic_object_element(node, 0);
    if (get_value_type(child).is(*type::PersistentHashSetCollisionNode))
        return collision_node_seq(child, *child_parent);
    return array_node_seq(child, *child_parent);
}

Force collision_node_next(Value node, Value child, Value parent, Int64 index)
{
    auto k = get_dynamic_object_element(child, 0);
    Root child_parent{create_static_object(*type::PersistentHashSetSeqParent, index + 1, node, parent)};
    return create_static_object(*type::PersistentHashSetSeq, k, child, 2, *child_parent);
}

Force array_node_next(Value node, Value child, Value parent, Int64 index)
{
    Root child_parent{create_static_object(*type::PersistentHashSetSeqParent, index + 1, node, parent)};

    std::uint64_t value_node_set = get_dynamic_object_int(child, 0);
    std::uint32_t value_set{static_cast<std::uint32_t>(value_node_set)};
    if (value_set != 0)
    {
        auto k = get_dynamic_object_element(child, 0);
        return create_static_object(*type::PersistentHashSetSeq, k, child, 2, *child_parent);
    }

    return array_node_seq(child, *child_parent);
}

}

Force create_persistent_hash_set()
{
    return create_set(0, *SENTINEL);
}

Int64 get_persistent_hash_set_size(Value set)
{
    return get_static_object_int(set, 0);
}

Value persistent_hash_set_get(Value set, Value k)
{
    return persistent_hash_set_get(set, k, nil);
}

Value persistent_hash_set_get(Value set, Value key, Value def_val)
{
    auto node_or_key = get_static_object_element(set, 1);
    if (node_or_key.is(*SENTINEL))
        return def_val;
    auto node_or_key_type = get_value_type(node_or_key);
    if (node_or_key_type.is(*type::PersistentHashSetCollisionNode))
    {
        std::uint32_t key_hash = hash_value(key);
        return collision_node_get(node_or_key, key, key_hash, def_val);
    }
    if (node_or_key_type.is(*type::PersistentHashSetArrayNode))
    {
        std::uint32_t key_hash = hash_value(key);
        return array_node_get(node_or_key, 0, key, key_hash, def_val);
    }
    return node_or_key == key ? key : def_val;
}

Force persistent_hash_set_conj(Value set, Value key)
{
    auto node_or_key = get_static_object_element(set, 1);
    if (node_or_key.is(*SENTINEL))
        return create_single_value_set(key);

    auto node_or_key_type = get_value_type(node_or_key);
    if (node_or_key_type.is(*type::PersistentHashSetCollisionNode))
    {
        std::uint32_t key_hash = hash_value(key);
        auto added = true;
        Root new_node{collision_node_conj(node_or_key, 0, key, key_hash, added)};
        auto size = get_persistent_hash_set_size(set);
        return create_set(added ? (size + 1) : size, *new_node);
    }
    if (node_or_key_type.is(*type::PersistentHashSetArrayNode))
    {
        std::uint32_t key_hash = hash_value(key);
        auto added = true;
        Root new_node{array_node_conj(node_or_key, 0, key, key_hash, added)};
        auto size = get_persistent_hash_set_size(set);
        return create_set(added ? (size + 1) : size, *new_node);
    }

    if (node_or_key == key)
        return create_single_value_set(key);
    std::uint32_t key_hash = hash_value(key);
    std::uint32_t key0_hash = hash_value(node_or_key);
    if (key_hash == key0_hash)
        return create_collision_set(key_hash, node_or_key, key);
    return create_array_set(node_or_key, key0_hash, key, key_hash);
}

Force persistent_hash_set_disj(Value set, Value key)
{
    auto node_or_key = get_static_object_element(set, 1);
    if (node_or_key.is(*SENTINEL))
        return set;
    auto node_or_key_type = get_value_type(node_or_key);
    if (node_or_key_type.is(*type::PersistentHashSetCollisionNode))
    {
        std::uint32_t key_hash = hash_value(key);
        std::pair<Root, Value> new_node{collision_node_disj(node_or_key, key, key_hash)};
        if (new_node.first->is(node_or_key))
            return set;
        if (!new_node.second.is(*SENTINEL))
            return create_single_value_set(new_node.second);
        auto size = get_persistent_hash_set_size(set);
        return create_set(size - 1, *new_node.first);
    }
    if (node_or_key_type.is(*type::PersistentHashSetArrayNode))
    {
        std::uint32_t key_hash = hash_value(key);
        std::pair<Root, Value> new_node{array_node_disj(node_or_key, 0, key, key_hash)};
        if (new_node.first->is(node_or_key))
            return set;
        if (!new_node.second.is(*SENTINEL))
            return create_single_value_set(new_node.second);
        auto size = get_persistent_hash_set_size(set);
        return create_set(size - 1, *new_node.first);
    }
    if (node_or_key == key)
        return *EMPTY_HASH_SET;
    return set;
}

Value persistent_hash_set_contains(Value m, Value k)
{
    auto val = persistent_hash_set_get(m, k, *SENTINEL);
    return val.is(*SENTINEL) ? nil : TRUE;
}

Value are_persistent_hash_sets_equal(Value left, Value right)
{
    if (get_persistent_hash_set_size(left) != get_persistent_hash_set_size(right))
        return nil;
    auto left_node_or_key = get_static_object_element(left, 1);
    auto right_node_or_key = get_static_object_element(right, 1);
    if (left_node_or_key.is(*SENTINEL) || right_node_or_key.is(*SENTINEL))
        return left_node_or_key.is(right_node_or_key) ? TRUE : nil;
    auto left_type = get_value_type(left_node_or_key);
    auto right_type = get_value_type(right_node_or_key);
    if (left_type.is(*type::PersistentHashSetCollisionNode) || right_type.is(*type::PersistentHashSetCollisionNode))
    {
        if (!left_type.is(right_type))
            return nil;
        return collision_node_equal(left_node_or_key, right_node_or_key);
    }
    if (left_type.is(*type::PersistentHashSetArrayNode) || right_type.is(*type::PersistentHashSetArrayNode))
    {
        if (!left_type.is(right_type))
            return nil;
        return array_node_equal(left_node_or_key, right_node_or_key);
    }
    return left_node_or_key == right_node_or_key ? TRUE : nil;
}

Force persistent_hash_set_seq(Value set)
{
    auto node_or_key = get_static_object_element(set, 1);
    if (node_or_key.is(*SENTINEL))
        return nil;
    auto node_type = get_value_type(node_or_key);
    if (node_type.is(*type::PersistentHashSetCollisionNode))
        return collision_node_seq(node_or_key, nil);
    if (node_type.is(*type::PersistentHashSetArrayNode))
        return array_node_seq(node_or_key, nil);
    return create_static_object(*type::PersistentHashSetSeq, node_or_key, nil, 0, nil);
}

Value get_persistent_hash_set_seq_first(Value s)
{
    return get_static_object_element(s, 0);
}

Force get_persistent_hash_set_seq_next(Value s)
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
    if (get_value_type(child).is(*type::PersistentHashSetCollisionNode))
        return collision_node_next(node, child, parent, index);
    if (get_value_type(child).is(*type::PersistentHashSetArrayNode))
        return array_node_next(node, child, parent, index);
    return create_static_object(*type::PersistentHashSetSeq, child, node, index + 2, parent);
}

}
