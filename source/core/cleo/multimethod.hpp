#pragma once
#include "value.hpp"

namespace cleo
{
namespace type
{
extern const Value MULTIMETHOD;
}

class illegal_argument {};

Value define_multimethod(Value name, Value dispatchFn);
void define_method(Value name, Value dispatchVal, Value fn);
void derive(Value tag, Value parent);
Value isa(Value child, Value parent);
Value get_method(Value multi, Value dispatchVal);
Value call_multimethod(Value multi, const Value *args, std::uint8_t numArgs);

}
