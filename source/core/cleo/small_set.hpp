#pragma once
#include "value.hpp"

namespace cleo
{

Force create_small_set();
std::uint32_t get_small_set_size(Value s);
Value get_small_set_elem(Value s, std::uint32_t index);
Value small_set_get(Value s, Value k);
Force small_set_conj(Value s, Value k);
Value small_set_contains(Value s, Value k);
Force small_set_seq(Value s);
Value get_small_set_seq_first(Value s);
Force get_small_set_seq_next(Value s);

}
