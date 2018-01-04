#include "eval.hpp"
#include "var.hpp"
#include "list.hpp"
#include "multimethod.hpp"
#include "fn.hpp"
#include "macro.hpp"
#include "global.hpp"
#include "error.hpp"
#include "small_vector.hpp"
#include "small_set.hpp"
#include "small_map.hpp"
#include "namespace.hpp"
#include "reader.hpp"
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
    if (get_value_type(get_list_first(*next)) == type::SMALL_VECTOR)
    {
        auto params = get_list_first(*next);
        next = get_list_next(*next);
        auto body = get_list_first(*next);
        return create_fn(env, name, params, body);
    }
    std::vector<Value> params, bodies;
    while (*next != nil)
    {
        Root pb{get_list_first(*next)};
        params.push_back(get_list_first(*pb));
        pb = get_list_next(*pb);
        bodies.push_back(get_list_first(*pb));
        next = get_list_next(*next);
    }
    return create_fn(env, name, params.data(), bodies.data(), params.size());
}

Force eval_macro(Value list)
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
    return create_macro(name, params, body);
}

Force eval_def(Value list, Value env)
{
    Root next{get_list_next(list)};
    auto sym = get_list_first(*next);
    auto ns = get_symbol_namespace(sym);
    auto current_ns = lookup_var(CURRENT_NS);
    if (current_ns == nil ||
        (ns != nil && are_equal(ns, get_symbol_name(current_ns)) == nil))
    {
        Root msg{create_string("illegal namespace")};
        throw_exception(new_illegal_argument(*msg));
    }
    next = get_list_next(*next);
    Root val{eval(get_list_first(*next), env)};
    auto current_ns_name = get_symbol_name(current_ns);
    auto sym_name = get_symbol_name(sym);
    sym = create_symbol(
        {get_string_ptr(current_ns_name), get_string_len(current_ns_name)},
        {get_string_ptr(sym_name), get_string_len(sym_name)});
    define(sym, *val);
    return *val;
}

Force eval_let(Value list, Value env)
{
    Root n{get_list_next(list)};
    auto bindings = get_list_first(*n);
    auto size = get_small_vector_size(bindings);
    Root lenv{env == nil && size > 0 ? *EMPTY_MAP : env};
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
    Root lenv{env == nil && size > 0 ? *EMPTY_MAP : env};
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

Force eval_throw(Value list, Value env)
{
    Root n{get_list_next(list)};
    Root ex{eval(get_list_first(*n), env)};
    throw_exception(*ex);
}

Force eval_try(Value list, Value env)
{
    Root n{get_list_next(list)};
    auto expr = get_list_first(*n);
    n = get_list_next(*n);
    auto catch_ = get_list_first(*n);
    Root val;
    try
    {
        val = eval(expr, env);
    }
    catch (const Exception& )
    {
        Root ex{catch_exception()};
        if (get_list_first(catch_) == CATCH)
        {
            n = get_list_next(catch_);
            auto type = get_list_first(*n);
            if (!isa(get_value_type(*ex), type))
            {
                throw_exception(*ex);
            }
            n = get_list_next(*n);
            auto name = get_list_first(*n);
            n = get_list_next(*n);
            Root lenv{env != nil ? env : *EMPTY_MAP};
            lenv = small_map_assoc(*lenv, name, *ex);
            return eval(get_list_first(*n), *lenv);
        }
        else { // finally
            n = get_list_next(catch_);
            eval(get_list_first(*n), env);
            throw_exception(*ex);
        }
    }
    if (get_list_first(catch_) == FINALLY)
    {
        n = get_list_next(catch_);
        eval(get_list_first(*n), env);
    }
    return *val;
}

bool is_va(Value fn, std::uint8_t i)
{
    auto params = get_fn_params(fn, i);
    auto size = get_small_vector_size(params);
    return
        size >= 2 &&
        get_small_vector_elem(params, size - 2) == VA;
}

Value get_var_arg(Value params)
{
    auto size = get_small_vector_size(params);
    return get_small_vector_elem(params, size - 1);
}

std::uint8_t find_fn_index(Value fn, std::uint8_t n)
{
    auto size = get_fn_size(fn);
    for (std::uint8_t i = 0; i < size; ++i)
    {
        auto va = is_va(fn, i);
        auto size = get_small_vector_size(get_fn_params(fn, i));
        if (!va && size == n)
            return i;
    }

    for (std::uint8_t i = 0; i < size; ++i)
    {
        auto va = is_va(fn, i);
        auto size = get_small_vector_size(get_fn_params(fn, i));
        if (va && (size - 2) <= n)
            return i;
    }

    Root msg{create_string("arity error")};
    throw_exception(new_call_error(*msg));
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
    if (first == MACRO)
        return eval_macro(list);
    if (first == DEF)
        return eval_def(list, env);
    if (first == LET)
        return eval_let(list, env);
    if (first == IF)
        return eval_if(list, env);
    if (first == LOOP)
        return eval_loop(list, env);
    if (first == THROW)
        return eval_throw(list, env);
    if (first == TRY)
        return eval_try(list, env);
    Root val{first == RECUR ? *recur : eval(first, env)};
    auto type = get_value_type(*val);
    if (type == type::Macro)
    {
        Root args{get_list_next(list)};
        if (*args == nil)
            args = *EMPTY_LIST;
        Root form{list_conj(*args, *val)};
        Root exp{macroexpand(*form)};
        return eval(*exp, env);
    }
    Roots arg_roots(get_int64_value(get_list_size(list)));
    std::vector<Value> elems;
    elems.reserve(get_int64_value(get_list_size(list)));
    elems.push_back(*val);
    Roots::size_type i = 0;
    for (Root arg_list{get_list_next(list)}; *arg_list != nil; arg_list = get_list_next(*arg_list), ++i)
    {
        arg_roots.set(i, eval(get_list_first(*arg_list), env));
        elems.push_back(arg_roots[i]);
    }
    if (type == type::NATIVE_FUNCTION)
        return get_native_function_ptr(*val)(elems.data() + 1, elems.size() - 1);
    if (type == type::MULTIMETHOD)
        return call_multimethod(*val, elems.data() + 1, elems.size() - 1);
    if (type == type::FN)
    {
        auto fni = find_fn_index(*val, elems.size() - 1);
        auto va = is_va(*val, fni);
        auto params = get_fn_params(*val, fni);
        auto n_params = va ? get_small_vector_size(params) - 1 : get_small_vector_size(params);
        auto n_fixed_params = va ? n_params - 1 : n_params;
        Root fenv{get_fn_env(*val)};
        if (*fenv == nil && (n_params > 0 || va))
            fenv = *EMPTY_MAP;
        for (decltype(n_fixed_params) i = 0; i < n_fixed_params; ++i)
            fenv = small_map_assoc(*fenv, get_small_vector_elem(params, i), elems[i + 1]);
        if (va)
        {
            Root vargs;
            if (elems.size() - 1 > n_fixed_params)
                vargs = create_list(elems.data() + n_fixed_params + 1, elems.size() - n_fixed_params - 1);
            fenv = small_map_assoc(*fenv, get_var_arg(params), *vargs);
        }
        auto body = get_fn_body(*val, fni);
        Root val{eval(body, *fenv)};
        while (get_value_type(*val) == type::RECUR)
        {
            for (decltype(n_fixed_params) i = 0; i < n_fixed_params; ++i)
                fenv = small_map_assoc(*fenv, get_small_vector_elem(params, i), get_object_element(*val, i));
            if (va)
            {
                Root vargs;
                if (get_object_size(*val) > n_fixed_params)
                {
                    vargs = create_list(elems.data() + n_fixed_params + 1, elems.size() - n_fixed_params - 1);
                    for (std::uint8_t i = get_object_size(*val); i > n_fixed_params; --i)
                        vargs = list_conj(*vargs, get_object_element(*val, i - 1));
                }
                fenv = small_map_assoc(*fenv, get_var_arg(params), *vargs);
            }
            val = eval(body, *fenv);
        }
        return *val;
    }
    if (isa(type, type::Callable))
        return call_multimethod(lookup_var(OBJ_CALL), elems.data(), elems.size());
    Root msg{create_string("call error")};
    throw_exception(new_call_error(*msg));
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

Force eval_set(Value s, Value env)
{
    auto size = get_small_set_size(s);
    Root e{create_small_set()};
    for (decltype(size) i = 0; i != size; ++i)
    {
        Root val{eval(get_small_set_elem(s, i), env)};
        e = small_set_conj(*e, *val);
    }
    return *e;
}

Force eval_map(Value m, Value env)
{
    auto size = get_small_map_size(m);
    Root e{*EMPTY_MAP};
    for (decltype(size) i = 0; i != size; ++i)
    {
        Root k{eval(get_small_map_key(m, i), env)};
        Root v{eval(get_small_map_val(m, i), env)};
        e = small_map_assoc(*e, *k, *v);
    }

    return *e;
}

}

Force macroexpand1(Value val)
{
    if (get_value_type(val) != type::LIST || get_list_size(val) == 0)
        return val;

    auto m = get_list_first(val);
    if (get_value_type(m) != type::Macro)
        return val;

    auto params = get_macro_params(m);
    auto n = get_small_vector_size(params);
    Root args{get_list_next(val)};
    if (*args == nil)
        args = *EMPTY_LIST;
    if (get_int64_value(get_list_size(*args)) != n)
    {
        Root msg{create_string("arity error")};
        throw_exception(new_call_error(*msg));
    }
    Root env{*EMPTY_MAP};
    for (decltype(n) i = 0; i < n; ++i)
    {
        env = small_map_assoc(*env, get_small_vector_elem(params, i), get_list_first(*args));
        args = get_list_next(*args);
    }

    return eval(get_macro_body(m), *env);
}

Force macroexpand(Value val)
{
    Root exp{macroexpand1(val)};
    if (*exp != val)
        return macroexpand(*exp);
    return *exp;
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
    if (type == type::SMALL_SET)
        return eval_set(val, env);
    if (type == type::SMALL_MAP)
        return eval_map(val, env);
    return val;
}

Force load(Value source)
{
    Root bindings{small_map_assoc(*EMPTY_MAP, CURRENT_NS, lookup(CURRENT_NS))};
    PushBindingsGuard guard{*bindings};
    Root forms{read_forms(source)};
    forms = small_vector_seq(*forms);
    Root ret;

    while (*forms)
    {
        ret = eval(get_small_vector_seq_first(*forms));
        forms = get_small_vector_seq_next(*forms);
    }

    return *ret;
}

}
