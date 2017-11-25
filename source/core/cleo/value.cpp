#include "value.hpp"
#include "memory.hpp"
#include <cstring>

namespace cleo
{

struct String
{
    std::uint32_t len;
    char firstChar;
};

Value tag_ptr(void *ptr, Value tag)
{
    return reinterpret_cast<Value>(ptr) | tag;
}

template <typename T>
T *get_ptr(Value ptr)
{
    return reinterpret_cast<T *>(ptr & ~tag::MASK);
}

Value create_native_function(NativeFunction f)
{
    auto val = alloc<NativeFunction>();
    *val = f;
    return tag_ptr(val, tag::NATIVE_FUNCTION);
}

NativeFunction get_native_function_ptr(Value val)
{
    return *get_ptr<NativeFunction>(val);
}

Value create_int64(Int64 intVal)
{
    auto val = alloc<Int64>();
    *val = intVal;
    return tag_ptr(val, tag::INT64);
}

Int64 get_int64_value(Value val)
{
    return *get_ptr<Int64>(val);
}

Value create_float64(Float64 floatVal)
{
    auto val = alloc<Float64>();
    *val = floatVal;
    return tag_ptr(val, tag::FLOAT64);
}

Float64 get_float64_value(Value val)
{
    return *get_ptr<Float64>(val);
}

Value create_string(const char *str, std::uint32_t len)
{
    auto val = static_cast<String *>(mem_alloc(offsetof(String, firstChar) + len));
    val->len = len;
    std::memcpy(&val->firstChar, str, len);
    return tag_ptr(val, tag::STRING);
}

const char *get_string_ptr(Value val)
{
    return &get_ptr<String>(val)->firstChar;
}

std::uint32_t get_string_len(Value val)
{
    return get_ptr<String>(val)->len;
}

}
