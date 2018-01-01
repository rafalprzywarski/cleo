#pragma once
#include "value.hpp"

namespace cleo
{

void define_var(Value sym, Value val);
Value lookup_var(Value sym);

}
