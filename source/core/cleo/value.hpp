#pragma once
#include <cstdint>

namespace cleo
{

using Value = std::uintptr_t;
using Type = Value;
using Int64 = std::int64_t;
using Float64 = double;
static_assert(sizeof(Float64) == 8, "Float64 should have 64 bits");

namespace tag
{
constexpr Type NIL = 0;
constexpr Type INT64 = 4;
constexpr Type FLOAT64 = 5;
constexpr Type STRING = 6;
constexpr Type MASK = 7;
}

inline Type get_value_tag(Value val)
{
    return val & tag::MASK;
}

inline Value get_nil()
{
    return 0;
}

Value create_int64(Int64 val);
Int64 get_int64_value(Value val);

Value create_float64(Float64 val);
Float64 get_float64_value(Value val);

Value create_string(const char *str, std::uint32_t len);
const char *get_string_ptr(Value val);
std::uint32_t get_string_len(Value val);

}
