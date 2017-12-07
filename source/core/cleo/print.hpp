#pragma once
#include "value.hpp"

namespace cleo
{

Force pr_str_object(Value val);
Force pr_str_small_vector(Value val);
Force pr_str_sequable(Value v);

Force pr_str(Value val);

}
