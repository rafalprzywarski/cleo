#include "eval.hpp"
#include "var.hpp"
#include "list.hpp"
#include "multimethod.hpp"
#include "fn.hpp"
#include "global.hpp"
#include "error.hpp"
#include "small_vector.hpp"
#include <vector>

namespace cleo
{

namespace
{

Value eval_symbol(Value sym, const Environment& env)
{
    auto it = env.find(sym);
    if (it != end(env))
        return it->second;
    return lookup(sym);
}

Force eval_list(Value list, const Environment& env)
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
        arg_roots.set(i, eval(get_list_first(*arg_list), env));
        args.push_back(arg_roots[i]);
    }
    Root val{eval(first, env)};
    auto type = get_value_type(*val);
    if (type == type::NATIVE_FUNCTION)
        return get_native_function_ptr(*val)(args.data(), args.size());
    if (type == type::MULTIMETHOD)
        return call_multimethod(*val, args.data(), args.size());
    if (type == type::FN)
    {
        auto params = get_fn_params(*val);
        auto n = get_small_vector_size(params);
        Environment env;
        for (decltype(n) i = 0; i < n; ++i)
            env[get_small_vector_elem(params, i)] = args[i];
        return eval(get_fn_body(*val), env);
    }
    throw CallError("call error");
}

}

Force eval(Value val, const Environment& env)
{
    if (get_value_tag(val) == tag::SYMBOL)
        return eval_symbol(val, env);
    if (get_value_type(val) == type::LIST)
        return eval_list(val, env);
    return val;
}

}
