#pragma once
#include "value.hpp"

namespace cleo
{

Force create_small_map();
std::uint32_t get_small_map_size(Value m);
Value small_map_get(Value m, Value k);
Force small_map_assoc(Value m, Value k, Value v);
Force small_map_merge(Value l, Value r);
Value small_map_contains(Value m, Value k);

}
