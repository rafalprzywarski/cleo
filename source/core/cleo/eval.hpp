#pragma once
#include "value.hpp"

namespace cleo
{

Force macroexpand1(Value val, Value form = nil, Value env = nil);
Force macroexpand(Value val, Value form = nil, Value env = nil);
Force apply(Value fn, Value args);
Force eval(Value val, Value env = nil);
Force load(Value source);

}
