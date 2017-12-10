#include "fn.hpp"
#include "global.hpp"
#include <array>

namespace cleo
{

Force create_fn(Value env, Value name, Value params, Value body)
{
    std::array<Value, 4> ms{{env, name, params, body}};
    return create_object(type::FN, ms.data(), ms.size());
}

Value get_fn_env(Value fn)
{
    return get_object_element(fn, 0);
}

Value get_fn_name(Value fn)
{
    return get_object_element(fn, 1);
}

Value get_fn_params(Value fn)
{
    return get_object_element(fn, 2);
}

Value get_fn_body(Value fn)
{
    return get_object_element(fn, 3);
}

}
