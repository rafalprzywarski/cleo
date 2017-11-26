#pragma once
#include "value.hpp"

namespace cleo
{

class symbol_not_found {};
class call_error {};

Value eval(Value val);

}
