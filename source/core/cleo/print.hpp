#pragma once
#include "value.hpp"

namespace cleo
{

Value pr_str_object(Value val);
Value pr_str_small_vector(Value val);
Value pr_str_sequable(Value v);

Value pr_str(Value val);

}
