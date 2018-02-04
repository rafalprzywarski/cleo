#include "persistent_hash_map.hpp"
#include "global.hpp"

namespace cleo
{

// HashMap:
//   size 0: [0 nil]
//   size 1: [1 value key]
//   size>1: [size collision-node hash]
//   size>1: [size array-node]
// CollisionNode:
//   [key0 value0 key1 value1 key2? value2? ...]
// ArrayNode:
//   [valuemap nodemap key0 value0 ...  node0]

namespace
{

Force create_collision_node(Value k0, Value v0, Value k1, Value v1)
{
    return create_object4(*type::PersistentHashMapCollisionNode, k0, v0, k1, v1);
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

Force create_collision_map(Value k0, Value v0, Value k1, Value v1)
{
    Root node{create_collision_node(k0, v0, k1, v1)};
    return create_map(*TWO, *node);
}

Value collision_node_get(Value node, Value k, Value def_v)
{
    auto size = get_object_size(node);
    for (decltype(size) i = 0; i < size; i += 2)
        if (get_object_element(node, i) == k)
            return get_object_element(node, i + 1);
    return def_v;
}

Value collision_node_assoc(Value node, Value k, Value v, bool& replaced)
{
    auto node_size = get_object_size(node);
    bool should_replace = !collision_node_get(node, k, *SENTINEL).is(*SENTINEL);
    if (should_replace)
    {
        Root new_node{create_object(*type::PersistentHashMapCollisionNode, nullptr, node_size)};
        for (decltype(node_size) i = 0; i < node_size; i += 2)
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
        for (decltype(node_size) i = 0; i < node_size; i += 2)
        {
            set_object_element(*new_node, i, get_object_element(node, i));
            set_object_element(*new_node, i + 1, get_object_element(node, i + 1));
        }
        set_object_element(*new_node, node_size, k);
        set_object_element(*new_node, node_size + 1, v);
        replaced = false;
        return *new_node;
    }
}

}

Force create_persistent_hash_map()
{
    return create_map(*ZERO, nil);
}

Int64 get_persistent_hash_map_size(Value m)
{
    return get_int64_value(get_object_element(m, 0));
}

Value persistent_hash_map_get(Value m, Value k)
{
    return persistent_hash_map_get(m, k, nil);
}

Value persistent_hash_map_get(Value m, Value k, Value def_v)
{
    auto node_or_val = get_object_element(m, 1);
    if (get_value_type(node_or_val) == *type::PersistentHashMapCollisionNode)
        return collision_node_get(node_or_val, k, def_v);
    return get_persistent_hash_map_size(m) == 1 && get_object_element(m, 2) == k ? node_or_val : def_v;
}

Force persistent_hash_map_assoc(Value m, Value k, Value v)
{
    const auto size = get_persistent_hash_map_size(m);
    if (size == 0)
        return create_single_value_map(k, v);
    if (size == 1)
    {
        if (get_object_element(m, 2) == k)
            return create_single_value_map(k, v);
        return create_collision_map(
            get_object_element(m, 2), get_object_element(m, 1),
            k, v);
    }
    auto replaced = false;
    Root new_node{collision_node_assoc(get_object_element(m, 1), k, v, replaced)};
    return create_map(replaced ? size : (size + 1), *new_node);
}

Value persistent_hash_map_contains(Value m, Value k)
{
    auto val = persistent_hash_map_get(m, k, *SENTINEL);
    return val.is(*SENTINEL) ? nil : TRUE;
}

}
