#include "eval.hpp"
#include "var.hpp"
#include "list.hpp"
#include "multimethod.hpp"
#include "fn.hpp"
#include "global.hpp"
#include "error.hpp"
#include "small_vector.hpp"
#include "small_map.hpp"
#include <vector>

namespace cleo
{

namespace
{

Value eval_symbol(Value sym, Value env)
{
    if (env != nil && small_map_contains(env, sym))
        return small_map_get(env, sym);
    return lookup(sym);
}

Value eval_quote(Value list)
{
    Root next{get_list_next(list)};
    return get_list_first(*next);
}

Force eval_fn(Value list, Value env)
{
    Root next{get_list_next(list)};
    auto name = nil;
    if (get_value_tag(get_list_first(*next)) == tag::SYMBOL)
    {
        name = get_list_first(*next);
        next = get_list_next(*next);
    }
    auto params = get_list_first(*next);
    next = get_list_next(*next);
    auto body = get_list_first(*next);
    return create_fn(env, name, params, body);
}

Force eval_def(Value list, Value env)
{
    Root next{get_list_next(list)};
    auto sym = get_list_first(*next);
    next = get_list_next(*next);
    Root val{eval(get_list_first(*next), env)};
    define(sym, *val);
    return *val;
}

Force eval_let(Value list, Value env)
{
    Root n{get_list_next(list)};
    auto bindings = get_list_first(*n);
    auto size = get_small_vector_size(bindings);
    Root lenv{env == nil && size > 0 ? create_small_map() : env};
    for (decltype(size) i = 0; i != size; i += 2)
    {
        Root val{eval(get_small_vector_elem(bindings, i + 1), *lenv)};
        lenv = small_map_assoc(*lenv, get_small_vector_elem(bindings, i), *val);
    }
    n = get_list_next(*n);
    return eval(get_list_first(*n), *lenv);
}

Force eval_loop(Value list, Value env)
{
    Root n{get_list_next(list)};
    auto bindings = get_list_first(*n);
    auto size = get_small_vector_size(bindings);
    Root lenv{env == nil && size > 0 ? create_small_map() : env};
    for (decltype(size) i = 0; i != size; i += 2)
    {
        Root val{eval(get_small_vector_elem(bindings, i + 1), *lenv)};
        lenv = small_map_assoc(*lenv, get_small_vector_elem(bindings, i), *val);
    }
    n = get_list_next(*n);
    Root val{eval(get_list_first(*n), *lenv)};
    while (get_value_type(*val) == type::RECUR)
    {
        auto size = get_small_vector_size(bindings) / 2;
        for (decltype(size) i = 0; i != size; ++i)
            lenv = small_map_assoc(*lenv, get_small_vector_elem(bindings, i * 2), get_object_element(*val, i));

        val = eval(get_list_first(*n), *lenv);
    }
    return *val;
}

Force eval_if(Value list, Value env)
{
    Root n{get_list_next(list)};
    Root cond{eval(get_list_first(*n), env)};
    n = get_list_next(*n);
    if (*cond != nil)
        return eval(get_list_first(*n), env);
    n = get_list_next(*n);
    return *n == nil ? nil : eval(get_list_first(*n), env);
}

Force eval_list(Value list, Value env)
{
    if (get_int64_value(get_list_size(list)) == 0)
        return list;
    Value first = get_list_first(list);
    if (first == QUOTE)
        return eval_quote(list);
    if (first == FN)
        return eval_fn(list, env);
    if (first == DEF)
        return eval_def(list, env);
    if (first == LET)
        return eval_let(list, env);
    if (first == IF)
        return eval_if(list, env);
    if (first == LOOP)
        return eval_loop(list, env);
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
        Root fenv{get_fn_env(*val)};
        if (*fenv == nil && n > 0)
            fenv = create_small_map();
        for (decltype(n) i = 0; i < n; ++i)
            fenv = small_map_assoc(*fenv, get_small_vector_elem(params, i), args[i]);
        auto body = get_fn_body(*val);
        Root val{eval(body, *fenv)};
        while (get_value_type(*val) == type::RECUR)
        {
            for (decltype(n) i = 0; i < n; ++i)
                fenv = small_map_assoc(*fenv, get_small_vector_elem(params, i), get_object_element(*val, i));
            val = eval(body, *fenv);
        }
        return *val;
    }
    throw CallError("call error");
}

Force eval_vector(Value v, Value env)
{
    auto size = get_small_vector_size(v);
    Roots roots(size);
    std::vector<Value> vals;
    vals.reserve(size);
    for (decltype(size) i = 0; i != size; ++i)
    {
        roots.set(i, eval(get_small_vector_elem(v, i), env));
        vals.push_back(roots[i]);
    }

    return create_small_vector(vals.data(), vals.size());
}

Force eval_map(Value m, Value env)
{
    auto size = get_small_map_size(m);
    Root e{create_small_map()};
    for (decltype(size) i = 0; i != size; ++i)
    {
        Root k{eval(get_small_map_key(m, i), env)};
        Root v{eval(get_small_map_val(m, i), env)};
        e = small_map_assoc(*e, *k, *v);
    }

    return *e;
}

}

Force eval(Value val, Value env)
{
    if (get_value_tag(val) == tag::SYMBOL)
        return eval_symbol(val, env);
    auto type = get_value_type(val);
    if (type == type::LIST)
        return eval_list(val, env);
    if (type == type::SMALL_VECTOR)
        return eval_vector(val, env);
    if (type == type::SMALL_MAP)
        return eval_map(val, env);
    return val;
}

}
