#pragma once
#include "value.hpp"

namespace cleo
{

Force resolve_value(Value val, Value env);

Force macroexpand1(Value form, Value env = nil);
Force macroexpand(Value form, Value env = nil);
Force apply(Value fn, Value args);
Force call(const Value *vals, std::uint32_t size);
Force eval(Value val, Value env = nil);
Force load(Value source);

}
