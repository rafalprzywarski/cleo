#pragma once
#include "value.hpp"

namespace cleo
{

Force create_macro(Value name, Value params, Value body);
Value get_macro_name(Value fn);
Value get_macro_params(Value fn);
Value get_macro_body(Value fn);

}
