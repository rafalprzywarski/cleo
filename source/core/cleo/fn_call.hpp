#pragma once
#include "value.hpp"

namespace cleo
{

Force create_fn_call(const Value *elems, std::uint32_t size);
std::uint32_t get_fn_call_size(Value fc);
Value get_fn_call_fn(Value fc);
Value get_fn_call_arg(Value fc, std::uint32_t index);
Value fn_call_equals(Value l, Value r);
Force fn_call_pr_str(Value fc);

}
