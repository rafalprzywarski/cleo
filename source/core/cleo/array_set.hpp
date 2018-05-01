#pragma once
#include "value.hpp"

namespace cleo
{

Force create_array_set();
std::uint32_t get_array_set_size(Value s);
Value get_array_set_elem(Value s, std::uint32_t index);
Value array_set_get(Value s, Value k);
Force array_set_conj(Value s, Value k);
Value array_set_contains(Value s, Value k);
Force array_set_seq(Value s);
Value get_array_set_seq_first(Value s);
Force get_array_set_seq_next(Value s);

}
