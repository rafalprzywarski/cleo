#pragma once
#include "value.hpp"

namespace cleo
{

Force create_persistent_hash_map();
Int64 get_persistent_hash_map_size(Value m);
Value persistent_hash_map_get(Value m, Value k);
Value persistent_hash_map_get(Value m, Value k, Value def_v);
Force persistent_hash_map_assoc(Value map, Value key, Value val);
Force persistent_hash_map_dissoc(Value map, Value key);
Value persistent_hash_map_contains(Value m, Value k);
Value are_persistent_hash_maps_equal(Value left, Value right);
Force persistent_hash_map_seq(Value m);
Value get_persistent_hash_map_seq_first(Value s);
Force get_persistent_hash_map_seq_next(Value s);

}
