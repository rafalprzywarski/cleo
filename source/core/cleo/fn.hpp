#pragma once
#include "value.hpp"

namespace cleo
{

Force create_fn(Value name, Value params, Value body);
Value get_fn_name(Value fn);
Value get_fn_params(Value fn);
Value get_fn_body(Value fn);

}
