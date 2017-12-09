#include "eval.hpp"
#include "var.hpp"
#include "list.hpp"
#include "multimethod.hpp"
#include "global.hpp"
#include "error.hpp"
#include <vector>

namespace cleo
{

namespace
{

Value eval_symbol(Value sym)
{
    auto val = lookup(sym);
    if (val == nil)
        throw SymbolNotFound("symbol not found");
    return val;
}

Force eval_list(Value list)
{
    Value first = get_list_first(list);
    if (first == QUOTE)
    {
        Root next{get_list_next(list)};
        return get_list_first(*next);
    }
    Roots arg_roots(get_int64_value(get_list_size(list)));
    std::vector<Value> args;
    args.reserve(get_int64_value(get_list_size(list)));
    Roots::size_type i = 0;
    for (Root arg_list{get_list_next(list)}; *arg_list != nil; arg_list = get_list_next(*arg_list), ++i)
    {
        arg_roots.set(i, eval(get_list_first(*arg_list)));
        args.push_back(arg_roots[i]);
    }
    Root val{eval(first)};
    if (get_value_tag(*val) == tag::NATIVE_FUNCTION)
        return get_native_function_ptr(*val)(args.data(), args.size());
    if (get_value_type(*val) == type::MULTIMETHOD)
        return call_multimethod(*val, args.data(), args.size());
    throw CallError("call error");
}

}

Force eval(Value val)
{
    if (get_value_tag(val) == tag::SYMBOL)
        return eval_symbol(val);
    if (get_value_type(val) == type::LIST)
        return eval_list(val);
    return val;
}

}
