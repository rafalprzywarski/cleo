#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <cassert>

#ifdef __ARM_ARCH
#define CLEO_CDECL
#else
#define CLEO_CDECL __attribute__((cdecl))
#endif

namespace cleo
{

using ValueBits = std::uint64_t;

struct Value
{
    ValueBits bits_{0};
    Value() = default;
    explicit constexpr Value(ValueBits bits) : bits_(bits) { }
    explicit operator bool() const { return !is_nil(); }
    constexpr bool is(Value other) const { return bits_ == other.bits_; }
    constexpr bool is_nil() const { return bits_ == Value{}.bits_; }
    constexpr ValueBits bits() const { return bits_; }
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

struct Object
{
    Value type;
    std::uint32_t intCount, valCount;
    ValueBits firstVal;
    static constexpr int VALS_PER_INT = sizeof(Int64) / sizeof(ValueBits);
};

// Top 16 bits are flipped, so that nil encoding is 0
//               SEEEEEEE EEEEMMMM MMMMMMMM MMMMMMMM MMMMMMMM MMMMMMMM MMMMMMMM MMMMMMMM
// Float64:      | not all 0 ||--- -------- ------- float bits ------- -------- -------|
// Float64 QNaN: 00000000 00000111 00000000 00000000 00000000 00000000 00000000 00000000
// No SNaNs
// Pointer:      00000000 0000|tag||------- -------- 48 pointer bits - -------- -------| (tag != 0111)
// nil:          00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

namespace tag
{
constexpr Tag OBJECT = ValueBits(0) << 48;
constexpr Tag OBJECT_TYPE = ValueBits(1) << 48;
constexpr Tag NATIVE_FUNCTION = ValueBits(2) << 48;
constexpr Tag SYMBOL = ValueBits(3) << 48;
constexpr Tag KEYWORD = ValueBits(4) << 48;
constexpr Tag INT64 = ValueBits(5) << 48;
constexpr Tag STRING = ValueBits(6) << 48;
constexpr Tag FLOAT64 = ValueBits(7) << 48;

constexpr Tag DATA_MASK = ~(ValueBits(0xffff) << 48);
constexpr unsigned short DATA_SHIFT = 16;
constexpr Tag TAG_MASK = ValueBits(0xf) << 48;
constexpr Tag NAN_MASK = ValueBits(0xfff) << 52;
constexpr Tag FLIP_MASK = TAG_MASK | NAN_MASK;
}

constexpr Value nil{};


inline bool is_value_ptr(Value val)
{
    return (val.bits() & tag::NAN_MASK) == 0 && (val.bits() & tag::TAG_MASK) != tag::FLOAT64;
}

inline Tag get_value_tag(Value val)
{
    if ((val.bits() & tag::NAN_MASK) != 0)
        return tag::FLOAT64;
    return val.bits() & tag::TAG_MASK;
}

inline void *get_value_ptr(Value val)
{
    assert(is_value_ptr(val));
#ifdef __APPLE__
    static_assert(std::int64_t(-4) >> 2 == -1, "needs arithmetic left shift");
    return reinterpret_cast<void *>(std::uintptr_t(std::uint64_t(std::int64_t(val.bits() << tag::DATA_SHIFT) >> tag::DATA_SHIFT)));
#else
    return reinterpret_cast<void *>(std::uintptr_t(std::uint64_t(val.bits())));
#endif
}

template <typename T>
T *get_ptr(Value val)
{
    return reinterpret_cast<T *>(get_value_ptr(val));
}

Force create_native_function(NativeFunction f, Value name);
inline Force create_native_function(NativeFunction f)
{
    return create_native_function(f, nil);
}

NativeFunction get_native_function_ptr(Value val);
Value get_native_function_name(Value fn);

Value create_symbol(const std::string& ns, const std::string& name);
Value create_symbol(const std::string& name);
Value get_symbol_namespace(Value s);
Value get_symbol_name(Value s);
std::uint32_t get_symbol_hash(Value val);
void set_symbol_hash(Value val, std::uint32_t h);

Value create_keyword(const std::string& ns, const std::string& name);
Value create_keyword(const std::string& name);
Value get_keyword_namespace(Value s);
Value get_keyword_name(Value s);
std::uint32_t get_keyword_hash(Value val);
void set_keyword_hash(Value val, std::uint32_t h);

Force CLEO_CDECL create_int64(Int64 val);
ValueBits CLEO_CDECL create_int64_unsafe(Int64 val);

inline Int64 get_int64_value(Value val)
{
    return *get_ptr<Int64>(val);
}

Force create_float64(Float64 val);
Float64 get_float64_value(Value val);

Force create_string(const std::string& str);
const char *get_string_ptr(Value val);
std::uint32_t get_string_len(Value val);
std::uint32_t get_string_hash(Value val);
void set_string_hash(Value val, std::uint32_t h);

Force create_object(Value type, const Int64 *ints, std::uint32_t int_size, const Value *elems, std::uint32_t size);
inline Force create_object(Value type, const Value *elems, std::uint32_t size) { return create_object(type, nullptr, 0, elems, size); }
Force create_object0(Value type);
Force create_object1(Value type, Value elem);
Force create_object2(Value type, Value elem0, Value elem1);
Force create_object3(Value type, Value elem0, Value elem1, Value elem2);
Force create_object4(Value type, Value elem0, Value elem1, Value elem2, Value elem3);
Force create_object5(Value type, Value elem0, Value elem1, Value elem2, Value elem3, Value elem4);
Force create_object1_1(Value type, Int64 i0, Value elem0);
Force create_object1_2(Value type, Int64 i0, Value elem0, Value elem1);
Force create_object1_3(Value type, Int64 i0, Value elem0, Value elem1, Value elem2);
Force create_object1_4(Value type, Int64 i0, Value elem0, Value elem1, Value elem2, Value elem3);
std::uint32_t get_object_int_size(Value obj);
std::uint32_t get_object_size(Value obj);
void set_object_size(Value obj, std::uint32_t size);
Int64 get_object_int(Value obj, std::uint32_t index);
const void *get_object_int_ptr(Value obj, std::uint32_t index);
void set_object_type(Value obj, Value type);
void set_object_int(Value obj, std::uint32_t index, Int64 val);
void set_object_element(Value obj, std::uint32_t index, Value val);

Force create_object_type(Value name, const Value *fields, std::uint32_t size, bool is_constructible);
Force create_object_type(const std::string& ns, const std::string& name, const Value *fields, std::uint32_t size, bool is_constructible);
inline Force create_object_type(const std::string& ns, const std::string& name) { return create_object_type(ns, name, nullptr, 0, false); }
Value get_object_type_name(Value type);
Int64 get_object_type_field_count(Value type);
bool is_object_type_constructible(Value type);
Int64 get_object_field_index(Value type, Value name);

inline Value get_object_type(Value obj)
{
    return obj ? get_ptr<Object>(obj)->type : nil;
}

inline Value get_object_element(Value obj, std::uint32_t index)
{
    assert(index < get_object_size(obj));
    auto ptr = get_ptr<Object>(obj);
    return Value{(&ptr->firstVal)[ptr->intCount * Object::VALS_PER_INT + index]};
}

}
