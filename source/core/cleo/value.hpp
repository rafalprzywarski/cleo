#pragma once
#include <cstdint>
#include <string>

namespace cleo
{

using Value = std::uintptr_t;
using Tag = Value;

class Force
{
public:
    Force(Value val) : val(val) { }
private:
    Value val;
    friend class Root;
    friend class Roots;
};

inline Force force(Value val) { return val; }

using NativeFunction = Force(*)(const Value *, std::uint8_t);
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

constexpr Value nil = tag::NIL;


inline Tag get_value_tag(Value val)
{
    return val & tag::MASK;
}

inline void *get_value_ptr(Value ptr)
{
    return reinterpret_cast<void *>(ptr & ~tag::MASK);
}

Force create_native_function(NativeFunction f);

NativeFunction get_native_function_ptr(Value val);

Value create_symbol(const std::string& ns, const std::string& name);
Value create_symbol(const std::string& name);
Value get_symbol_namespace(Value s);
Value get_symbol_name(Value s);

Value create_keyword(const std::string& ns, const std::string& name);
Value create_keyword(const std::string& name);
Value get_keyword_namespace(Value s);
Value get_keyword_name(Value s);

Force create_int64(Int64 val);
Int64 get_int64_value(Value val);

Force create_float64(Float64 val);
Float64 get_float64_value(Value val);

Force create_string(const std::string& str);
const char *get_string_ptr(Value val);
std::uint32_t get_string_len(Value val);

Force create_object(Value type, const Value *elems, std::uint32_t size);
Force create_object0(Value type);
Force create_object1(Value type, Value elem);
Force create_object2(Value type, Value elem0, Value elem1);
Force create_object3(Value type, Value elem0, Value elem1, Value elem2);
Value get_object_type(Value obj);
std::uint32_t get_object_size(Value obj);
Value get_object_element(Value obj, std::uint32_t index);
void set_object_element(Value obj, std::uint32_t index, Value val);

Value get_value_type(Value val);

}
