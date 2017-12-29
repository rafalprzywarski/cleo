#include "fn.hpp"
#include "global.hpp"
#include <array>

namespace cleo
{

Force create_macro(Value name, Value params, Value body)
{
    std::array<Value, 3> ms{{name, params, body}};
    return create_object(type::Macro, ms.data(), ms.size());
}

Value get_macro_name(Value fn)
{
    return get_object_element(fn, 0);
}

Value get_macro_params(Value fn)
{
    return get_object_element(fn, 1);
}

Value get_macro_body(Value fn)
{
    return get_object_element(fn, 2);
}

}
