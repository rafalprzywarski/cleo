#pragma once
#include "value.hpp"

namespace cleo
{

class illegal_argument {};

Value define_multimethod(Value name, Value dispatchFn, Value defaultDispatchVal);
void define_method(Value name, Value dispatchVal, Value fn);
void derive(Value tag, Value parent);
Value isa(Value child, Value parent);
Value get_method(Value multi, Value dispatchVal);
Value call_multimethod(Value multi, const Value *args, std::uint8_t numArgs);

}
