#pragma once
#include "value.hpp"

namespace cleo
{

Value create_list(const Value *elems, std::uint32_t size);
Value get_list_size(Value list);
Value get_list_first(Value list);
Value get_list_next(Value list);
Value list_conj(Value list, Value elem);
Value list_seq(Value list);

}
