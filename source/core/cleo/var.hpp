#pragma once
#include "value.hpp"

namespace cleo
{

void define(Value sym, Value val);
Value lookup(Value sym);

}
