#pragma once
#include "value.hpp"

namespace cleo
{

Value in_ns(Value ns);
Value refer(Value ns);
Value define(Value sym, Value val, Value meta);
inline Value define(Value sym, Value val) { return define(sym, val, nil); }
Value resolve(Value ns, Value sym);
Value resolve(Value sym);
Value lookup(Value ns, Value sym);
Value lookup(Value sym);
Value require(Value ns);

}
