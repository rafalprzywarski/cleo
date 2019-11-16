#pragma once
#include "value.hpp"

namespace cleo
{

std::string to_string(Value val);
Force CLEO_CDECL create_arity_error(Value name, std::uint8_t n);
Force CLEO_CDECL create_arg_type_error(Value val, std::uint8_t n);
[[noreturn]] void throw_call_error(const std::string& msg);
[[noreturn]] void throw_arity_error(Value name, std::uint8_t n);
[[noreturn]] void throw_arg_type_error(Value val, std::uint8_t n);
void check_arity(Value name, std::uint8_t num_args, std::uint8_t actual_num_args);
void check_type(const std::string& name, Value val, Value type);
[[noreturn]] void throw_illegal_argument(const std::string& msg);
[[noreturn]] void throw_integer_overflow();
[[noreturn]] void throw_index_out_of_bounds();
[[noreturn]] void throw_illegal_state(const std::string& msg);
[[noreturn]] void throw_compilation_error(const std::string& msg);

template <const Value *name>
inline Value deref_name() { return *name; }

template <>
inline Value deref_name<nullptr>() { return nil; }

template <Force f(), const Value *name>
Force create_native_function0()
{
    return create_native_function([](const Value *, std::uint8_t num_args)
    {
        if (num_args != 0)
            throw_arity_error(deref_name<name>(), num_args);
        return f();
    }, deref_name<name>());
}

template <Value f(), const Value *name>
Force create_native_function0()
{
    return create_native_function([](const Value *, std::uint8_t num_args)
    {
        if (num_args != 0)
            throw_arity_error(deref_name<name>(), num_args);
        return force(f());
    }, deref_name<name>());
}

template <Force f(), const Value *name>
Force create_native_new0()
{
    return create_native_function([](const Value *, std::uint8_t num_args)
    {
        if (num_args != 1)
            throw_arity_error(deref_name<name>(), num_args);
        return f();
    }, deref_name<name>());
}

template <Force f(Value), const Value *name = nullptr>
Force create_native_function1()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 1)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[0]);
    }, deref_name<name>());
}

template <Value f(Value), const Value *name = nullptr>
Force create_native_function1()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 1)
            throw_arity_error(deref_name<name>(), num_args);
        return force(f(args[0]));
    }, deref_name<name>());
}

template <Value f1(Value), Value f2(Value, Value), const Value *name = nullptr>
Force create_native_function1or2()
{
    return create_native_function([](const Value *args, std::uint8_t num_args) -> Force
    {
        if (num_args == 1)
            return f1(args[0]);
        if (num_args == 2)
            return f2(args[0], args[1]);
        throw_arity_error(deref_name<name>(), num_args);
    }, deref_name<name>());
}

template <Force f(Value), const Value *name = nullptr>
Force create_native_new1()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 2)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[1]);
    }, deref_name<name>());
}

template <Force f(Value, Value), const Value *name = nullptr>
Force create_native_function2()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 2)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[0], args[1]);
    }, deref_name<name>());
}

template <Value f(Value, Value), const Value *name = nullptr>
Force create_native_function2()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 2)
            throw_arity_error(deref_name<name>(), num_args);
        return force(f(args[0], args[1]));
    }, deref_name<name>());
}

template <Force f(Value, Value), const Value *name = nullptr>
Force create_native_new2()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 3)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[1], args[2]);
    }, deref_name<name>());
}

template <Force f(Value, Value, Value), const Value *name = nullptr>
Force create_native_function3()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 3)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[0], args[1], args[2]);
    }, deref_name<name>());
}

template <Value f(Value, Value, Value), const Value *name = nullptr>
Force create_native_function3()
{
    return create_native_function([](const Value *args, std::uint8_t num_args) -> Force
    {
        if (num_args != 3)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[0], args[1], args[2]);
    }, deref_name<name>());
}

template <Value f2(Value, Value), Value f3(Value, Value, Value), const Value *name = nullptr>
Force create_native_function2or3()
{
    return create_native_function([](const Value *args, std::uint8_t num_args) -> Force
    {
        if (num_args == 2)
            return f2(args[0], args[1]);
        if (num_args == 3)
            return f3(args[0], args[1], args[2]);
        throw_arity_error(deref_name<name>(), num_args);
    }, deref_name<name>());
}

template <Force f2(Value, Value), Force f3(Value, Value, Value), const Value *name = nullptr>
Force create_native_function2or3()
{
    return create_native_function([](const Value *args, std::uint8_t num_args) -> Force
    {
        if (num_args == 2)
            return f2(args[0], args[1]);
        if (num_args == 3)
            return f3(args[0], args[1], args[2]);
        throw_arity_error(deref_name<name>(), num_args);
    }, deref_name<name>());
}

template <Force f(Value, Value, Value), const Value *name = nullptr>
Force create_native_new3()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 4)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[1], args[2], args[3]);
    }, deref_name<name>());
}

template <Force f(Value, Value, Value, Value), const Value *name = nullptr>
Force create_native_function4()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 4)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[0], args[1], args[2], args[3]);
    }, deref_name<name>());
}

template <Force f(Value, Value, Value, Value, Value), const Value *name = nullptr>
Force create_native_function5()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 5)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[0], args[1], args[2], args[3], args[4]);
    }, deref_name<name>());
}

Int64 count(Value val);

bool is_map(Value val);
bool map_contains(Value m, Value k);
Force map_assoc(Value m, Value k, Value v);
Value map_get(Value m, Value k);
Int64 map_count(Value m);
Force map_merge(Value m1, Value m2);

Value namespace_symbol(Value sym);
Value name_symbol(Value sym);

bool is_seq(Value val);
Force seq(Value val);
Force seq_first(Value s);
Force seq_next(Value s);
std::uint32_t seq_count(Value s);

}
