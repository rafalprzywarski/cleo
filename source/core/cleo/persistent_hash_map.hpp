#pragma once
#include "value.hpp"

namespace cleo
{

Force create_persistent_hash_map();
Int64 get_persistent_hash_map_size(Value m);
Value persistent_hash_map_get(Value m, Value k);
Value persistent_hash_map_get(Value m, Value k, Value def_v);
Force persistent_hash_map_assoc(Value m, Value k, Value v);
Value persistent_hash_map_contains(Value m, Value k);

}
