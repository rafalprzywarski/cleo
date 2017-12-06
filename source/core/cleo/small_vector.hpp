#pragma once
#include "value.hpp"

namespace cleo
{

Value create_small_vector(const Value *elems, std::uint32_t size);
std::uint32_t get_small_vector_size(Value v);
Value get_small_vector_elem(Value v, std::uint32_t index);
Value small_vector_seq(Value v);
Value get_small_vector_seq_first(Value s);
Value get_small_vector_seq_next(Value s);

}
