#include "value.hpp"
#include "memory.hpp"
#include "global.hpp"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace cleo
{

struct NativeFunctionWithName
{
    Value name;
    NativeFunction ptr;
};

struct String
{
    std::uint32_t len;
    std::uint32_t hashVal;
    char firstChar;
};

struct Symbol
{
    Value ns, name;
    std::uint32_t hashVal;
};

struct Keyword
{
    Value ns, name;
    std::uint32_t hashVal;
};

namespace
{

template <typename T, typename U>
T bit_cast(const U& u)
{
    static_assert(sizeof(U) == sizeof(T), "types sizes should match");
    T t;
    std::memcpy(&t, &u, sizeof(u));
    return t;
}

Value tag_data(ValueBits data, Tag tag)
{
    return Value{(data & tag::DATA_MASK) | tag};
}

Value tag_ptr(void *ptr, Tag tag)
{
    return tag_data(reinterpret_cast<std::uintptr_t>(ptr), tag);
}

}

Force create_native_function(NativeFunction f, Value name)
{
    assert(!name || get_value_tag(name) == tag::SYMBOL);
    auto val = alloc<NativeFunctionWithName>();
    val->name = name;
    val->ptr = f;
    return tag_ptr(val, tag::NATIVE_FUNCTION);
}

NativeFunction get_native_function_ptr(Value fn)
{
    return get_ptr<NativeFunctionWithName>(fn)->ptr;
}

Value get_native_function_name(Value fn)
{
    return get_ptr<NativeFunctionWithName>(fn)->name;
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
    val->hashVal = 0;
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

std::uint32_t get_symbol_hash(Value val)
{
    return get_ptr<Symbol>(val)->hashVal;
}

void set_symbol_hash(Value val, std::uint32_t h)
{
    get_ptr<Symbol>(val)->hashVal = h;
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
    val->hashVal = 0;
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

std::uint32_t get_keyword_hash(Value val)
{
    return get_ptr<Keyword>(val)->hashVal;
}

void set_keyword_hash(Value val, std::uint32_t h)
{
    get_ptr<Keyword>(val)->hashVal = h;
}

Force create_int64(Int64 intVal)
{
    static_assert(std::int64_t(-4) >> 2 == -1, "needs arithmetic left shift");
    if ((Int64(std::uint64_t(intVal) << tag::DATA_SHIFT) >> tag::DATA_SHIFT) == intVal)
        return tag_data(std::uint64_t(intVal), tag::INT48);
    auto val = alloc<Int64>();
    *val = intVal;
    return tag_ptr(val, tag::INT64);
}

ValueBits CLEO_CDECL create_int64_unsafe(Int64 val)
{
    return create_int64(val).value().bits();
}

Force create_float64(Float64 floatVal)
{
    if (std::isnan(floatVal))
        return Value{tag::FLOAT64};
    return Value{bit_cast<ValueBits>(floatVal) ^ tag::FLIP_MASK};
}

Float64 get_float64_value(Value val)
{
    return bit_cast<Float64>(val.bits() ^ tag::FLIP_MASK);
}

Force create_string(const std::string& str)
{
    auto len = str.length();
    auto val = static_cast<String *>(mem_alloc(offsetof(String, firstChar) + len + 1));
    val->len = len;
    val->hashVal = 0;
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

std::uint32_t get_string_hash(Value val)
{
    return get_ptr<String>(val)->hashVal;
}

void set_string_hash(Value val, std::uint32_t h)
{
    get_ptr<String>(val)->hashVal = h;
}

namespace
{

template <typename T>
T *set_up_common_object(Value type, const ObjectType& otype, const Int64 *ints, std::uint32_t int_size, const Value *elems, std::uint32_t size)
{
    auto val = static_cast<T *>(mem_alloc(offsetof(T, firstVal) + (int_size + size) * sizeof(T::firstVal)));
    val->type = type;
    if (ints)
        std::memcpy(&val->firstVal, ints, int_size * sizeof(DynamicObject::firstVal));
    else
        std::memset(&val->firstVal, 0, int_size * sizeof(DynamicObject::firstVal));
    auto first_obj = &val->firstVal + int_size;
    if (elems)
        std::transform(elems, elems + size, first_obj, [](auto& v) { return v.bits(); });
    else
        std::fill_n(first_obj, size, nil.bits());
    return val;
}

}

Force create_object(Value type, const Int64 *ints, std::uint32_t int_size, const Value *elems, std::uint32_t size)
{
    assert(get_value_tag(type) == tag::OBJECT_TYPE);
    auto otype = get_ptr<const ObjectType>(type);
    if (otype->isDynamic)
    {
        auto val = set_up_common_object<DynamicObject>(type, *otype, ints, int_size, elems, size);
        val->intCount = int_size;
        val->valCount = size;
        return tag_ptr(val, tag::OBJECT);
    }
    assert(int_size == 0);
    assert(size == otype->fieldCount);
    return tag_ptr(set_up_common_object<StaticObject>(type, *otype, nullptr, 0, elems, size), tag::OBJECT);
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


std::uint32_t get_object_int_size(Value obj)
{
    if (!obj || !is_object_dynamic(obj))
        return 0;
    return get_ptr<DynamicObject>(obj)->intCount;
}

std::uint32_t get_object_size(Value obj)
{
    if (!obj)
        return 0;
    auto type = get_ptr<ObjectType>(get_object_type(obj));
    if (type->isDynamic)
        return get_ptr<DynamicObject>(obj)->valCount;
    return type->fieldCount;
}

void set_object_size(Value obj, std::uint32_t size)
{
    assert(size <= get_object_size(obj));
    assert(is_object_dynamic(obj));
    get_ptr<DynamicObject>(obj)->valCount = size;
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
    assert(is_object_dynamic(obj));
    return &get_ptr<DynamicObject>(obj)->firstVal + index;
}

void set_object_type(Value obj, Value type)
{
    static_assert(offsetof(StaticObject, type) == 0, "type has to be first");
    static_assert(offsetof(DynamicObject, type) == 0, "type has to be first");
    *get_ptr<Value>(obj) = type;
}

void set_object_int(Value obj, std::uint32_t index, Int64 val)
{
    assert(index < get_object_int_size(obj));
    std::memcpy(&get_ptr<DynamicObject>(obj)->firstVal + index, &val, sizeof(val));
}

void set_object_element(Value obj, std::uint32_t index, Value val)
{
    assert(index < get_object_size(obj));
    if (!is_object_dynamic(obj))
    {
        (&get_ptr<StaticObject>(obj)->firstVal)[index] = val.bits();
        return;
    }
    auto ptr = get_ptr<DynamicObject>(obj);
    (&ptr->firstVal)[ptr->intCount + index] = val.bits();
}

Force create_object_type(Value name, const Value *fields, std::uint32_t size, bool is_constructible, bool is_dynamic)
{
    assert(get_value_type(name).is(*type::Symbol));
    auto t = static_cast<ObjectType *>(mem_alloc(offsetof(ObjectType, firstFieldName) + size * sizeof(ObjectType::firstFieldName)));
    t->name = name;
    t->fieldCount = size;
    t->isConstructible = is_constructible;
    t->isDynamic = is_dynamic;
    if (size)
        std::copy_n(fields, size, &t->firstFieldName);
    return tag_ptr(t, tag::OBJECT_TYPE);
}

Force create_object_type(const std::string& ns, const std::string& name, const Value *fields, std::uint32_t size, bool is_constructible, bool is_dynamic)
{
    return create_object_type(create_symbol(ns, name), fields, size, is_constructible, is_dynamic);
}

Value get_object_type_name(Value type)
{
    assert(get_value_type(type).is(*type::Type));
    return get_ptr<ObjectType>(type)->name;
}

Int64 get_object_type_field_count(Value type)
{
    assert(get_value_type(type).is(*type::Type));
    return get_ptr<ObjectType>(type)->fieldCount;
}

bool is_object_type_constructible(Value type)
{
    assert(get_value_type(type).is(*type::Type));
    return get_ptr<ObjectType>(type)->isConstructible;
}

Int64 get_object_field_index(Value type, Value name)
{
    auto ptr = get_ptr<ObjectType>(type);
    auto names = &ptr->firstFieldName;
    for (Int64 i = 0, size = ptr->fieldCount; i < size; ++i)
        if (names[i] == name)
            return i;
    return -1;
}

}
