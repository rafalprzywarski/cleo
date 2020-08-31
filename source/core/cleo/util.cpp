#include "util.hpp"
#include "global.hpp"
#include "print.hpp"
#include "error.hpp"
#include "multimethod.hpp"
#include "persistent_hash_map.hpp"
#include "persistent_hash_set.hpp"
#include "array_map.hpp"
#include "array_set.hpp"

namespace cleo
{

std::string to_string(Value val)
{
    Root text{pr_str(val)};
    return {get_string_ptr(*text), get_string_size(*text)};
}

Force CLEO_CDECL create_arity_error(Value name, std::uint8_t n)
{
    Root msg{create_string("Wrong number of args (" + std::to_string(n) + ") passed to: " + to_string(name))};
    return new_call_error(*msg);
}

Force CLEO_CDECL create_arg_type_error(Value val, std::uint8_t n)
{
    Root msg{create_string("Wrong arg " + std::to_string(n) + " type: " + to_string(get_value_type(val)))};
    return new_call_error(*msg);
}

void throw_call_error(const std::string& msg)
{
    Root rmsg{create_string(msg)};
    throw_exception(new_call_error(*rmsg));
}

void throw_arity_error(Value name, std::uint8_t n)
{
    throw_exception(create_arity_error(name, n));
}

void throw_arg_type_error(Value val, std::uint8_t n)
{
    throw_exception(create_arg_type_error(val, n));
}

void check_arity(Value name, std::uint8_t num_args, std::uint8_t actual_num_args)
{
    if (num_args != actual_num_args)
        throw_arity_error(name, actual_num_args);
}

void check_type(const std::string& name, Value val, Value type)
{
    if (get_value_type(val).is(type))
        return;
    Root msg{create_string(name + " expected to be of type " + to_string(type))};
    throw_exception(new_illegal_argument(*msg));
}

void throw_illegal_argument(const std::string& msg)
{
    Root s{create_string(msg)};
    throw_exception(new_illegal_argument(*s));
}

void throw_integer_overflow()
{
    Root s{create_string("Integer overflow")};
    throw_exception(new_arithmetic_exception(*s));
}

void throw_index_out_of_bounds()
{
    throw_exception(new_index_out_of_bounds());
}

void throw_illegal_state(const std::string& msg)
{
    Root s{create_string(msg)};
    throw_exception(new_illegal_state(*s));
}

[[noreturn]] void throw_compilation_error(const std::string& msg)
{
    Root s{create_string(msg)};
    throw_exception(new_compilation_error(*s));
}

Int64 count(Value val)
{
    Root n{call_multimethod1(*rt::count, val)};
    return get_int64_value(*n);
}

bool is_map(Value val)
{
    auto vt = get_value_type(val);
    return vt == *type::PersistentHashMap || vt == *type::ArrayMap;
}

bool map_contains(Value m, Value k)
{
    if (!m)
        return false;
    auto type = get_value_type(m);
    if (type.is(*type::PersistentHashMap))
        return static_cast<bool>(persistent_hash_map_contains(m, k));
    if (type.is(*type::ArrayMap))
        return static_cast<bool>(array_map_contains(m, k));
    throw_illegal_argument("invalid map type: " + to_string(type));
}

namespace
{

Force array_map_to_persistent_hash_map(Value m)
{
    Root pm{*EMPTY_HASH_MAP};
    auto size = get_array_map_size(m);
    for (decltype(size) i = 0; i != size; ++i)
        pm = persistent_hash_map_assoc(*pm, get_array_map_key(m, i), get_array_map_val(m, i));
    return *pm;
}

Force array_set_to_persistent_hash_set(Value s)
{
    Root ps{*EMPTY_HASH_SET};
    auto size = get_array_set_size(s);
    for (decltype(size) i = 0; i != size; ++i)
        ps = persistent_hash_set_conj(*ps, get_array_set_elem(s, i));
    return *ps;
}

}

Force map_assoc(Value m, Value k, Value v)
{
    if (!m)
        m = *EMPTY_MAP;
    auto type = get_value_type(m);
    if (type.is(*type::PersistentHashMap))
        return persistent_hash_map_assoc(m, k, v);
    if (type.is(*type::ArrayMap))
    {
        if (get_array_map_size(m) >= 16)
        {
            Root pm{array_map_to_persistent_hash_map(m)};
            return persistent_hash_map_assoc(*pm, k, v);
        }
        return array_map_assoc(m, k, v);
    }
    throw_illegal_argument("invalid map type: " + to_string(type));
}

Value map_get(Value m, Value k)
{
    if (!m)
        return nil;
    auto type = get_value_type(m);
    if (type.is(*type::PersistentHashMap))
        return persistent_hash_map_get(m, k);
    if (type.is(*type::ArrayMap))
        return array_map_get(m, k);
    throw_illegal_argument("invalid map type: " + to_string(type));
}

Int64 map_count(Value m)
{
    if (!m)
        return 0;
    auto type = get_value_type(m);
    if (type.is(*type::PersistentHashMap))
        return get_persistent_hash_map_size(m);
    if (type.is(*type::ArrayMap))
        return get_array_map_size(m);
    throw_illegal_argument("invalid map type: " + to_string(type));
}

Force map_merge(Value m1, Value m2)
{
    return call_multimethod2(*rt::merge, m1, m2);
}

Force map_seq(Value m)
{
    if (!m)
        return nil;
    auto type = get_value_type(m);
    if (type.is(*type::PersistentHashMap))
        return persistent_hash_map_seq(m);
    if (type.is(*type::ArrayMap))
        return array_map_seq(m);
    throw_illegal_argument("invalid map type: " + to_string(type));
}

Value map_seq_first(Value s)
{
    if (!s)
        return nil;
    auto type = get_value_type(s);
    if (type.is(*type::PersistentHashMapSeq))
        return get_persistent_hash_map_seq_first(s);
    if (type.is(*type::ArrayMapSeq))
        return get_array_map_seq_first(s);
    throw_illegal_argument("invalid map seq type: " + to_string(type));
}

Force map_seq_next(Value s)
{
    if (!s)
        return nil;
    auto type = get_value_type(s);
    if (type.is(*type::PersistentHashMapSeq))
        return get_persistent_hash_map_seq_next(s);
    if (type.is(*type::ArrayMapSeq))
        return get_array_map_seq_next(s);
    throw_illegal_argument("invalid map seq type: " + to_string(type));
}

bool is_set(Value val)
{
    auto vt = get_value_type(val);
    return vt == *type::PersistentHashSet || vt == *type::ArraySet;
}

Force set_conj(Value s, Value k)
{
    if (!s)
        s = *EMPTY_SET;
    auto type = get_value_type(s);
    if (type.is(*type::PersistentHashSet))
        return persistent_hash_set_conj(s, k);
    if (type.is(*type::ArraySet))
    {
        if (get_array_set_size(s) >= 16)
        {
            Root ps{array_set_to_persistent_hash_set(s)};
            return persistent_hash_set_conj(*ps, k);
        }
        return array_set_conj(s, k);
    }
    throw_illegal_argument("invalid set type: " + to_string(type));
}

bool set_contains(Value s, Value k)
{
    if (!s)
        return false;
    auto type = get_value_type(s);
    if (type.is(*type::PersistentHashSet))
        return static_cast<bool>(persistent_hash_set_contains(s, k));
    if (type.is(*type::ArraySet))
        return static_cast<bool>(array_set_contains(s, k));
    throw_illegal_argument("invalid set type: " + to_string(type));
}

Int64 set_count(Value s)
{
    if (!s)
        return 0;
    auto type = get_value_type(s);
    if (type.is(*type::PersistentHashSet))
        return get_persistent_hash_set_size(s);
    if (type.is(*type::ArraySet))
        return get_array_set_size(s);
    throw_illegal_argument("invalid set type: " + to_string(type));
}

Value namespace_symbol(Value sym)
{
    check_type("symbol", sym, *type::Symbol);
    auto ns = get_symbol_namespace(sym);
    if (!ns)
        return nil;
    return create_symbol({get_string_ptr(ns), get_string_size(ns)});
}

Value name_symbol(Value sym)
{
    check_type("symbol", sym, *type::Symbol);
    if (!get_symbol_namespace(sym))
        return sym;
    auto name = get_symbol_name(sym);
    return create_symbol({get_string_ptr(name), get_string_size(name)});
}

bool is_seq(Value val)
{
    return static_cast<bool>(isa(get_value_type(val), *type::Sequence));
}

Force seq(Value val)
{
    return call_multimethod1(*rt::seq, val);
}

Force seq_first(Value s)
{
    return call_multimethod1(*rt::first, s);
}

Force seq_next(Value s)
{
    return call_multimethod1(*rt::next, s);
}

std::uint32_t seq_count(Value s)
{
    std::uint32_t n = 0;
    for (Root v{seq(s)}; *v; v = seq_next(*v))
        ++n;
    return n;
}

}
