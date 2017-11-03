#pragma once
#include <cstdint>

namespace cleo
{

using Value = void *;
using Type = std::uint64_t;
using Int = std::int64_t;

namespace type
{
constexpr Type NIL = 0;
constexpr Type INT = 1;
}

Type get_value_type(Value val);
Value get_nil();
Value create_int(std::int64_t val);

Int get_int_value(Value val);

}
