#pragma once
#include "value.hpp"

namespace cleo
{

Force create_cons(Value elem, Value next);
Value cons_first(Value c);
Force cons_next(Value c);
Force cons_conj(Value c, Value elem);

}
