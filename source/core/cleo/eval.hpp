#pragma once
#include "value.hpp"

namespace cleo
{

Force macroexpand1(Value val);
Force macroexpand(Value val);
Force eval(Value val, Value env = nil);

}
