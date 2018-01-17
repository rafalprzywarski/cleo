#pragma once
#include "value.hpp"

namespace cleo
{

std::string to_string(Value val);
[[noreturn]] void throw_arity_error(Value name, std::uint8_t n);
void check_arity(Value name, std::uint8_t num_args, std::uint8_t actual_num_args);
void check_type(const std::string& name, Value val, Value type);
[[noreturn]] void throw_illegal_argument(const std::string& msg);
[[noreturn]] void throw_integer_overflow();

template <Force f(), const Value *name>
Force create_native_function0()
{
    return create_native_function([](const Value *, std::uint8_t num_args)
    {
        if (num_args != 0)
            throw_arity_error(name != nullptr ? *name : nil, num_args);
        return f();
    });
}

template <Force f(Value), const Value *name = nullptr>
Force create_native_function1()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 1)
            throw_arity_error(name != nullptr ? *name : nil, num_args);
        return f(args[0]);
    });
}

template <Value f(Value), const Value *name = nullptr>
Force create_native_function1()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 1)
            throw_arity_error(name != nullptr ? *name : nil, num_args);
        return force(f(args[0]));
    });
}

template <Force f(Value, Value), const Value *name = nullptr>
Force create_native_function2()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 2)
            throw_arity_error(name != nullptr ? *name : nil, num_args);
        return f(args[0], args[1]);
    });
}

template <Value f(Value, Value), const Value *name = nullptr>
Force create_native_function2()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 2)
            throw_arity_error(name != nullptr ? *name : nil, num_args);
        return force(f(args[0], args[1]));
    });
}

template <Force f(Value, Value, Value), const Value *name = nullptr>
Force create_native_function3()
{
    return create_native_function([](const Value *args, std::uint8_t num_args)
    {
        if (num_args != 3)
            throw_arity_error(name != nullptr ? *name : nil, num_args);
        return f(args[0], args[1], args[2]);
    });
}

}
