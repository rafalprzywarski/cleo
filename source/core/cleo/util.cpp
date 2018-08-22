#include "util.hpp"
#include "global.hpp"
#include "print.hpp"
#include "error.hpp"
#include "multimethod.hpp"

namespace cleo
{

std::string to_string(Value val)
{
    Root text{pr_str(val)};
    return {get_string_ptr(*text), get_string_len(*text)};
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

bool map_contains(Value m, Value k)
{
    return !Root(call_multimethod2(*rt::contains, m, k))->is_nil();
}

Force map_assoc(Value m, Value k, Value v)
{
    return call_multimethod3(*rt::assoc, m, k, v);
}

Value map_get(Value m, Value k)
{
    return *Root(call_multimethod2(*rt::get, m, k));
}

Force map_merge(Value m1, Value m2)
{
    return call_multimethod2(*rt::merge, m1, m2);
}

Value namespace_symbol(Value sym)
{
    check_type("symbol", sym, *type::Symbol);
    auto ns = get_symbol_namespace(sym);
    if (!ns)
        return nil;
    return create_symbol({get_string_ptr(ns), get_string_len(ns)});
}

Value name_symbol(Value sym)
{
    check_type("symbol", sym, *type::Symbol);
    if (!get_symbol_namespace(sym))
        return sym;
    auto name = get_symbol_name(sym);
    return create_symbol({get_string_ptr(name), get_string_len(name)});
}

}
