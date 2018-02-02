#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <ostream>

namespace cleo
{

struct Value
{
    std::uintptr_t bits{0};
    Value() = default;
    explicit constexpr Value(std::uintptr_t bits) : bits(bits) { }
    explicit operator bool() const { return !is_nil(); }
    bool is(Value other) const { return bits == other.bits; }
    bool is_nil() const { return bits == Value{}.bits; }
};

static_assert(sizeof(Value) == sizeof(Value::bits), "Value should have no overhead");

inline bool operator==(Value left, Value right) { return left.is(right); }
inline bool operator!=(Value left, Value right) { return left.bits != right.bits; }
inline std::ostream& operator<<(std::ostream& os, Value val)
{
    return os << val.bits;
}

}

namespace std
{
template <>
struct hash<cleo::Value>
{
    using argument_type = cleo::Value;
    using result_type = std::size_t;

    result_type operator()(argument_type val) const
    {
        return std::hash<decltype(val.bits)>()(val.bits);
    }
};
}

namespace cleo
{

using Tag = std::uintptr_t;

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

constexpr Value nil{};


inline Tag get_value_tag(Value val)
{
    return val.bits & tag::MASK;
}

inline void *get_value_ptr(Value val)
{
    return reinterpret_cast<void *>(val.bits & ~tag::MASK);
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
void set_object_type(Value obj, Value type);
void set_object_element(Value obj, std::uint32_t index, Value val);

Value get_value_type(Value val);

}
