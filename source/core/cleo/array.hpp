#pragma once
#include "value.hpp"

namespace cleo
{

Force create_array(const Value *elems, std::uint32_t size);
std::uint32_t get_array_size(Value v);
Value get_array_elem(Value v, std::uint32_t index);
Force array_seq(Value v);
Value get_array_seq_first(Value s);
Force get_array_seq_next(Value s);
Force array_conj(Value v, Value e);

}
