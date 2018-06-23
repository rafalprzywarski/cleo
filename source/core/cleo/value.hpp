#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <ostream>

#ifdef __ARM_ARCH
#define CLEO_CDECL
#else
#define CLEO_CDECL __attribute__((cdecl))
#endif

namespace cleo
{

using ValueBits = std::uintptr_t;

struct Value
{
    ValueBits bits_{0};
    Value() = default;
    explicit constexpr Value(std::uintptr_t bits) : bits_(bits) { }
    explicit operator bool() const { return !is_nil(); }
    constexpr bool is(Value other) const { return bits_ == other.bits_; }
    constexpr bool is_nil() const { return bits_ == Value{}.bits_; }
    constexpr std::uintptr_t bits() const { return bits_; }
};

static_assert(sizeof(Value) == sizeof(Value().bits()), "Value should have no overhead");

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
        return std::hash<decltype(val.bits())>()(val.bits());
    }
};
}

namespace cleo
{

using Tag = ValueBits;

struct Force
{
    Value val;
    Force(Value val) : val(val) { }
    Value value() const { return val; }
};

static_assert(sizeof(Force) == sizeof(Value), "Force should have no overhead");

inline Force force(Value val) { return val; }

using NativeFunction = Force(*)(const Value *, std::uint8_t);
using Int64 = std::int64_t;
using Float64 = double;
static_assert(sizeof(Float64) == 8, "Float64 should have 64 bits");

namespace tag
{
constexpr Tag OBJECT = 0;
constexpr Tag NATIVE_FUNCTION = 1;
constexpr Tag SYMBOL = 2;
constexpr Tag KEYWORD = 3;
constexpr Tag INT64 = 4;
constexpr Tag FLOAT64 = 5;
constexpr Tag STRING = 6;
constexpr Tag MASK = 7;
}

constexpr Value nil{};


inline Tag get_value_tag(Value val)
{
    return val.bits() & tag::MASK;
}

inline void *get_value_ptr(Value val)
{
    return reinterpret_cast<void *>(val.bits() & ~tag::MASK);
}

template <typename T>
T *get_ptr(Value val)
{
    return reinterpret_cast<T *>(val.bits() & ~tag::MASK);
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

Force CLEO_CDECL create_int64(Int64 val);
inline Int64 get_int64_value(Value val)
{
    return *get_ptr<Int64>(val);
}

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
Force create_object4(Value type, Value elem0, Value elem1, Value elem2, Value elem3);
Force create_object5(Value type, Value elem0, Value elem1, Value elem2, Value elem3, Value elem4);
Value get_object_type(Value obj);
std::uint32_t get_object_size(Value obj);
void set_object_size(Value obj, std::uint32_t size);
Value get_object_element(Value obj, std::uint32_t index);
void set_object_type(Value obj, Value type);
void set_object_element(Value obj, std::uint32_t index, Value val);

Value get_value_type(Value val);

}
