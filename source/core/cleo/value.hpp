#pragma once
#include <cstdint>
#include <cstddef>
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

namespace type
{
extern const Value Int64;
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
using Char32 = std::uint32_t;
using Float64 = double;
static_assert(sizeof(Float64) == 8, "Float64 should have 64 bits");

struct ObjectType
{
    Value name;
    std::uint32_t fieldCount;
    bool isConstructible;
    bool isDynamic;
    struct NameType
    {
        Value name;
        Value type;
    } firstField;
};

struct StaticObject
{
    Value type;
    ValueBits firstVal;
};

struct DynamicObject
{
    Value type;
    std::uint32_t intCount, valCount;
    ValueBits firstVal;
};

// Top 16 bits are flipped, so that nil encoding is 0
//               SEEEEEEE EEEEMMMM MMMMMMMM MMMMMMMM MMMMMMMM MMMMMMMM MMMMMMMM MMMMMMMM
// Float64:      | not all 0 ||--- -------- ------- float bits ------- -------- -------|
// Float64 QNaN: 00000000 00000111 00000000 00000000 00000000 00000000 00000000 00000000
// No SNaNs
// Int48         00000000 00001101 |----------------  48 integer bits  -------- -------|
// Pointer:      00000000 0000|tag||------- --------  48 pointer bits  -------- -------| (tag != 0111) && (tag != 1101)
// nil:          00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

namespace tag
{
constexpr Tag OBJECT = ValueBits(0) << 48;
constexpr Tag OBJECT_TYPE = ValueBits(1) << 48;
constexpr Tag NATIVE_FUNCTION = ValueBits(2) << 48;
constexpr Tag SYMBOL = ValueBits(3) << 48;
constexpr Tag KEYWORD = ValueBits(4) << 48;
constexpr Tag INT64 = ValueBits(5) << 48;
constexpr Tag UTF8STRING = ValueBits(6) << 48;
constexpr Tag FLOAT64 = ValueBits(7) << 48;
constexpr Tag UCHAR = ValueBits(8) << 48;

constexpr Tag INT48 = ValueBits(13) << 48;

constexpr Tag DATA_MASK = ~(ValueBits(0xffff) << 48);
constexpr unsigned short DATA_SHIFT = 16;
constexpr Tag TAG_MASK = ValueBits(0xf) << 48;
constexpr Tag NAN_MASK = ValueBits(0xfff) << 52;
constexpr Tag FLIP_MASK = TAG_MASK | NAN_MASK;
}

constexpr Value nil{};


inline bool is_value_ptr(Value val)
{
    auto bits = val.bits();
    auto tag = bits & tag::TAG_MASK;
    return
        (bits & tag::NAN_MASK) == 0 &&
        (tag == tag::OBJECT ||
         tag == tag::OBJECT_TYPE ||
         tag == tag::NATIVE_FUNCTION ||
         tag == tag::SYMBOL ||
         tag == tag::KEYWORD ||
         tag == tag::INT64 ||
         tag == tag::UTF8STRING);
}

inline Tag get_value_tag(Value val)
{
    if ((val.bits() & tag::NAN_MASK) != 0)
        return tag::FLOAT64;
    auto tag = val.bits() & tag::TAG_MASK;
    if (tag == tag::INT48)
        return tag::INT64;
    return tag;
}

inline Int64 get_sign_extended_value_data(Value val)
{
    return Int64(val.bits() << tag::DATA_SHIFT) >> tag::DATA_SHIFT;
}

inline void *get_value_ptr(Value val)
{
    assert(is_value_ptr(val));
#ifdef __APPLE__
    static_assert(std::int64_t(-4) >> 2 == -1, "needs arithmetic left shift");
    return reinterpret_cast<void *>(std::uintptr_t(std::uint64_t(get_sign_extended_value_data(val))));
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
    static_assert(std::int64_t(-4) >> 2 == -1, "needs arithmetic left shift");
    if ((val.bits() & tag::TAG_MASK) == tag::INT48)
        return get_sign_extended_value_data(val);
    return *get_ptr<Int64>(val);
}

Value create_uchar(Char32 val);
Char32 get_uchar_value(Value val);

Force create_float64(Float64 val);
Float64 get_float64_value(Value val);

Force create_string(const char* str, std::uint32_t size);
inline Force create_string(const std::string& str) { return create_string(str.c_str(), str.length()); }
const char *get_string_ptr(Value val);
std::uint32_t get_string_size(Value val);
std::uint32_t get_string_len(Value val);
Char32 get_string_char(Value val, std::uint32_t index);
Char32 get_string_char_offset(Value val, std::uint32_t index);
Char32 get_string_char_at_offset(Value val, std::uint32_t offset);
std::uint32_t get_string_next_offset(Value val, std::uint32_t offset);
std::uint32_t get_string_hash(Value val);
void set_string_hash(Value val, std::uint32_t h);

Force create_object(Value type, const Int64 *ints, std::uint32_t int_size, const Value *elems, std::uint32_t size);
inline Force create_object(Value type, const Value *elems, std::uint32_t size) { return create_object(type, nullptr, 0, elems, size); }
Force create_static_object(Value type, Value elem0, Value elem1);
Force create_static_object(Value type, Int64 elem0, Value elem1, Value elem2);
Force create_static_object(Value type, Value elem0, Value elem1, Int64 elem2);
Force create_static_object(Value type, Value elem0, Value elem1, Int64 elem2, Value elem3);
Force create_static_object(Value type, Value elem0, Int64 elem1);
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
std::uint32_t get_dynamic_object_int_size(Value obj);
void set_dynamic_object_size(Value obj, std::uint32_t size);
void set_object_type(Value obj, Value type);
void set_dynamic_object_int(Value obj, std::uint32_t index, Int64 val);
void set_static_object_int(Value obj, std::uint32_t index, Int64 val);
void set_dynamic_object_element(Value obj, std::uint32_t index, Value val);
void set_static_object_element(Value obj, std::uint32_t index, Value val);

Force create_object_type(Value name, const Value *fields, const Value *types, std::uint32_t size, bool is_constructible, bool is_dynamic);
Force create_object_type(const std::string& ns, const std::string& name, const Value *fields, const Value *types, std::uint32_t size, bool is_constructible, bool is_dynamic);
inline Force create_dynamic_object_type(const std::string& ns, const std::string& name) { return create_object_type(ns, name, nullptr, nullptr, 0, false, true); }
inline Force create_static_object_type(Value name, const Value *fields, const Value *types, std::uint32_t size)
{ return create_object_type(name, fields, types, size, true, false); }
inline Force create_static_object_type(const std::string& ns, const std::string& name, const Value *fields, const Value *types, std::uint32_t size)
{ return create_object_type(ns, name, fields, types, size, true, false); }
Value get_object_type_name(Value type);
Int64 get_object_type_field_count(Value type);
Value get_object_type_field_type(Value type, Int64 index);
bool is_object_type_constructible(Value type);
Int64 get_object_field_index(Value type, Value name);

inline Value get_object_type_unchecked(Value obj)
{
    static_assert(offsetof(StaticObject, type) == 0, "type has to be first");
    static_assert(offsetof(DynamicObject, type) == 0, "type has to be first");
    assert(obj);
    return *get_ptr<Value>(obj);
}

inline bool is_object_type_dynamic(Value type)
{
    return get_ptr<ObjectType>(type)->isDynamic;
}

inline Value get_object_type(Value obj)
{
    return obj ? get_object_type_unchecked(obj) : nil;
}

inline bool is_object_dynamic(Value obj)
{
    assert(obj);
    return is_object_type_dynamic(get_object_type_unchecked(obj));
}

inline std::uint32_t get_dynamic_object_size(Value obj)
{
    if (!obj)
        return 0;
    assert(is_object_dynamic(obj));
    return get_ptr<DynamicObject>(obj)->valCount;
}

inline std::uint32_t get_static_object_size(Value obj)
{
    if (!obj)
        return 0;
    auto type = get_object_type(obj);
    assert(!is_object_type_dynamic(type));
    return get_ptr<ObjectType>(type)->fieldCount;
}

inline Value get_static_object_element_type(Value obj, std::uint32_t index)
{
    assert(!is_object_dynamic(obj));
    auto type = get_ptr<ObjectType>(get_object_type_unchecked(obj));
    assert(index < type->fieldCount);
    return (&type->firstField)[index].type;
}

inline bool is_static_object_element_value(Value obj, std::uint32_t index)
{
    return !get_static_object_element_type(obj, index).is(type::Int64);
}

inline Value get_dynamic_object_element(Value obj, std::uint32_t index)
{
    assert(is_object_dynamic(obj));
    assert(index < get_dynamic_object_size(obj));
    auto ptr = get_ptr<DynamicObject>(obj);
    return Value{(&ptr->firstVal)[ptr->intCount + index]};
}

inline Value get_static_object_element(Value obj, std::uint32_t index)
{
    assert(!is_object_dynamic(obj));
    assert(index < get_static_object_size(obj));
    assert(is_static_object_element_value(obj, index));
    return Value{(&get_ptr<StaticObject>(obj)->firstVal)[index]};
}

inline Int64 get_static_object_int(Value obj, std::uint32_t index)
{
    assert(!is_object_dynamic(obj));
    assert(index < get_static_object_size(obj));
    assert(get_static_object_element_type(obj, index).is(type::Int64));
    return Int64((&get_ptr<StaticObject>(obj)->firstVal)[index]);
}

inline const void *get_dynamic_object_int_ptr(Value obj, std::uint32_t index)
{
    assert(is_object_dynamic(obj));
    return &get_ptr<DynamicObject>(obj)->firstVal + index;
}

inline Int64 get_dynamic_object_int(Value obj, std::uint32_t index)
{
    assert(is_object_dynamic(obj));
    assert(index < get_dynamic_object_int_size(obj));
    return Int64((&get_ptr<DynamicObject>(obj)->firstVal)[index]);
}

}
