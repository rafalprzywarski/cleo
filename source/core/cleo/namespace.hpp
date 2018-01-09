#pragma once
#include "value.hpp"

namespace cleo
{

Value in_ns(Value ns);
Value refer(Value ns);
Value define(Value sym, Value val);
Value resolve(Value ns, Value sym);
Value resolve(Value sym);
Value lookup(Value ns, Value sym);
Value lookup(Value sym);
Value require(Value ns);

}
