#pragma once
#include "value.hpp"

namespace cleo
{

std::string to_string(Value val);
Force CLEO_CDECL create_arity_error(Value name, std::uint8_t n);
Force CLEO_CDECL create_arg_type_error(Value val, std::uint8_t n);
[[noreturn]] void throw_arity_error(Value name, std::uint8_t n);
void check_arity(Value name, std::uint8_t num_args, std::uint8_t actual_num_args);
void check_type(const std::string& name, Value val, Value type);
[[noreturn]] void throw_illegal_argument(const std::string& msg);
[[noreturn]] void throw_integer_overflow();

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
    });
}

template <Force f(), const Value *name>
Force create_native_new0()
{
    return create_native_function([](const Value *, std::uint8_t num_args)
    {
        if (num_args != 1)
            throw_arity_error(deref_name<name>(), num_args);
        return f();
    });
}

template <Force f(Value), const Value *name = nullptr>
Force create_native_function1()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 1)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[0]);
    });
}

template <Value f(Value), const Value *name = nullptr>
Force create_native_function1()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 1)
            throw_arity_error(deref_name<name>(), num_args);
        return force(f(args[0]));
    });
}

template <Force f(Value), const Value *name = nullptr>
Force create_native_new1()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 2)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[1]);
    });
}

template <Force f(Value, Value), const Value *name = nullptr>
Force create_native_function2()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 2)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[0], args[1]);
    });
}

template <Value f(Value, Value), const Value *name = nullptr>
Force create_native_function2()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 2)
            throw_arity_error(deref_name<name>(), num_args);
        return force(f(args[0], args[1]));
    });
}

template <Force f(Value, Value), const Value *name = nullptr>
Force create_native_new2()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 3)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[1], args[2]);
    });
}

template <Force f(Value, Value, Value), const Value *name = nullptr>
Force create_native_function3()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 3)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[0], args[1], args[2]);
    });
}

template <Force f(Value, Value, Value), const Value *name = nullptr>
Force create_native_new3()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 4)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[1], args[2], args[3]);
    });
}

template <Force f(Value, Value, Value, Value, Value), const Value *name = nullptr>
Force create_native_function5()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 5)
            throw_arity_error(deref_name<name>(), num_args);
        return f(args[0], args[1], args[2], args[3], args[4]);
    });
}

bool map_contains(Value m, Value k);
Force map_assoc(Value m, Value k, Value v);
Value map_get(Value m, Value k);
Force map_merge(Value m1, Value m2);

}
