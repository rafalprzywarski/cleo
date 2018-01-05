#pragma once
#include "value.hpp"

namespace cleo
{

Value in_ns(Value ns);
Value refer(Value ns);
Value define(Value sym, Value val);
Value lookup(Value sym);
Value require(Value ns);

}
