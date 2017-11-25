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

Value create_int64(std::int64_t intVal)
{
    auto val = alloc<std::int64_t>();
    *val = intVal;
    return tag_ptr(val, tag::INT64);
}

std::int64_t get_int_value(Value val)
{
    return *get_ptr<std::int64_t>(val);
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
