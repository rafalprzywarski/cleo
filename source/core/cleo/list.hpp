#pragma once
#include "value.hpp"

namespace cleo
{

Force create_list(const Value *elems, std::uint32_t size);
Int64 get_list_size(Value list);
Value get_list_first(Value list);
Value get_list_next(Value list);
Force list_conj(Value list, Value elem);
Value list_seq(Value list);

}
