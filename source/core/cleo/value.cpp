#include "value.hpp"
#include "memory.hpp"
#include "global.hpp"
#include <cstring>
#include <algorithm>

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

struct ObjectType
{
    Value name;
};

struct Object
{
    Value type;
    std::uint32_t intCount, valCount;
    ValueBits firstVal;
    static constexpr int VALS_PER_INT = sizeof(Int64) / sizeof(ValueBits);
};

Value tag_ptr(void *ptr, Tag tag)
{
    return Value{reinterpret_cast<decltype(Value().bits())>(ptr) | tag};
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
    if (entry)
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
    if (entry)
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

Force CLEO_CDECL create_int64(Int64 intVal)
{
    auto val = alloc<Int64>();
    *val = intVal;
    return tag_ptr(val, tag::INT64);
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
    auto len = str.length();
    auto val = static_cast<String *>(mem_alloc(offsetof(String, firstChar) + len + 1));
    val->len = len;
    std::memcpy(&val->firstChar, str.data(), str.length());
    (&val->firstChar)[len] = 0;
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

Force create_object(Value type, const Int64 *ints, std::uint32_t int_size, const Value *elems, std::uint32_t size)
{
    assert(get_value_tag(type) == tag::OBJECT_TYPE);
    auto int_vals_size = int_size * Object::VALS_PER_INT;
    auto val = static_cast<Object *>(mem_alloc(offsetof(Object, firstVal) + (int_vals_size + size) * sizeof(Object::firstVal)));
    val->type = type;
    val->intCount = int_size;
    val->valCount = size;
    if (ints)
        std::memcpy(&val->firstVal, ints, int_vals_size * sizeof(Object::firstVal));
    else
        std::memset(&val->firstVal, 0, int_vals_size * sizeof(Object::firstVal));
    auto first_obj = &val->firstVal + int_vals_size;
    if (elems)
        std::transform(elems, elems + size, first_obj, [](auto& v) { return v.bits(); });
    else
        std::fill_n(first_obj, size, nil.bits());
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

Force create_object4(Value type, Value elem0, Value elem1, Value elem2, Value elem3)
{
    std::array<Value, 4> elems{{elem0, elem1, elem2, elem3}};
    return create_object(type, elems.data(), elems.size());
}

Force create_object5(Value type, Value elem0, Value elem1, Value elem2, Value elem3, Value elem4)
{
    std::array<Value, 5> elems{{elem0, elem1, elem2, elem3, elem4}};
    return create_object(type, elems.data(), elems.size());
}

Force create_object1_1(Value type, Int64 i0, Value elem0)
{
    return create_object(type, &i0, 1, &elem0, 1);
}

Force create_object1_2(Value type, Int64 i0, Value elem0, Value elem1)
{
    std::array<Value, 2> elems{{elem0, elem1}};
    return create_object(type, &i0, 1, elems.data(), elems.size());
}

Force create_object1_3(Value type, Int64 i0, Value elem0, Value elem1, Value elem2)
{
    std::array<Value, 3> elems{{elem0, elem1, elem2}};
    return create_object(type, &i0, 1, elems.data(), elems.size());
}

Force create_object1_4(Value type, Int64 i0, Value elem0, Value elem1, Value elem2, Value elem3)
{
    std::array<Value, 4> elems{{elem0, elem1, elem2, elem3}};
    return create_object(type, &i0, 1, elems.data(), elems.size());
}


Value get_object_type(Value obj)
{
    return obj ? get_ptr<Object>(obj)->type : nil;
}

std::uint32_t get_object_int_size(Value obj)
{
    return obj ? get_ptr<Object>(obj)->intCount : 0;
}

std::uint32_t get_object_size(Value obj)
{
    return obj ? get_ptr<Object>(obj)->valCount : 0;
}

void set_object_size(Value obj, std::uint32_t size)
{
    assert(size <= get_object_size(obj));
    get_ptr<Object>(obj)->valCount = size;
}

Int64 get_object_int(Value obj, std::uint32_t index)
{
    assert(index < get_object_int_size(obj));
    Int64 val{};
    std::memcpy(&val, get_object_int_ptr(obj, index), sizeof(val));
    return val;
}

const void *get_object_int_ptr(Value obj, std::uint32_t index)
{
    return &get_ptr<Object>(obj)->firstVal + index * Object::VALS_PER_INT;
}

Value get_object_element(Value obj, std::uint32_t index)
{
    assert(index < get_object_size(obj));
    auto ptr = get_ptr<Object>(obj);
    return Value{(&ptr->firstVal)[ptr->intCount * Object::VALS_PER_INT + index]};
}

void set_object_type(Value obj, Value type)
{
    get_ptr<Object>(obj)->type = type;
}

void set_object_int(Value obj, std::uint32_t index, Int64 val)
{
    assert(index < get_object_int_size(obj));
    std::memcpy(&get_ptr<Object>(obj)->firstVal + index * Object::VALS_PER_INT, &val, sizeof(val));
}

void set_object_element(Value obj, std::uint32_t index, Value val)
{
    assert(index < get_object_size(obj));
    auto ptr = get_ptr<Object>(obj);
    (&ptr->firstVal)[ptr->intCount * Object::VALS_PER_INT + index] = val.bits();
}

Force create_object_type(const std::string& ns, const std::string& name)
{
    auto name_sym = create_symbol(ns, name);
    auto t = alloc<ObjectType>();
    t->name = name_sym;
    return tag_ptr(t, tag::OBJECT_TYPE);
}

Value get_object_type_name(Value type)
{
    return get_ptr<ObjectType>(type)->name;
}

Value get_value_type(Value val)
{
    auto tag = get_value_tag(val);
    if (tag == tag::OBJECT)
        return get_object_type(val);
    return type_by_tag[tag];
}

}
