#include "eval.hpp"
#include "var.hpp"
#include "list.hpp"
#include "multimethod.hpp"
#include "fn.hpp"
#include "global.hpp"
#include "error.hpp"
#include "array.hpp"
#include "array_set.hpp"
#include "namespace.hpp"
#include "reader.hpp"
#include "util.hpp"
#include <vector>

namespace cleo
{

namespace
{

Value symbol_var(Value sym, Value env)
{
    if (env && map_contains(env, sym))
        return nil;
    return lookup_var_or_nil(resolve(sym));
}

Force eval_symbol(Value sym, Value env)
{
    if (env && map_contains(env, sym))
        return call_multimethod2(*rt::get, env, sym);
    return lookup(sym);
}

Value eval_quote(Value list)
{
    check_arity(QUOTE, 1, get_int64_value(get_list_size(list)) - 1);
    Root next{get_list_next(list)};
    return get_list_first(*next);
}


Force reverse_list(Value l)
{
    if (get_int64_value(get_list_size(l)) == 0)
        return l;
    Root ret{*EMPTY_LIST};
    for (Root f{l}; *f; f = get_list_next(*f))
        ret = list_conj(*ret, get_list_first(*f));
    return *ret;
}

Force bind_params(Value params, Value env)
{
    Root lenv{env ? env : *EMPTY_MAP};
    auto n = get_array_size(params);
    for (decltype(n) i = 0; i < n; ++i)
        lenv = map_assoc(*lenv, get_array_elem(params, i), nil);
    return *lenv;
}

Force resolve_symbol(Value sym, Value env)
{
    if (map_contains(env, sym))
        return sym;
    auto var = lookup_var(resolve(sym));
    if (get_value_type(get_var_value(var)).is(*type::Macro))
        throw_illegal_state("Can't take value of a macro: " + to_string(var));
    return get_var_name(var);
}

Force resolve_bindings(Value b, Root& env)
{
    auto size = get_array_size(b);
    Roots roots(size);
    std::vector<Value> vals;
    vals.reserve(size);
    for (decltype(size) i = 0; (i + 1) < size; i += 2)
    {
        auto name = get_array_elem(b, i);
        roots.set(i, name);
        roots.set(i + 1, resolve_value(get_array_elem(b, i + 1), *env));
        env = map_assoc(*env, name, nil);
        vals.push_back(roots[i]);
        vals.push_back(roots[i + 1]);
    }

    return create_array(vals.data(), vals.size());
}

Force resolve_pure_list(Value l, Value env)
{
    Root ret{*EMPTY_LIST}, val;
    for (; l; l = get_list_next(l))
    {
        val = resolve_value(get_list_first(l), env);
        ret = list_conj(*ret, *val);
    }

    return reverse_list(*ret);
}

Force resolve_fn_body(Value name, Value body, Value env)
{
    Root lenv{name ? map_assoc(env, name, nil) : env}, ret;
    lenv = bind_params(get_list_first(body), *lenv);
    ret = resolve_pure_list(get_list_next(body), *lenv);
    return list_conj(*ret, get_list_first(body));
}

Force resolve_list(Value l, Value env)
{
    if (get_int64_value(get_list_size(l)) == 0)
        return *EMPTY_LIST;

    Root ret;

    auto f = get_list_first(l);
    if (f == QUOTE)
        return l;
    if (f == MACRO)
        throw_illegal_state("Cannot define a macro inside fn");
    if (f == FN)
    {
        l = get_list_next(l);

        auto name = nil;
        if (get_value_tag(get_list_first(l)) == tag::SYMBOL)
        {
            name = get_list_first(l);
            l = get_list_next(l);
        }

        if (get_value_type(get_list_first(l)).is(*type::List))
        {
            ret = *EMPTY_LIST;
            Root val;
            for (; l; l = get_list_next(l))
            {
                val = resolve_fn_body(name, get_list_first(l), env);
                ret = list_conj(*ret, *val);
            }

            ret = reverse_list(*ret);
        }
        else
            ret = resolve_fn_body(name, l, env);

        if (name)
            ret = list_conj(*ret, name);
        return list_conj(*ret, f);
    }
    if (f == LET || f == LOOP)
    {
        l = get_list_next(l);
        Root lenv{env};
        Root bindings{resolve_bindings(get_list_first(l), lenv)};
        ret = resolve_pure_list(get_list_next(l), *lenv);
        ret = list_conj(*ret, *bindings);
        return list_conj(*ret, f);
    }
    if (f == CATCH)
    {
        l = get_list_next(l);
        auto type = get_list_first(l);
        Root resolved_type{resolve_value(type, env)};
        l = get_list_next(l);
        Root lenv{map_assoc(env, get_list_first(l), nil)};
        ret = resolve_pure_list(l, *lenv);
        ret = list_conj(*ret, *resolved_type);
        return list_conj(*ret, f);
    }
    if (f == DEF)
    {
        l = get_list_next(l);
        auto name = get_list_first(l);
        auto meta = nil;
        if (get_value_tag(name) != tag::SYMBOL)
        {
            meta = name;
            l = get_list_next(l);
            name = get_list_first(l);
        }
        Root lenv{map_assoc(env, name, nil)};
        ret = resolve_pure_list(get_list_next(l), *lenv);
        ret = list_conj(*ret, name);
        if (meta)
            ret = list_conj(*ret, meta);
        return list_conj(*ret, f);
    }
    if (f == DO || f == IF || f == RECUR || f == THROW || f == TRY || f == FINALLY)
    {
        ret = resolve_pure_list(get_list_next(l), env);
        return list_conj(*ret, f);
    }
    if (get_value_tag(f) == tag::SYMBOL && !map_contains(env, f) && get_value_type(lookup(f)).is(*type::Macro))
    {
        Root expanded{macroexpand(l, env)};
        return resolve_value(*expanded, env);
    }

    return resolve_pure_list(l, env);
}

Force resolve_vector(Value v, Value env)
{
    auto size = get_array_size(v);
    Roots roots(size);
    std::vector<Value> vals;
    vals.reserve(size);
    for (decltype(size) i = 0; i != size; ++i)
    {
        roots.set(i, resolve_value(get_array_elem(v, i), env));
        vals.push_back(roots[i]);
    }

    return create_array(vals.data(), vals.size());
}

Force resolve_set(Value s, Value env)
{
    auto size = get_array_set_size(s);
    Root e{create_array_set()};
    for (decltype(size) i = 0; i != size; ++i)
    {
        Root val{resolve_value(get_array_set_elem(s, i), env)};
        e = array_set_conj(*e, *val);
    }
    return *e;
}

Force resolve_map(Value val, Value env)
{
    Root e{*EMPTY_MAP}, kv, k, v;
    for (Root seq{call_multimethod1(*rt::seq, val)}; *seq; seq = call_multimethod1(*rt::next, *seq))
    {
        kv = call_multimethod1(*rt::first, *seq);
        k = resolve_value(get_array_elem(*kv, 0), env);
        v = resolve_value(get_array_elem(*kv, 1), env);
        e = map_assoc(*e, *k, *v);
    }

    return *e;
}

Force resolve_fn_body(Value name, Value params, Value body, Value env)
{
    Root lenv{name ? map_assoc(env, name, nil) : env};
    lenv = bind_params(params, *lenv);
    return resolve_value(body, *lenv);
}

template <typename CreateFn>
Force eval_fn(Value list, Value env, Value penv, CreateFn create_fn)
{
    Root next{get_list_next(list)};
    auto name = nil;
    if (*next && get_value_tag(get_list_first(*next)) == tag::SYMBOL)
    {
        name = get_list_first(*next);
        next = get_list_next(*next);
    }
    if (!*next)
        return create_fn(env, name, nullptr, nullptr, 0);
    std::vector<Value> params, bodies;
    Roots body_roots(get_value_type(get_list_first(*next)).is(*type::List) ? get_int64_value(get_list_size(*next)) : 1);
    auto parse_fn = [&](Value list)
    {
        params.push_back(get_list_first(list));
        check_type("fn* parameter list", params.back(), *type::Array);
        auto n = get_array_size(params.back());
        for (decltype(n) i = 0; i < n; ++i)
        {
            auto p = get_array_elem(params.back(), i);
            check_type("fn* parameters", p, *type::Symbol);
            if (get_symbol_namespace(p))
                throw_illegal_argument("Can't use qualified name as parameter: " + to_string(p));
        }
        auto body_list = get_list_next(list);
        auto body = body_list ? get_list_first(body_list) : nil;
        body_roots.set(bodies.size(), resolve_fn_body(name, params.back(), body, penv));
        bodies.push_back(body_roots[bodies.size()]);
        if (body_list && get_list_next(body_list))
            throw_illegal_argument("fn* can contain only one expression");
    };

    if (get_value_type(get_list_first(*next)).is(*type::List))
        for (; *next; next = get_list_next(*next))
            parse_fn(get_list_first(*next));
    else
        parse_fn(*next);
    return create_fn(env, name, params.data(), bodies.data(), params.size());
}

Force eval_fn(Value list, Value env)
{
    return eval_fn(list, env, env, static_cast<Force(*)(Value, Value, const Value*, const Value*, std::uint8_t)>(create_fn));
}

Force eval_macro(Value list, Value env)
{
    Root penv{env ? env : *EMPTY_MAP};
    penv = map_assoc(*penv, FORM, nil);
    penv = map_assoc(*penv, ENV, nil);
    return eval_fn(list, env, *penv, static_cast<Force(*)(Value, Value, const Value*, const Value*, std::uint8_t)>(create_macro));
}

Force eval_def(Value list, Value env)
{
    Root next{get_list_next(list)};
    if (!*next)
        throw_arity_error(DEF, 0);
    auto meta = nil;
    auto sym = get_list_first(*next);
    if (get_value_type(sym).is(*type::PersistentHashMap))
    {
        meta = sym;
        next = get_list_next(*next);
        if (!*next)
            throw_illegal_argument("Var name missing");
        sym = get_list_first(*next);
    }
    check_type("Symbol name", sym, *type::Symbol);
    auto ns = get_symbol_namespace(sym);
    if (!*rt::current_ns ||
        (ns && ns != get_symbol_name(*rt::current_ns)))
        throw_illegal_argument("Can't refer to qualified var that doesn't exist: " + to_string(sym));
    next = get_list_next(*next);
    if (*next && get_list_next(*next))
        throw_arity_error(DEF, get_int64_value(get_list_size(list)) - 1);
    Root val{eval(!*next ? nil : get_list_first(*next), env)};
    auto current_ns_name = get_symbol_name(*rt::current_ns);
    auto sym_name = get_symbol_name(sym);
    sym = create_symbol(
        {get_string_ptr(current_ns_name), get_string_len(current_ns_name)},
        {get_string_ptr(sym_name), get_string_len(sym_name)});
    return define(sym, *val, meta);
}

Force eval_let(Value list, Value env)
{
    check_arity(LET, 2, get_int64_value(get_list_size(list)) - 1);
    Root n{get_list_next(list)};
    auto bindings = get_list_first(*n);
    if (!get_value_type(bindings).is(*type::Array))
        throw_illegal_argument(to_string(LET) + " requires a vector for its binding");
    auto size = get_array_size(bindings);
    if (size % 2 == 1)
        throw_illegal_argument(to_string(LET) + " requires an even number of forms in binding vector");
    Root lenv{!env && size > 0 ? *EMPTY_MAP : env};
    for (decltype(size) i = 0; i != size; i += 2)
    {
        Root val{eval(get_array_elem(bindings, i + 1), *lenv)};
        auto binding = get_array_elem(bindings, i);
        if (!get_value_type(binding).is(*type::Symbol))
            throw_illegal_argument("Bad binding form, expected symbol, got: " + to_string(binding));
        if (get_symbol_namespace(binding))
            throw_illegal_argument("Can't let qualified name: " + to_string(binding));
        lenv = map_assoc(*lenv, binding, *val);
    }
    n = get_list_next(*n);
    return eval(get_list_first(*n), *lenv);
}

Force eval_do(Value list, Value env)
{
    list = get_list_next(list);
    Root ret;
    while (list)
    {
        ret = eval(get_list_first(list), env);
        list = get_list_next(list);
    }
    return *ret;
}

Force eval_loop(Value list, Value env)
{
    check_arity(LOOP, 2, get_int64_value(get_list_size(list)) - 1);
    Root n{get_list_next(list)};
    auto bindings = get_list_first(*n);
    if (!get_value_type(bindings).is(*type::Array))
        throw_illegal_argument(to_string(LOOP) + " requires a vector for its binding");
    auto size = get_array_size(bindings);
    if (size % 2 == 1)
        throw_illegal_argument(to_string(LOOP) + " requires an even number of forms in binding vector");
    Root lenv{!env && size > 0 ? *EMPTY_MAP : env};
    for (decltype(size) i = 0; i != size; i += 2)
    {
        Root val{eval(get_array_elem(bindings, i + 1), *lenv)};
        auto binding = get_array_elem(bindings, i);
        if (!get_value_type(binding).is(*type::Symbol))
            throw_illegal_argument("Bad binding form, expected symbol, got: " + to_string(binding));
        if (get_symbol_namespace(binding))
            throw_illegal_argument("Can't let qualified name: " + to_string(binding));
        lenv = map_assoc(*lenv, binding, *val);
    }
    n = get_list_next(*n);
    Root val{eval(get_list_first(*n), *lenv)};
    while (get_value_type(*val).is(*type::Recur))
    {
        check_arity(RECUR, get_array_size(bindings) / 2, get_object_size(*val));
        auto size = get_array_size(bindings) / 2;
        for (decltype(size) i = 0; i != size; ++i)
            lenv = map_assoc(*lenv, get_array_elem(bindings, i * 2), get_object_element(*val, i));

        val = eval(get_list_first(*n), *lenv);
    }
    return *val;
}

Force eval_if(Value list, Value env)
{
    auto size = get_int64_value(get_list_size(list));
    if (size < 3 || size > 4)
        throw_arity_error(IF, size - 1);
    Root n{get_list_next(list)};
    Root cond{eval(get_list_first(*n), env)};
    n = get_list_next(*n);
    if (*cond)
        return eval(get_list_first(*n), env);
    n = get_list_next(*n);
    return !*n ? nil : eval(get_list_first(*n), env);
}

Force eval_throw(Value list, Value env)
{
    check_arity(THROW, 1, get_int64_value(get_list_size(list)) - 1);
    Root n{get_list_next(list)};
    Root ex{eval(get_list_first(*n), env)};
    throw_exception(*ex);
}

Force eval_try(Value list, Value env)
{
    check_arity(TRY, 2, get_int64_value(get_list_size(list)) - 1);
    Root n{get_list_next(list)};
    auto expr = get_list_first(*n);
    n = get_list_next(*n);
    auto clause = get_list_first(*n);
    if (get_list_first(clause).is(FINALLY))
        check_arity(FINALLY, 1, get_int64_value(get_list_size(clause)) - 1);
    else if (get_list_first(clause).is(CATCH))
    {
        check_arity(CATCH, 3, get_int64_value(get_list_size(clause)) - 1);
        auto name = get_list_first(get_list_next(get_list_next(clause)));
        if (get_value_tag(name) != tag::SYMBOL || get_symbol_namespace(name))
            throw_illegal_argument("Bad binding form, expected symbol, got: " + to_string(name));
    }
    else
        throw_illegal_argument("Expected " + to_string(CATCH) + " or " + to_string(FINALLY));
    Root val;
    try
    {
        val = eval(expr, env);
    }
    catch (const Exception& )
    {
        Root ex{catch_exception()};
        if (get_list_first(clause).is(CATCH))
        {
            n = get_list_next(clause);
            auto type = lookup(get_list_first(*n));
            if (!isa(get_value_type(*ex), type))
            {
                throw_exception(*ex);
            }
            n = get_list_next(*n);
            auto name = get_list_first(*n);
            n = get_list_next(*n);
            Root lenv{env ? env : *EMPTY_MAP};
            lenv = map_assoc(*lenv, name, *ex);
            return eval(get_list_first(*n), *lenv);
        }
        else { // finally
            n = get_list_next(clause);
            eval(get_list_first(*n), env);
            throw_exception(*ex);
        }
    }
    if (get_list_first(clause).is(FINALLY))
    {
        n = get_list_next(clause);
        eval(get_list_first(*n), env);
    }
    return *val;
}

bool is_va(Value fn, std::uint8_t i)
{
    auto params = get_fn_params(fn, i);
    auto size = get_array_size(params);
    return
        size >= 2 &&
        get_array_elem(params, size - 2).is(VA);
}

Value get_var_arg(Value params)
{
    auto size = get_array_size(params);
    return get_array_elem(params, size - 1);
}

std::uint8_t find_fn_index(Value fn, std::uint8_t n, std::uint8_t public_n)
{
    auto size = get_fn_size(fn);
    for (std::uint8_t i = 0; i < size; ++i)
    {
        auto va = is_va(fn, i);
        auto size = get_array_size(get_fn_params(fn, i));
        if (!va && size == n)
            return i;
    }

    for (std::uint8_t i = 0; i < size; ++i)
    {
        auto va = is_va(fn, i);
        auto size = get_array_size(get_fn_params(fn, i));
        if (va && (size - 2) <= n)
            return i;
    }

    throw_arity_error(get_fn_name(fn), public_n);
}

std::vector<Value> eval_args(Roots& arg_roots, Value firstVal, Value list, Value env)
{
    std::vector<Value> elems;
    elems.reserve(get_int64_value(get_list_size(list)));
    elems.push_back(firstVal);
    Roots::size_type i = 0;
    for (Root arg_list{get_list_next(list)}; *arg_list; arg_list = get_list_next(*arg_list), ++i)
    {
        arg_roots.set(i, eval(get_list_first(*arg_list), env));
        elems.push_back(arg_roots[i]);
    }

    return elems;
}

Force call_fn(const std::vector<Value>& elems, std::uint8_t public_n)
{
    const auto fn = elems[0];
    auto fni = find_fn_index(fn, elems.size() - 1, public_n);
    auto va = is_va(fn, fni);
    auto params = get_fn_params(fn, fni);
    auto n_params = va ? get_array_size(params) - 1 : get_array_size(params);
    auto n_fixed_params = va ? n_params - 1 : n_params;
    Root fenv{get_fn_env(fn)};
    if (!*fenv && (n_params > 0 || va))
        fenv = *EMPTY_MAP;
    for (decltype(n_fixed_params) i = 0; i < n_fixed_params; ++i)
        fenv = map_assoc(*fenv, get_array_elem(params, i), elems[i + 1]);
    if (va)
    {
        Root vargs;
        if (elems.size() - 1 > n_fixed_params)
            vargs = create_list(elems.data() + n_fixed_params + 1, elems.size() - n_fixed_params - 1);
        fenv = map_assoc(*fenv, get_var_arg(params), *vargs);
    }
    auto body = get_fn_body(fn, fni);
    Root val{eval(body, *fenv)};
    while (get_value_type(*val).is(*type::Recur))
    {
        check_arity(RECUR, n_params, get_object_size(*val));
        for (decltype(n_fixed_params) i = 0; i < n_fixed_params; ++i)
            fenv = map_assoc(*fenv, get_array_elem(params, i), get_object_element(*val, i));
        if (va)
            fenv = map_assoc(*fenv, get_var_arg(params), get_object_element(*val, n_params - 1));
        val = eval(body, *fenv);
    }
    return *val;
}

Force call_macro(Value fn, Value list, Value env)
{
    Root args{get_list_next(list)};
    if (!*args)
        args = *EMPTY_LIST;
    Root form{list_conj(*args, fn)};
    Root exp{macroexpand(*form, list, env)};
    return eval(*exp, env);
}

Force call_fn(Value type, const std::vector<Value>& elems)
{
    if (type.is(*type::NativeFunction))
        return get_native_function_ptr(elems.front())(elems.data() + 1, elems.size() - 1);
    if (type.is(*type::Multimethod))
        return call_multimethod(elems.front(), elems.data() + 1, elems.size() - 1);
    if (type.is(*type::Fn))
        return call_fn(elems, elems.size() - 1);
    if (isa(type, *type::Callable))
        return call_multimethod(*rt::obj_call, elems.data(), elems.size());
    Root msg{create_string("call error " + to_string(type))};
    throw_exception(new_call_error(*msg));
}

Force eval_list(Value list, Value env)
{
    if (get_int64_value(get_list_size(list)) == 0)
        return list;
    Value first = get_list_first(list);
    if (first.is(QUOTE))
        return eval_quote(list);
    if (first.is(FN))
        return eval_fn(list, env);
    if (first.is(MACRO))
        return eval_macro(list, env);
    if (first.is(DEF))
        return eval_def(list, env);
    if (first.is(LET))
        return eval_let(list, env);
    if (first.is(DO))
        return eval_do(list, env);
    if (first.is(IF))
        return eval_if(list, env);
    if (first.is(LOOP))
        return eval_loop(list, env);
    if (first.is(THROW))
        return eval_throw(list, env);
    if (first.is(TRY))
        return eval_try(list, env);
    if (get_value_tag(first) == tag::SYMBOL)
    {
        auto var = nil;
        try
        {
            var = lookup_var(resolve(first));
        }
        catch (cleo::Exception& ) { }
        if (var && is_var_macro(var))
            return call_macro(first, list, env);
    }
    Root val{first.is(RECUR) ? *recur : eval(first, env)};
    auto type = get_value_type(*val);
    if (type.is(*type::Macro))
        return call_macro(*val, list, env);
    Roots arg_roots(get_int64_value(get_list_size(list)));
    auto elems = eval_args(arg_roots, *val, list, env);
    return call_fn(type, elems);
}

Force eval_vector(Value v, Value env)
{
    auto size = get_array_size(v);
    Roots roots(size);
    std::vector<Value> vals;
    vals.reserve(size);
    for (decltype(size) i = 0; i != size; ++i)
    {
        roots.set(i, eval(get_array_elem(v, i), env));
        vals.push_back(roots[i]);
    }

    return create_array(vals.data(), vals.size());
}

Force eval_set(Value s, Value env)
{
    auto size = get_array_set_size(s);
    Root e{create_array_set()};
    for (decltype(size) i = 0; i != size; ++i)
    {
        Root val{eval(get_array_set_elem(s, i), env)};
        e = array_set_conj(*e, *val);
    }
    return *e;
}

Force eval_map(Value m, Value env)
{
    Root e{*EMPTY_MAP}, kv, k, v;
    for (Root seq{call_multimethod1(*rt::seq, m)}; *seq; seq = call_multimethod1(*rt::next, *seq))
    {
        kv = call_multimethod1(*rt::first, *seq);
        k = eval(get_array_elem(*kv, 0), env);
        v = eval(get_array_elem(*kv, 1), env);
        e = map_assoc(*e, *k, *v);
    }

    return *e;
}

}

Force resolve_value(Value val, Value env)
{
    auto type = get_value_type(val);
    if (type.is(*type::Symbol))
        return resolve_symbol(val, env);
    if (type.is(*type::List))
        return resolve_list(val, env);
    if (type.is(*type::Array))
        return resolve_vector(val, env);
    if (type.is(*type::ArraySet))
        return resolve_set(val, env);
    if (isa(type, *type::PersistentMap))
        return resolve_map(val, env);
    return val;
}

Force macroexpand1(Value val, Value form, Value env)
{
    if (!get_value_type(val).is(*type::List) || get_int64_value(get_list_size(val)) == 0)
        return val;

    Root m{get_list_first(val)};
    if (get_value_tag(*m) == tag::SYMBOL)
    {
        if (SPECIAL_SYMBOLS.count(*m))
            return val;
        auto var = symbol_var(*m, env);
        if (!var)
            return val;
        m = get_var_value(var);
        if (!(is_var_macro(var) && get_value_type(*m).is(*type::Fn)) && !get_value_type(*m).is(*type::Macro))
            return val;
    }
    else if (!get_value_type(*m).is(*type::Macro))
        return val;

    std::vector<Value> elems;
    elems.reserve(get_int64_value(get_list_size(val)) + 2);
    elems.push_back(*m);
    elems.push_back(form);
    elems.push_back(env);
    for (Root arg_list{get_list_next(val)}; *arg_list; arg_list = get_list_next(*arg_list))
        elems.push_back(get_list_first(*arg_list));

    if (get_value_type(*m).is(*type::Fn))
        return call_fn(elems, elems.size() - 1);
    return call_fn(elems, elems.size() - 3);
}

Force macroexpand(Value val, Value form, Value env)
{
    Root exp{macroexpand1(val, form, env)};
    if (!exp->is(val))
        return macroexpand(*exp, *exp, env);
    return *exp;
}

Force apply(Value fn, Value args)
{
    std::uint32_t len = 0;
    for (Root s{call_multimethod1(*rt::seq, args)}; *s; s = call_multimethod1(*rt::next, *s))
        ++len;
    Roots roots{len};

    std::uint32_t i = 0;
    std::vector<Value> form;
    form.push_back(fn);
    for (Root s{call_multimethod1(*rt::seq, args)}; *s; s = call_multimethod1(*rt::next, *s))
    {
        roots.set(i, call_multimethod1(*rt::first, *s));
        form.push_back(roots[i]);
        ++i;
    }

    return call_fn(get_value_type(form.front()), form);
}

Force eval(Value val, Value env)
{
    if (get_value_tag(val) == tag::SYMBOL)
        return eval_symbol(val, env);
    auto type = get_value_type(val);
    if (type.is(*type::List))
        return eval_list(val, env);
    if (type.is(*type::Array))
        return eval_vector(val, env);
    if (type.is(*type::ArraySet))
        return eval_set(val, env);
    if (isa(type, *type::PersistentMap))
        return eval_map(val, env);
    return val;
}

Force load(Value source)
{
    if (get_value_tag(source) != tag::STRING)
        throw_illegal_argument("expected a string");

    Root bindings{map_assoc(*EMPTY_MAP, CURRENT_NS, *rt::current_ns)};
    PushBindingsGuard guard{*bindings};
    ReaderStream stream{source};
    Root form, ret;

    while (!stream.eos())
    {
        form = read(stream);
        ret = eval(*form);
    }

    return *ret;
}

}
