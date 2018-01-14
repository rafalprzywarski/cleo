#pragma once
#include "value.hpp"

namespace cleo
{

std::string to_string(Value val);
[[noreturn]] void throw_arity_error(Value name, std::uint8_t n);

}
