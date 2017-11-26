#include "value.hpp"
#include "memory.hpp"
#include "singleton.hpp"
#include <cstring>
#include <unordered_map>
#include <string>
#include <array>

namespace cleo
{

class Symbols{};
class Keywords{};
singleton<std::unordered_map<std::string, std::unordered_map<std::string, Value>>, Symbols> symbols;
singleton<std::unordered_map<std::string, std::unordered_map<std::string, Value>>, Keywords> keywords;

std::array<Value, 7> fixed_types{{
    nil,
    create_symbol("cleo.core", "NativeFunction"),
    create_symbol("cleo.core", "Symbol"),
    create_symbol("cleo.core", "Keyword"),
    create_symbol("cleo.core", "Int64"),
    create_symbol("cleo.core", "Float64"),
    create_symbol("cleo.core", "String")
}};

struct String
{
    std::uint32_t len;
    char firstChar;
};

struct Symbol
{
    Value ns, name;
};

struct Keyword
{
    Value ns, name;
};

struct Object
{
    Value type;
    std::uint32_t size;
    Value firstElem;
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

Value create_symbol(const std::string& ns, const std::string& name)
{
    auto& entry = (*symbols)[ns][name];
    if (entry != Value())
        return entry;
    auto val = alloc<Symbol>();
    val->ns = !ns.empty() ? create_string(ns) : nil;
    val->name = create_string(name);
    return entry = tag_ptr(val, tag::SYMBOL);
}

Value create_symbol(const std::string& name)
{
    return create_symbol({}, name);
}

Value get_symbol_namespace(Value s)
{
    return get_ptr<Symbol>(s)->ns;
}

Value get_symbol_name(Value s)
{
    return get_ptr<Symbol>(s)->name;
}

Value create_keyword(const std::string& ns, const std::string& name)
{
    auto& entry = (*keywords)[ns][name];
    if (entry != Value())
        return entry;
    auto val = alloc<Keyword>();
    val->ns = !ns.empty() ? create_string(ns) : nil;
    val->name = create_string(name);
    return entry = tag_ptr(val, tag::KEYWORD);
}

Value create_keyword(const std::string& name)
{
    return create_keyword({}, name);
}

Value get_keyword_namespace(Value s)
{
    return get_ptr<Keyword>(s)->ns;
}

Value get_keyword_name(Value s)
{
    return get_ptr<Keyword>(s)->name;
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

Value create_string(const std::string& str)
{
    auto val = static_cast<String *>(mem_alloc(offsetof(String, firstChar) + str.length()));
    val->len = str.length();
    std::memcpy(&val->firstChar, str.data(), str.length());
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

Value create_object(Value type, const Value *elems, std::uint32_t size)
{
    auto val = static_cast<Object *>(mem_alloc(offsetof(Object, firstElem) + size * sizeof(Object::firstElem)));
    val->type = type;
    val->size = size;
    std::copy_n(elems, size, &val->firstElem);
    return tag_ptr(val, tag::OBJECT);
}

Value get_object_type(Value obj)
{
    return get_ptr<Object>(obj)->type;
}

std::uint32_t get_object_size(Value obj)
{
    return get_ptr<Object>(obj)->size;
}

Value get_object_element(Value obj, std::uint32_t index)
{
    return (&get_ptr<Object>(obj)->firstElem)[index];
}

Value get_value_type(Value val)
{
    auto tag = get_value_tag(val);
    if (tag == tag::OBJECT)
        return get_object_type(val);
    return fixed_types[tag];
}

}
