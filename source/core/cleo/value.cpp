#include "value.hpp"
#include "memory.hpp"
#include <cstring>
#include <unordered_map>
#include <string>

namespace cleo
{

std::unordered_map<std::string, std::unordered_map<std::string, Value>> symbols;

struct String
{
    std::uint32_t len;
    char firstChar;
};

struct Symbol
{
    Value ns, name;
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

Value create_symbol(const char *ns, std::uint32_t ns_len, const char *name, std::uint32_t name_len)
{
    auto& entry = symbols[std::string(ns, ns_len)][std::string(name, name_len)];
    if (entry != Value())
        return entry;
    auto val = alloc<Symbol>();
    val->ns = ns ? create_string(ns, ns_len) : get_nil();
    val->name = create_string(name, name_len);
    return entry = tag_ptr(val, tag::SYMBOL);
}

Value create_symbol(const char *name, std::uint32_t name_len)
{
    return create_symbol(nullptr, 0, name, name_len);
}

Value get_symbol_namespace(Value s)
{
    return get_ptr<Symbol>(s)->ns;
}

Value get_symbol_name(Value s)
{
    return get_ptr<Symbol>(s)->name;
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
