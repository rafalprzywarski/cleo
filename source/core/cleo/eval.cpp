#include "eval.hpp"
#include "var.hpp"
#include "list.hpp"
#include <vector>

namespace cleo
{

namespace
{

Value eval_symbol(Value sym)
{
    auto val = lookup(sym);
    if (val == nil)
        throw symbol_not_found();
    return val;
}

Value eval_list(Value list)
{
    auto val = lookup(get_list_first(list));
    if (get_value_tag(val) != tag::NATIVE_FUNCTION)
        throw call_error();
    std::vector<Value> args;
    args.reserve(get_int64_value(get_list_size(list)));
    for (list = get_list_next(list); list != nil; list = get_list_next(list))
        args.push_back(eval(get_list_first(list)));
    return get_native_function_ptr(val)(args.data(), args.size());
}

}

Value eval(Value val)
{
    if (get_value_tag(val) == tag::SYMBOL)
        return eval_symbol(val);
    if (get_value_type(val) == type::LIST)
        return eval_list(val);
    return val;
}

}
