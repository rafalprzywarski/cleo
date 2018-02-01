#include "value.hpp"
#include "memory.hpp"
#include "global.hpp"
#include <cstring>

namespace cleo
{

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

Value tag_ptr(void *ptr, Tag tag)
{
    return Value{reinterpret_cast<decltype(Value::bits)>(ptr) | tag};
}

template <typename T>
T *get_ptr(Value val)
{
    return reinterpret_cast<T *>(val.bits & ~tag::MASK);
}

Force create_native_function(NativeFunction f)
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
    auto& entry = symbols[ns][name];
    if (entry != Value())
        return entry;
    Root ns_root, name_root;
    if (!ns.empty())
        ns_root = create_string(ns);
    name_root = create_string(name);
    auto val = alloc<Symbol>();
    val->ns = *ns_root;
    val->name = *name_root;
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
    auto& entry = keywords[ns][name];
    if (entry != Value())
        return entry;
    Root ns_root, name_root;
    if (!ns.empty())
        ns_root = create_string(ns);
    name_root = create_string(name);
    auto val = alloc<Keyword>();
    val->ns = *ns_root;
    val->name = *name_root;
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

Force create_int64(Int64 intVal)
{
    auto val = alloc<Int64>();
    *val = intVal;
    return tag_ptr(val, tag::INT64);
}

Int64 get_int64_value(Value val)
{
    return *get_ptr<Int64>(val);
}

Force create_float64(Float64 floatVal)
{
    auto val = alloc<Float64>();
    *val = floatVal;
    return tag_ptr(val, tag::FLOAT64);
}

Float64 get_float64_value(Value val)
{
    return *get_ptr<Float64>(val);
}

Force create_string(const std::string& str)
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

Force create_object(Value type, const Value *elems, std::uint32_t size)
{
    auto val = static_cast<Object *>(mem_alloc(offsetof(Object, firstElem) + size * sizeof(Object::firstElem)));
    val->type = type;
    val->size = size;
    std::copy_n(elems, size, &val->firstElem);
    return tag_ptr(val, tag::OBJECT);
}

Force create_object0(Value type)
{
    return create_object(type, nullptr, 0);
}

Force create_object1(Value type, Value elem)
{
    return create_object(type, &elem, 1);
}

Force create_object2(Value type, Value elem0, Value elem1)
{
    std::array<Value, 2> elems{{elem0, elem1}};
    return create_object(type, elems.data(), elems.size());
}

Force create_object3(Value type, Value elem0, Value elem1, Value elem2)
{
    std::array<Value, 3> elems{{elem0, elem1, elem2}};
    return create_object(type, elems.data(), elems.size());
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

void set_object_type(Value obj, Value type)
{
    get_ptr<Object>(obj)->type = type;
}

void set_object_element(Value obj, std::uint32_t index, Value val)
{
    (&get_ptr<Object>(obj)->firstElem)[index] = val;
}

Value get_value_type(Value val)
{
    auto tag = get_value_tag(val);
    if (tag == tag::OBJECT)
        return get_object_type(val);
    return type_by_tag[tag];
}

}
