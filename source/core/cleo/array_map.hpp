#pragma once
#include "value.hpp"

namespace cleo
{

Force create_array_map();
std::uint32_t get_array_map_size(Value m);
Value get_array_map_key(Value m, std::uint32_t index);
Value get_array_map_val(Value m, std::uint32_t index);
Value array_map_get(Value m, Value k);
Value array_map_get(Value m, Value k, Value def_v);
Force array_map_assoc(Value m, Value k, Value v);
Force array_map_dissoc(Value m, Value k);
Force array_map_merge(Value l, Value r);
Value array_map_contains(Value m, Value k);
Force array_map_seq(Value m);
Value get_array_map_seq_first(Value s);
Force get_array_map_seq_next(Value s);

}
