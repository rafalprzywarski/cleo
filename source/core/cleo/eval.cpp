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
#include "fn_call.hpp"
#include "bytecode_fn.hpp"
#include <vector>

namespace cleo
{

Force eval_resolved(Value val, Value env);

namespace
{

Value lookup_not_macro_var(Value sym)
{
    auto var = resolve_var(sym);
    if (is_var_macro(var))
        throw_illegal_state("Can't take value of a macro: " + to_string(var));
    return var;
}

Value symbol_var(Value sym, Value env)
{
    if (env && map_contains(env, sym))
        return nil;
    return maybe_resolve_var(sym);
}

Force eval_symbol(Value sym, Value env)
{
    if (env && map_contains(env, sym))
        return call_multimethod2(*rt::get, env, sym);
    Root msg{create_string("unable to resolve symbol " + to_string(sym))};
    throw_exception(new_symbol_not_found(*msg));
}

Value eval_quote(Value list)
{
    check_arity(QUOTE, 1, get_list_size(list) - 1);
    Root next{get_list_next(list)};
    return get_list_first(*next);
}


Force reverse_list(Value l)
{
    if (get_list_size(l) == 0)
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
    return create_var_value_ref(lookup_not_macro_var(sym));
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

Force resolve_fn_call(Value l, Value env)
{
    auto size = get_list_size(l);
    assert(size > 0);
    Roots roots(size);
    std::vector<Value> vals;
    vals.reserve(size);
    roots.set(0, get_list_first(l) == RECUR ? *recur : resolve_value(get_list_first(l), env));
    vals.push_back(roots[0]);
    l = get_list_next(l);
    for (decltype(size) i = 1; i != size; ++i)
    {
        roots.set(i, resolve_value(get_list_first(l), env));
        vals.push_back(roots[i]);
        l = get_list_next(l);
    }

    return create_fn_call(vals.data(), vals.size());
}

Force resolve_fn_body(Value name, Value body, Value env)
{
    Root lenv{name ? map_assoc(env, name, nil) : env}, ret;
    check_type("fn* parameter list", get_list_first(body), *type::Array);
    lenv = bind_params(get_list_first(body), *lenv);
    ret = resolve_pure_list(get_list_next(body), *lenv);
    return list_conj(*ret, get_list_first(body));
}

Force resolve_list(Value l, Value env)
{
    if (get_list_size(l) == 0)
        return l;

    Root ret;

    auto f = get_list_first(l);
    if (f == QUOTE)
        return l;
    if (f == FN)
    {
        l = get_list_next(l);

        auto name = nil;
        if (l && get_value_tag(get_list_first(l)) == tag::SYMBOL)
        {
            name = get_list_first(l);
            l = get_list_next(l);
        }

        if (l.is_nil())
            ret = *EMPTY_LIST;
        else if (get_value_type(get_list_first(l)).is(*type::List))
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
        Root bindings;
        if (!l || !get_value_type(*(bindings = get_list_first(l))).is(*type::Array))
            throw_illegal_argument(to_string(f) + " requires a vector for its binding");
        Root lenv{env};
        if (get_array_size(*bindings) % 2 == 1)
            throw_illegal_argument(to_string(LET) + " requires an even number of forms in binding vector");
        bindings = resolve_bindings(get_list_first(l), lenv);
        ret = resolve_pure_list(get_list_next(l), *lenv);
        ret = list_conj(*ret, *bindings);
        return list_conj(*ret, f);
    }
    if (f == CATCH)
    {
        l = get_list_next(l);
        if (!l)
            throw_illegal_argument(to_string(f) + " expected a type");
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
        if (!l)
            throw_illegal_argument("Var name missing");
        auto name = get_list_first(l);
        auto meta = nil;
        if (get_value_tag(name) != tag::SYMBOL)
        {
            meta = name;
            l = get_list_next(l);
            if (!l)
                throw_illegal_argument("Var name missing");
            name = get_list_first(l);
        }
        check_type("Symbol name", name, *type::Symbol);
        Root lenv{map_assoc(env, name, nil)};
        ret = resolve_pure_list(get_list_next(l), *lenv);
        ret = list_conj(*ret, name);
        if (meta)
            ret = list_conj(*ret, meta);
        return list_conj(*ret, f);
    }
    if (f == DO || f == IF || f == THROW || f == TRY || f == FINALLY)
    {
        ret = resolve_pure_list(get_list_next(l), env);
        return list_conj(*ret, f);
    }
    if (f == RECUR)
        return resolve_fn_call(l, env);
    if (get_value_tag(f) == tag::SYMBOL && !map_contains(env, f) && is_var_macro(resolve_var(f)))
    {
        Root expanded{macroexpand(l, env)};
        return resolve_value(*expanded, env);
    }

    return resolve_fn_call(l, env);
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

Force eval_fn(Value list, Value env)
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
    Roots body_roots(get_value_type(get_list_first(*next)).is(*type::List) ? get_list_size(*next) : 1);
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
        body_roots.set(bodies.size(), resolve_fn_body(name, params.back(), body, env));
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
        (ns && ns != get_symbol_name(ns_name(*rt::current_ns))))
        throw_illegal_argument("Can't refer to qualified var that doesn't exist: " + to_string(sym));
    next = get_list_next(*next);
    if (*next && get_list_next(*next))
        throw_arity_error(DEF, get_list_size(list) - 1);
    Root val{eval_resolved(!*next ? nil : get_list_first(*next), env)};
    auto current_ns_name = get_symbol_name(ns_name(*rt::current_ns));
    auto sym_name = get_symbol_name(sym);
    sym = create_symbol(
        {get_string_ptr(current_ns_name), get_string_len(current_ns_name)},
        {get_string_ptr(sym_name), get_string_len(sym_name)});
    return define(sym, *val, meta);
}

Force eval_let(Value list, Value env)
{
    check_arity(LET, 2, get_list_size(list) - 1);
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
        Root val{eval_resolved(get_array_elem(bindings, i + 1), *lenv)};
        auto binding = get_array_elem(bindings, i);
        if (!get_value_type(binding).is(*type::Symbol))
            throw_illegal_argument("Bad binding form, expected symbol, got: " + to_string(binding));
        if (get_symbol_namespace(binding))
            throw_illegal_argument("Can't let qualified name: " + to_string(binding));
        lenv = map_assoc(*lenv, binding, *val);
    }
    n = get_list_next(*n);
    return eval_resolved(get_list_first(*n), *lenv);
}

Force eval_do(Value list, Value env)
{
    list = get_list_next(list);
    Root ret;
    while (list)
    {
        ret = eval_resolved(get_list_first(list), env);
        list = get_list_next(list);
    }
    return *ret;
}

Force eval_loop(Value list, Value env)
{
    check_arity(LOOP, 2, get_list_size(list) - 1);
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
        Root val{eval_resolved(get_array_elem(bindings, i + 1), *lenv)};
        auto binding = get_array_elem(bindings, i);
        if (!get_value_type(binding).is(*type::Symbol))
            throw_illegal_argument("Bad binding form, expected symbol, got: " + to_string(binding));
        if (get_symbol_namespace(binding))
            throw_illegal_argument("Can't let qualified name: " + to_string(binding));
        lenv = map_assoc(*lenv, binding, *val);
    }
    n = get_list_next(*n);
    Root val{eval_resolved(get_list_first(*n), *lenv)};
    while (get_value_type(*val).is(*type::Recur))
    {
        check_arity(RECUR, get_array_size(bindings) / 2, get_object_size(*val));
        auto size = get_array_size(bindings) / 2;
        for (decltype(size) i = 0; i != size; ++i)
            lenv = map_assoc(*lenv, get_array_elem(bindings, i * 2), get_object_element(*val, i));

        val = eval_resolved(get_list_first(*n), *lenv);
    }
    return *val;
}

Force eval_if(Value list, Value env)
{
    auto size = get_list_size(list);
    if (size < 3 || size > 4)
        throw_arity_error(IF, size - 1);
    Root n{get_list_next(list)};
    Root cond{eval_resolved(get_list_first(*n), env)};
    n = get_list_next(*n);
    if (*cond)
        return eval_resolved(get_list_first(*n), env);
    n = get_list_next(*n);
    return !*n ? nil : eval_resolved(get_list_first(*n), env);
}

Force eval_throw(Value list, Value env)
{
    check_arity(THROW, 1, get_list_size(list) - 1);
    Root n{get_list_next(list)};
    Root ex{eval_resolved(get_list_first(*n), env)};
    throw_exception(*ex);
}

Force eval_try(Value list, Value env)
{
    check_arity(TRY, 2, get_list_size(list) - 1);
    Root n{get_list_next(list)};
    auto expr = get_list_first(*n);
    n = get_list_next(*n);
    auto clause = get_list_first(*n);
    if (get_list_first(clause).is(FINALLY))
        check_arity(FINALLY, 1, get_list_size(clause) - 1);
    else if (get_list_first(clause).is(CATCH))
    {
        check_arity(CATCH, 3, get_list_size(clause) - 1);
        auto name = get_list_first(get_list_next(get_list_next(clause)));
        if (get_value_tag(name) != tag::SYMBOL || get_symbol_namespace(name))
            throw_illegal_argument("Bad binding form, expected symbol, got: " + to_string(name));
    }
    else
        throw_illegal_argument("Expected " + to_string(CATCH) + " or " + to_string(FINALLY));
    Root val;
    try
    {
        val = eval_resolved(expr, env);
    }
    catch (const Exception& )
    {
        Root ex{catch_exception()};
        if (get_list_first(clause).is(CATCH))
        {
            n = get_list_next(clause);
            auto type = get_list_first(*n);
            type = get_value_type(type).is(*type::VarValueRef) ? get_var_value_ref_value(type) : lookup(type);
            if (!isa(get_value_type(*ex), type))
            {
                throw_exception(*ex);
            }
            n = get_list_next(*n);
            auto name = get_list_first(*n);
            n = get_list_next(*n);
            Root lenv{env ? env : *EMPTY_MAP};
            lenv = map_assoc(*lenv, name, *ex);
            return eval_resolved(get_list_first(*n), *lenv);
        }
        else { // finally
            n = get_list_next(clause);
            eval_resolved(get_list_first(*n), env);
            throw_exception(*ex);
        }
    }
    if (get_list_first(clause).is(FINALLY))
    {
        n = get_list_next(clause);
        eval_resolved(get_list_first(*n), env);
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

std::vector<Value> eval_args(Roots& arg_roots, Value firstVal, Value fc, Value env)
{
    std::vector<Value> elems;
    auto size = get_fn_call_size(fc);
    elems.reserve(size + 1);
    elems.push_back(firstVal);
    for (Roots::size_type i = 0; i < size; ++i)
    {
        arg_roots.set(i, eval_resolved(get_fn_call_arg(fc, i), env));
        elems.push_back(arg_roots[i]);
    }

    return elems;
}

Force call_fn(const Value *elems, std::uint32_t elems_size, std::uint8_t public_n)
{
    const auto fn = elems[0];
    auto fni = find_fn_index(fn, elems_size - 1, public_n);
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
        if (elems_size - 1 > n_fixed_params)
            vargs = create_list(elems + n_fixed_params + 1, elems_size - n_fixed_params - 1);
        fenv = map_assoc(*fenv, get_var_arg(params), *vargs);
    }
    auto body = get_fn_body(fn, fni);
    Root val{eval_resolved(body, *fenv)};
    while (get_value_type(*val).is(*type::Recur))
    {
        check_arity(RECUR, n_params, get_object_size(*val));
        for (decltype(n_fixed_params) i = 0; i < n_fixed_params; ++i)
            fenv = map_assoc(*fenv, get_array_elem(params, i), get_object_element(*val, i));
        if (va)
            fenv = map_assoc(*fenv, get_var_arg(params), get_object_element(*val, n_params - 1));
        val = eval_resolved(body, *fenv);
    }
    return *val;
}

std::pair<Value, Int64> find_bytecode_fn_body(Value fn, std::uint8_t arity, std::uint8_t public_n)
{
    auto n = get_bytecode_fn_size(fn);
    for (decltype(n) i = 0; i < n; ++i)
        if (get_bytecode_fn_arity(fn, i) == arity)
            return {get_bytecode_fn_body(fn, i), arity};
    if (n > 0)
    {
        auto va_arity = get_bytecode_fn_arity(fn, n - 1);
        if (va_arity < 0 && ~va_arity <= arity)
            return {get_bytecode_fn_body(fn, n - 1), va_arity};
    }
    throw_arity_error(get_bytecode_fn_name(fn), public_n);
}

Force call_bytecode_fn(const Value *elems, std::uint32_t elems_size, std::uint8_t public_n)
{
    auto fn = elems[0];
    auto body_and_arity = find_bytecode_fn_body(fn, elems_size - 1, public_n);
    auto body = body_and_arity.first;
    auto arity = body_and_arity.second;
    auto consts = get_bytecode_fn_body_consts(body);
    auto vars = get_bytecode_fn_body_vars(body);
    auto locals_size = get_bytecode_fn_body_locals_size(body);
    auto bytes = get_bytecode_fn_body_bytes(body);
    auto bytes_size = get_bytecode_fn_body_bytes_size(body);
    auto stack_size = stack.size();
    if (arity < 0)
    {
        auto rest = ~arity + 1;
        stack.insert(stack.end(), elems + 1, elems + rest);
        stack_push(rest < elems_size ?  create_array(elems + rest, elems_size - rest) : nil);
    }
    else
        stack.insert(stack.end(), elems + 1, elems + elems_size);
    stack.resize(stack.size() + locals_size, nil);
    vm::eval_bytecode(stack, consts, vars, locals_size, nil, bytes, bytes_size);
    auto result = stack.back();
    stack.resize(stack_size);
    return result;
}

Force call_fn(Value type, const Value *elems, std::uint32_t elems_size, std::uint8_t public_n)
{
    if (type.is(*type::NativeFunction))
        return get_native_function_ptr(elems[0])(elems + 1, elems_size - 1);
    if (type.is(*type::Multimethod))
        return call_multimethod(elems[0], elems + 1, elems_size - 1);
    if (type.is(*type::Fn))
        return call_fn(elems, elems_size, public_n);
    if (type.is(*type::BytecodeFn))
        return call_bytecode_fn(elems, elems_size, public_n);
    if (isa(type, *type::Callable))
        return call_multimethod(*rt::obj_call, elems, elems_size);
    Root msg{create_string("call error " + to_string(type))};
    throw_exception(new_call_error(*msg));
}

Force call_fn(Value type, const Value *elems, std::uint32_t elems_size)
{
    return call_fn(type, elems, elems_size, elems_size - 1);
}

Force eval_list(Value list, Value env)
{
    if (get_list_size(list) == 0)
        return list;
    Value first = get_list_first(list);
    if (first.is(QUOTE))
        return eval_quote(list);
    if (first.is(FN))
        return eval_fn(list, env);
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
    throw_illegal_state("unresolved list");
}

Force eval_fn_call(Value fc, Value env)
{
    Root val{eval_resolved(get_fn_call_fn(fc), env)};
    Roots arg_roots(get_fn_call_size(fc) + 1);
    auto elems = eval_args(arg_roots, *val, fc, env);
    return call_fn(get_value_type(*val), elems.data(), elems.size());
}

Force eval_vector(Value v, Value env)
{
    auto size = get_array_size(v);
    Roots roots(size);
    std::vector<Value> vals;
    vals.reserve(size);
    for (decltype(size) i = 0; i != size; ++i)
    {
        roots.set(i, eval_resolved(get_array_elem(v, i), env));
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
        Root val{eval_resolved(get_array_set_elem(s, i), env)};
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
        k = eval_resolved(get_array_elem(*kv, 0), env);
        v = eval_resolved(get_array_elem(*kv, 1), env);
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

Force macroexpand1(Value form, Value env)
{
    if (!get_value_type(form).is(*type::List) || get_list_size(form) == 0)
        return form;

    Root m{get_list_first(form)};
    if (get_value_tag(*m) != tag::SYMBOL)
        return form;
    if (SPECIAL_SYMBOLS.count(*m))
        return form;
    auto var = symbol_var(*m, env);
    if (!var || !is_var_macro(var))
        return form;
    m = get_var_value(var);
    Value mtype = get_value_type(*m);
    if (!mtype.is(*type::Fn) && !mtype.is(*type::BytecodeFn))
        return form;

    std::vector<Value> elems;
    elems.reserve(get_list_size(form) + 2);
    elems.push_back(*m);
    elems.push_back(form);
    elems.push_back(env);
    for (Root arg_list{get_list_next(form)}; *arg_list; arg_list = get_list_next(*arg_list))
        elems.push_back(get_list_first(*arg_list));

    return call_fn(mtype, elems.data(), elems.size(), elems.size() - 3);
}

Force macroexpand(Value form, Value env)
{
    Root exp{macroexpand1(form, env)};
    if (!exp->is(form))
        return macroexpand(*exp, env);
    return *exp;
}

Force apply(Value fn, Value args)
{
    std::array<Value, 2> vals{{fn, args}};
    return apply(vals.data(), vals.size());
}

Force apply(const Value *vals, std::uint32_t size)
{
    assert(size >= 2);
    auto args = vals[size - 1];
    std::uint32_t len = 0;
    for (Root s{call_multimethod1(*rt::seq, args)}; *s; s = call_multimethod1(*rt::next, *s))
        ++len;
    Roots roots{len};

    std::uint32_t i = 0;
    std::vector<Value> form(vals, vals + (size - 1));
    for (Root s{call_multimethod1(*rt::seq, args)}; *s; s = call_multimethod1(*rt::next, *s))
    {
        roots.set(i, call_multimethod1(*rt::first, *s));
        form.push_back(roots[i]);
        ++i;
    }

    return call_fn(get_value_type(form.front()), form.data(), form.size());
}

Force eval_resolved(Value val, Value env)
{
    if (get_value_tag(val) == tag::SYMBOL)
        return eval_symbol(val, env);
    auto type = get_value_type(val);
    if (type.is(*type::VarValueRef))
        return get_var_value_ref_value(val);
    if (type.is(*type::List))
        return eval_list(val, env);
    if (type.is(*type::FnCall))
        return eval_fn_call(val, env);
    if (type.is(*type::Array))
        return eval_vector(val, env);
    if (type.is(*type::ArraySet))
        return eval_set(val, env);
    if (isa(type, *type::PersistentMap))
        return eval_map(val, env);
    return val;
}

Force call(const Value *vals, std::uint32_t size)
{
    return call_fn(get_value_type(vals[0]), vals, size);
}

Force eval(Value val, Value env)
{
    Root rval{resolve_value(val, env)};
    return eval_resolved(*rval, env);
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
