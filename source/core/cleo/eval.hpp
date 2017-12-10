#pragma once
#include "value.hpp"
#include <unordered_map>

namespace cleo
{

using Environment = std::unordered_map<Value, Value>;

Force eval(Value val, const Environment& env = {});

}
