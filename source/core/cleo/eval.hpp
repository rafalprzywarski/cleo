#pragma once
#include "value.hpp"

namespace cleo
{

Force macroexpand1(Value form, Value env = nil);
Force macroexpand(Value form, Value env = nil);
Force apply(const Value *vals, std::uint32_t size);
Force call(const Value *vals, std::uint32_t size);
Force eval(Value val);
Force load(Value source);

}
