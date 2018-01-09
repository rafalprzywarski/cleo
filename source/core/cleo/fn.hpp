#pragma once
#include "value.hpp"

namespace cleo
{

Force create_fn(Value env, Value name, Value params, Value body);
Force create_fn(Value env, Value name, const Value *params, const Value *bodies, std::uint8_t n);
Force create_macro(Value env, Value name, Value params, Value body);
Force create_macro(Value env, Value name, const Value *params, const Value *bodies, std::uint8_t n);
Value get_fn_env(Value fn);
Value get_fn_name(Value fn);
std::uint8_t get_fn_size(Value fn);
Value get_fn_params(Value fn, std::uint8_t i);
Value get_fn_body(Value fn, std::uint8_t i);

}
