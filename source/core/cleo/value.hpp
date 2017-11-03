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
constexpr Type STRING = 2;
}

Type get_value_type(Value val);

Value get_nil();

Value create_int(std::int64_t val);
Int get_int_value(Value val);

Value create_string(const char *str, std::uint32_t len);
const char *get_string_ptr(Value val);
std::uint32_t get_string_len(Value val);

}
