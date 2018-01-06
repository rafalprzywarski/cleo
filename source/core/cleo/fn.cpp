#include "fn.hpp"
#include "global.hpp"
#include <array>

namespace cleo
{

namespace
{

Force create_fn(Value type, Value env, Value name, const Value *params, const Value *bodies, std::uint8_t n)
{
    std::vector<Value> ms;
    ms.reserve(2 + 2 * n);
    ms.push_back(env);
    ms.push_back(name);
    for (std::uint8_t i = 0; i < n; ++i)
    {
        ms.push_back(params[i]);
        ms.push_back(bodies[i]);
    }

    return create_object(type, ms.data(), ms.size());
}

}

Force create_fn(Value env, Value name, Value params, Value body)
{
    return create_fn(env, name, &params, &body, 1);
}

Force create_fn(Value env, Value name, const Value *params, const Value *bodies, std::uint8_t n)
{
    return create_fn(type::Fn, env, name, params, bodies, n);
}

Force create_macro(Value name, Value params, Value body)
{
    return create_macro(name, &params, &body, 1);
}

Force create_macro(Value name, const Value *params, const Value *bodies, std::uint8_t n)
{
    return create_fn(type::Macro, nil, name, params, bodies, n);
}

Value get_fn_env(Value fn)
{
    return get_object_element(fn, 0);
}

Value get_fn_name(Value fn)
{
    return get_object_element(fn, 1);
}

std::uint8_t get_fn_size(Value fn)
{
    return (get_object_size(fn) - 2) / 2;
}

Value get_fn_params(Value fn, std::uint8_t i)
{
    return get_object_element(fn, 2 + 2 * i);
}

Value get_fn_body(Value fn, std::uint8_t i)
{
    return get_object_element(fn, 3 + 2 * i);
}

}
