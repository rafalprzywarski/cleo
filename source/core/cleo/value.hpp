#pragma once
#include <cstdint>

namespace cleo
{

using Value = std::uintptr_t;
using Tag = Value;
using NativeFunction = Value(*)(const Value *, std::uint8_t);
using Int64 = std::int64_t;
using Float64 = double;
static_assert(sizeof(Float64) == 8, "Float64 should have 64 bits");

namespace tag
{
constexpr Tag NIL = 0;
constexpr Tag NATIVE_FUNCTION = 1;
constexpr Tag SYMBOL = 2;
constexpr Tag KEYWORD = 3;
constexpr Tag INT64 = 4;
constexpr Tag FLOAT64 = 5;
constexpr Tag STRING = 6;
constexpr Tag OBJECT = 7;
constexpr Tag MASK = 7;
}

inline Tag get_value_tag(Value val)
{
    return val & tag::MASK;
}

inline Value get_nil()
{
    return 0;
}

Value create_native_function(NativeFunction f);
NativeFunction get_native_function_ptr(Value val);

Value create_symbol(const char *ns, std::uint32_t ns_len, const char *name, std::uint32_t name_len);
Value create_symbol(const char *name, std::uint32_t name_len);
Value get_symbol_namespace(Value s);
Value get_symbol_name(Value s);

Value create_keyword(const char *ns, std::uint32_t ns_len, const char *name, std::uint32_t name_len);
Value create_keyword(const char *name, std::uint32_t name_len);
Value get_keyword_namespace(Value s);
Value get_keyword_name(Value s);

Value create_int64(Int64 val);
Int64 get_int64_value(Value val);

Value create_float64(Float64 val);
Float64 get_float64_value(Value val);

Value create_string(const char *str, std::uint32_t len);
const char *get_string_ptr(Value val);
std::uint32_t get_string_len(Value val);

Value create_object(Value type, const Value *elems, std::uint32_t size);
std::uint32_t get_object_size(Value obj);
Value get_object_element(Value obj, std::uint32_t index);

}
