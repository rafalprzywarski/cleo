#pragma once
#include "value.hpp"

namespace cleo
{

Force resolve_value(Value val, Value env);

Force macroexpand1(Value val, Value form, Value env);
inline Force macroexpand1(Value form, Value env = nil) { return macroexpand1(form, form, env); }
Force macroexpand(Value val, Value form, Value env);
inline Force macroexpand(Value form, Value env = nil) { return macroexpand(form, form, env); }
Force apply(Value fn, Value args);
Force eval(Value val, Value env = nil);
Force load(Value source);

}
