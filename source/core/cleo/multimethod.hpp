#pragma once
#include "value.hpp"

namespace cleo
{

Value define_multimethod(Value name, Value dispatchFn, Value defaultDispatchVal);
void define_method(Value name, Value dispatchVal, Value fn);
void create_global_hierarchy();
void derive(Value tag, Value parent);
Value isa(Value child, Value parent);
Value get_method(Value multi, Value dispatchVal);
Value get_multimethod_name(Value multi);
Force call_multimethod(Value multi, const Value *args, std::uint8_t numArgs);
Force call_multimethod1(Value multi, Value arg);
Force call_multimethod2(Value multi, Value arg0, Value arg1);
Force call_multimethod3(Value multi, Value arg0, Value arg1, Value arg2);

}
