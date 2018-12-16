#pragma once
#include "value.hpp"

namespace cleo
{

Value define_ns(Value name, Value meta);
Value ns_name(Value ns);
Value find_ns(Value name);
Value get_ns(Value name);
Value get_ns_meta(Value ns);
Value in_ns(Value ns, Value meta);
inline Value in_ns(Value ns) { return in_ns(ns, nil); }
Value refer(Value ns);
Value define(Value sym, Value val, Value meta);
inline Value define(Value sym, Value val) { return define(sym, val, nil); }
Value resolve_var(Value ns, Value sym);
Value resolve_var(Value sym);
Value maybe_resolve_var(Value ns, Value sym);
Value maybe_resolve_var(Value sym);
Value lookup(Value ns, Value sym);
Value lookup(Value sym);
Value require(Value ns);
Value alias(Value as, Value ns);
Value ns_aliases(Value ns);
Value ns_map(Value ns);

}
