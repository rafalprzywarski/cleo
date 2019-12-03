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
    std::uint32_t size;
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

Value create_uchar(Char32 val)
{
    return tag_data(std::uint64_t(val), tag::UCHAR);
}

Char32 get_uchar_value(Value val)
{
    return Char32(val.bits());
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

namespace
{

auto valid_utf8_code_point_length(const char* str, const char *endp)
{
    struct R
    {
        std::uint32_t len{}, str_len{};
        bool valid{};
    };
    auto valid = [](std::uint32_t len) { return R{len, len, true}; };
    auto invalid = [](std::uint32_t len = 1) { return R{3, len, false}; };

    if ((*str & 0x80) == 0)
        return valid(1);
    if ((*str & 0xc0) == 0x80)
        return invalid();
    if ((*str & 0xe0) == 0xc0)
    {
        if ((endp - str) == 1 ||
            (str[1] & 0xc0) != 0x80)
            return invalid();
        if ((*str & 0x1e) == 0)
            return invalid(2);
        return valid(2);
    }
    if ((*str & 0xf0) == 0xe0)
    {
        if ((endp - str) < 3 ||
            (str[1] & 0xc0) != 0x80 ||
            (str[2] & 0xc0) != 0x80)
            return invalid();
        if ((str[0] & 0x0f) == 0 &&
            (str[1] & 0x20) == 0)
            return invalid(3);
        return valid(3);
    }
    if (static_cast<unsigned char>(*str) <= 0xf4)
    {
        if ((endp - str) < 4 ||
            (str[1] & 0xc0) != 0x80 ||
            (str[2] & 0xc0) != 0x80 ||
            (str[3] & 0xc0) != 0x80)
            return invalid();
        if ((str[0] & 0x7) == 0 &&
            (str[1] & 0x30) == 0)
            return invalid(4);
        if (static_cast<unsigned char>(*str) == 0xf4 &&
            (str[1] & 0x30) != 0)
            return invalid(4);
        return valid(4);
    }
    return invalid();
}

std::pair<std::uint32_t, bool> valid_utf8_string_size(const char* str, std::uint32_t size)
{
    std::uint32_t valid_size = 0;
    auto endp = str + size;
    bool valid = true;
    while (str != endp)
    {
        auto cpl = valid_utf8_code_point_length(str, endp);
        valid_size += cpl.len;
        str += cpl.str_len;
        valid = valid && cpl.valid;
    }
    return {valid_size, valid};
}

void fix_utf8_string(const char* str, std::uint32_t size, char *out)
{
    auto endp = str + size;
    while (str != endp)
    {
        auto cpl = valid_utf8_code_point_length(str, endp);
        if (!cpl.valid)
        {
            assert(cpl.len == 3);
            *out++ = 0xef;
            *out++ = 0xbf;
            *out++ = 0xbd;
            str += cpl.str_len;
            continue;
        }
        std::memcpy(out, str, cpl.len);
        out += cpl.len;
        str += cpl.len;
    }
}

std::uint32_t get_utf8_string_len(const char* str, std::uint32_t size)
{
    auto endp = str + size;
    std::uint32_t n = 0;
    for (; str != endp; ++str)
        n += ((*str & 0xc0) != 0x80);
    return n;
}

}

Force create_string(const char* str, std::uint32_t size)
{
    auto valid_size = valid_utf8_string_size(str, size);
    auto val = static_cast<String *>(mem_alloc(offsetof(String, firstChar) + valid_size.first + 1));
    val->size = valid_size.first;
    val->hashVal = 0;
    if (valid_size.second)
        std::memcpy(&val->firstChar, str, size);
    else
        fix_utf8_string(str, size, &val->firstChar);
    (&val->firstChar)[valid_size.first] = 0;
    val->len = get_utf8_string_len(&val->firstChar, val->size);
    return tag_ptr(val, tag::UTF8STRING);
}

const char *get_string_ptr(Value val)
{
    return &get_ptr<String>(val)->firstChar;
}

std::uint32_t get_string_size(Value val)
{
    return get_ptr<String>(val)->size;
}

std::uint32_t get_string_len(Value val)
{
    return get_ptr<String>(val)->len;
}

Char32 get_string_char(Value val, std::uint32_t index)
{
    return get_string_char_at_offset(val, get_string_char_offset(val, index));
}

Char32 get_string_char_offset(Value val, std::uint32_t index)
{
    assert(index <= get_string_len(val));
    auto offset = 0;
    for (; index; --index)
        offset = get_string_next_offset(val, offset);
    return offset;
}

Char32 get_string_char_at_offset(Value val, std::uint32_t offset)
{
    assert(offset < get_string_size(val));
    auto p = get_string_ptr(val) + offset;
    if ((*p & 0x80) == 0)
        return *p;
    if ((*p & 0x20) == 0)
        return (p[1] & 0x3f) | (Char32(p[0] & 0x1f) << 6);
    if ((*p & 0x10) == 0)
        return (p[2] & 0x3f) | (Char32(p[1] & 0x3f) << 6) | (Char32(p[0] & 0xf) << 12);
    return (p[3] & 0x3f) | (Char32(p[2] & 0x3f) << 6) | (Char32(p[1] & 0x3f) << 12) | (Char32(p[0] & 7) << 18);
}

std::uint32_t get_string_next_offset(Value val, std::uint32_t offset)
{
    assert(offset < get_string_size(val));
    unsigned char ch = get_string_ptr(val)[offset];
    return offset + ((0x43221111 >> (((ch & 0x80) >> 3) | ((ch & 0x30) >> 2))) & 0xf);
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

auto create_static_object_uninitialized(Value type)
{
    assert(get_value_tag(type) == tag::OBJECT_TYPE);
    auto otype = get_ptr<const ObjectType>(type);
    assert(!otype->isDynamic);
    auto val = static_cast<StaticObject *>(mem_alloc(offsetof(StaticObject, firstVal) + otype->fieldCount * sizeof(StaticObject::firstVal)));
    val->type = type;
    struct
    {
        ValueBits *elems;
        Value val;
    } ret{&val->firstVal, tag_ptr(val, tag::OBJECT)};
    return ret;
}

Force create_static_object(Value type, const ObjectType& otype, const Value *elems, std::uint32_t size)
{
    assert(size == otype.fieldCount);
    auto ev = create_static_object_uninitialized(type);
    if (elems)
        std::transform(elems, elems + size, &otype.firstField, ev.elems,
                       [](auto& v, auto& f)
                       {
                           if (!f.type.is(type::Int64))
                               return v.bits();
                           assert(get_value_tag(v) == tag::INT64);
                           return ValueBits(get_int64_value(v));
                       });
    else
        std::fill_n(ev.elems, size, nil.bits());
    assert(nil.bits() == 0);
    return ev.val;
}

Force create_dynamic_object(Value type, const ObjectType& otype, const Int64 *ints, std::uint32_t int_size, const Value *elems, std::uint32_t size)
{
    auto val = static_cast<DynamicObject *>(mem_alloc(offsetof(DynamicObject, firstVal) + (int_size + size) * sizeof(DynamicObject::firstVal)));
    val->type = type;
    val->intCount = int_size;
    val->valCount = size;
    if (ints)
        std::memcpy(&val->firstVal, ints, int_size * sizeof(val->firstVal));
    else
        std::memset(&val->firstVal, 0, int_size * sizeof(val->firstVal));
    auto first_obj = &val->firstVal + int_size;
    if (elems)
        std::transform(elems, elems + size, first_obj, [](auto& v) { return v.bits(); });
    else
        std::fill_n(first_obj, size, nil.bits());
    return tag_ptr(val, tag::OBJECT);
}

}

Force create_object(Value type, const Int64 *ints, std::uint32_t int_size, const Value *elems, std::uint32_t size)
{
    assert(get_value_tag(type) == tag::OBJECT_TYPE);
    auto otype = get_ptr<const ObjectType>(type);
    if (otype->isDynamic)
        return create_dynamic_object(type, *otype, ints, int_size, elems, size);
    assert(int_size == 0);
    return create_static_object(type, *otype, elems, size);
}

Force create_static_object(Value type, Int64 elem0, Value elem1, Value elem2)
{
    auto ev = create_static_object_uninitialized(type);
    ev.elems[0] = ValueBits(elem0);
    ev.elems[1] = elem1.bits();
    ev.elems[2] = elem2.bits();
    return ev.val;
}

Force create_static_object(Value type, Value elem0, Value elem1, Int64 elem2)
{
    auto ev = create_static_object_uninitialized(type);
    ev.elems[0] = elem0.bits();
    ev.elems[1] = elem1.bits();
    ev.elems[2] = ValueBits(elem2);
    return ev.val;
}

Force create_static_object(Value type, Value elem0, Value elem1, Int64 elem2, Value elem3)
{
    auto ev = create_static_object_uninitialized(type);
    ev.elems[0] = elem0.bits();
    ev.elems[1] = elem1.bits();
    ev.elems[2] = ValueBits(elem2);
    ev.elems[3] = elem3.bits();
    return ev.val;
}

Force create_static_object(Value type, Value elem0, Int64 elem1)
{
    auto ev = create_static_object_uninitialized(type);
    ev.elems[0] = elem0.bits();
    ev.elems[1] = ValueBits(elem1);
    return ev.val;
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


std::uint32_t get_dynamic_object_int_size(Value obj)
{
    if (!obj)
        return 0;
    assert(is_object_dynamic(obj));
    return get_ptr<DynamicObject>(obj)->intCount;
}

void set_dynamic_object_size(Value obj, std::uint32_t size)
{
    assert(size <= get_dynamic_object_size(obj));
    assert(is_object_dynamic(obj));
    get_ptr<DynamicObject>(obj)->valCount = size;
}

void set_object_type(Value obj, Value type)
{
    static_assert(offsetof(StaticObject, type) == 0, "type has to be first");
    static_assert(offsetof(DynamicObject, type) == 0, "type has to be first");
    assert(is_object_dynamic(obj) == is_object_type_dynamic(type));
    *get_ptr<Value>(obj) = type;
}

void set_dynamic_object_int(Value obj, std::uint32_t index, Int64 val)
{
    assert(is_object_dynamic(obj));
    assert(index < get_dynamic_object_int_size(obj));
    std::memcpy(&get_ptr<DynamicObject>(obj)->firstVal + index, &val, sizeof(val));
}

void set_static_object_int(Value obj, std::uint32_t index, Int64 val)
{
    assert(!is_object_dynamic(obj));
    auto type = get_ptr<ObjectType>(get_object_type(obj)); (void)type;
    assert(index < type->fieldCount);
    assert((&type->firstField)[index].type.is(type::Int64));
    (&get_ptr<StaticObject>(obj)->firstVal)[index] = ValueBits(val);
}

void set_dynamic_object_element(Value obj, std::uint32_t index, Value val)
{
    assert(is_object_dynamic(obj));
    assert(index < get_dynamic_object_size(obj));
    auto ptr = get_ptr<DynamicObject>(obj);
    (&ptr->firstVal)[ptr->intCount + index] = val.bits();
}

void set_static_object_element(Value obj, std::uint32_t index, Value val)
{
    assert(!is_object_dynamic(obj));
    auto type = get_ptr<ObjectType>(get_object_type(obj)); (void)type;
    assert(index < type->fieldCount);
    if (get_static_object_element_type(obj, index).is(type::Int64))
    {
        assert(get_value_tag(val) == tag::INT64);
        (&get_ptr<StaticObject>(obj)->firstVal)[index] = get_int64_value(val);
    }
    else
        (&get_ptr<StaticObject>(obj)->firstVal)[index] = val.bits();
}

Force create_object_type(Value name, const Value *fields, const Value *types, std::uint32_t size, bool is_constructible, bool is_dynamic)
{
    assert(get_value_type(name).is(*type::Symbol));
    auto t = static_cast<ObjectType *>(mem_alloc(offsetof(ObjectType, firstField) + size * sizeof(ObjectType::firstField)));
    t->name = name;
    t->fieldCount = size;
    t->isConstructible = is_constructible;
    t->isDynamic = is_dynamic;
    if (size)
    {
        if (types)
            std::transform(fields, fields + size, types, &t->firstField,
                           [](auto& name, auto& type) { return ObjectType::NameType{name, type}; });
        else
            std::transform(fields, fields + size, &t->firstField,
                           [](auto& name) { return ObjectType::NameType{name, nil}; });
    }
    return tag_ptr(t, tag::OBJECT_TYPE);
}

Force create_object_type(const std::string& ns, const std::string& name, const Value *fields, const Value *types, std::uint32_t size, bool is_constructible, bool is_dynamic)
{
    return create_object_type(create_symbol(ns, name), fields, types, size, is_constructible, is_dynamic);
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

Value get_object_type_field_type(Value type, Int64 index)
{
    assert(get_value_type(type).is(*type::Type));
    return (&get_ptr<ObjectType>(type)->firstField)[index].type;
}

bool is_object_type_constructible(Value type)
{
    assert(get_value_type(type).is(*type::Type));
    return get_ptr<ObjectType>(type)->isConstructible;
}

Int64 get_object_field_index(Value type, Value name)
{
    if (type.is_nil())
        return -1;
    auto ptr = get_ptr<ObjectType>(type);
    auto fields = &ptr->firstField;
    for (Int64 i = 0, size = ptr->fieldCount; i < size; ++i)
        if (fields[i].name == name)
            return i;
    return -1;
}

}
