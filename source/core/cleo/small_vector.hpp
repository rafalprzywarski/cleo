#pragma once
#include "value.hpp"

namespace cleo
{
namespace type
{
extern const Value SMALL_VECTOR;
}

Value create_small_vector(const Value *elems, std::uint32_t size);
std::uint32_t get_small_vector_size(Value v);
Value get_small_vector_elem(Value v, std::uint32_t index);

}
