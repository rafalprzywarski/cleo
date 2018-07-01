#include "fn_call.hpp"
#include "util.hpp"
#include "global.hpp"
#include <cassert>

namespace cleo
{

Force create_fn_call(const Value *elems, std::uint32_t size)
{
    assert(size >= 1);
    return create_object(*type::FnCall, elems, size);
}

std::uint32_t get_fn_call_size(Value fc)
{
    return get_object_size(fc) - 1;
}

Value get_fn_call_fn(Value fc)
{
    return get_object_element(fc, 0);
}

Value get_fn_call_arg(Value fc, std::uint32_t index)
{
    assert(index < get_fn_call_size(fc));
    return get_object_element(fc, index + 1);
}

Value fn_call_equals(Value l, Value r)
{
    auto size = get_object_size(l);
    if (size != get_object_size(r))
        return nil;
    for (decltype(size) i = 0; i < size; ++i)
        if (get_object_element(l, i) != get_object_element(r, i))
            return nil;
    return TRUE;
}

Force fn_call_pr_str(Value fc)
{
    std::string s = "#FnCall[" + to_string(get_object_element(fc, 0));
    auto size = get_object_size(fc);
    for (decltype(size) i = 1; i < size; ++i)
    {
        s += ' ';
        s += to_string(get_object_element(fc, i));
    }
    return create_string(s + ']');
}

}
