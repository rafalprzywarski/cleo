#pragma once
#include "value.hpp"

namespace cleo
{

Force create_atom(Value val);
Value atom_deref(Value atom);
Force atom_reset(Value atom, Value val);

}
