#include "eval.hpp"
#include "var.hpp"
#include "list.hpp"
#include "multimethod.hpp"
#include "fn.hpp"
#include "global.hpp"
#include "error.hpp"
#include "small_vector.hpp"
#include "small_set.hpp"
#include "small_map.hpp"
#include "namespace.hpp"
#include "reader.hpp"
#include "util.hpp"
#include <vector>

namespace cleo
{

namespace
{

Value eval_symbol(Value sym, Value env)
{
    if (env != nil)
    {
        if (small_map_contains(env, sym))
            return small_map_get(env, sym);
        if (small_map_contains(env, ENV_NS))
            return lookup(small_map_get(env, ENV_NS), sym);
    }
    return lookup(sym);
}

Value eval_quote(Value list)
{
    Root next{get_list_next(list)};
    return get_list_first(*next);
}

Force syntax_quote_val(Root& generated, Value val, Value env);

bool is_unquote_splicing(Value val)
{
    return
        get_value_type(val) == type::List &&
        get_int64_value(get_list_size(val)) == 2 &&
        get_list_first(val) == UNQUOTE_SPLICING;
}

bool is_generating(Value sym)
{
    auto name = get_symbol_name(sym);
    return
        get_symbol_namespace(sym) == nil &&
        get_string_len(name) > 0 &&
        get_string_ptr(name)[get_string_len(name) - 1] == '#';
}

Force generate_symbol(Root& generated, Value sym)
{
    auto found = small_map_get(*generated, sym);
    if (found != nil)
        return found;
    auto name = get_symbol_name(sym);
    auto id = gen_id();
    Root g{create_symbol(std::string(get_string_ptr(name), get_string_len(name) - 1) + "__" + std::to_string(id) + "__auto__")};
    generated = small_map_assoc(*generated, sym, *g);
    return *g;
}

Force syntax_quote_symbol(Root& generated, Value sym, Value env)
{
    if (SPECIAL_SYMBOLS.count(sym))
        return sym;
    if (is_generating(sym))
        return generate_symbol(generated, sym);
    auto ns = (env != nil && small_map_contains(env, ENV_NS)) ? small_map_get(env, ENV_NS) : lookup_var(CURRENT_NS);
    sym = resolve(ns, sym);
    if (get_symbol_namespace(sym) != nil)
        return sym;
    auto sym_ns = get_symbol_name(ns);
    auto sym_name = get_symbol_name(sym);
    return create_symbol({get_string_ptr(sym_ns), get_string_len(sym_ns)}, {get_string_ptr(sym_name), get_string_len(sym_name)});
}

Force syntax_quote_vector(Root& generated, Value v, Value env)
{
    auto seq = lookup(SEQ);
    auto first = lookup(FIRST);
    auto next = lookup(NEXT);
    Root ret{*EMPTY_VECTOR}, val;
    auto size = get_small_vector_size(v);
    for (decltype(size) i = 0; i < size; ++i)
    {
        auto elem = get_small_vector_elem(v, i);
        if (is_unquote_splicing(elem))
        {
            Root s{eval(get_list_first(get_list_next(elem)), env)}, val;
            for (s = call_multimethod1(seq, *s); *s != nil; s = call_multimethod1(next, *s))
            {
                val = call_multimethod1(first, *s);
                ret = small_vector_conj(*ret, *val);
            }
        }
        else
        {
            val = syntax_quote_val(generated, elem, env);
            ret = small_vector_conj(*ret, *val);
        }
    }
    return *ret;
}

Force reverse_list(Value l)
{
    if (get_int64_value(get_list_size(l)) == 0)
        return l;
    Root ret{*EMPTY_LIST};
    for (Root f{l}; *f != nil; f = get_list_next(*f))
        ret = list_conj(*ret, get_list_first(*f));
    return *ret;
}

Force syntax_quote_list(Root& generated, Value l, Value env)
{
    auto seq = lookup(SEQ);
    auto first = lookup(FIRST);
    auto next = lookup(NEXT);
    auto size = get_int64_value(get_list_size(l));
    if (size == 0)
        return *EMPTY_LIST;
    if (get_list_first(l) == UNQUOTE)
    {
        if (get_int64_value(get_list_size(l)) != 2)
        {
            Root msg{create_string("syntax-quote requires exactly 1 argument")};
            throw_exception(new_illegal_argument(*msg));
        }
        return eval(get_list_first(get_list_next(l)), env);
    }
    Root ret{*EMPTY_LIST}, val;
    for (; l != nil; l = get_list_next(l))
    {
        auto elem = get_list_first(l);
        if (is_unquote_splicing(elem))
        {
            Root s{eval(get_list_first(get_list_next(elem)), env)}, val;
            for (s = call_multimethod1(seq, *s); *s != nil; s = call_multimethod1(next, *s))
            {
                val = call_multimethod1(first, *s);
                ret = list_conj(*ret, *val);
            }
        }
        else
        {
            val = syntax_quote_val(generated, elem, env);
            ret = list_conj(*ret, *val);
        }
    }

    return reverse_list(*ret);
}

Force syntax_quote_set(Root& generated, Value s, Value env)
{
    auto seq = lookup(SEQ);
    auto first = lookup(FIRST);
    auto next = lookup(NEXT);
    Root ret{*EMPTY_SET}, val;
    auto size = get_small_set_size(s);
    for (decltype(size) i = 0; i < size; ++i)
    {
        auto elem = get_small_set_elem(s, i);
        if (is_unquote_splicing(elem))
        {
            Root s{eval(get_list_first(get_list_next(elem)), env)}, val;
            for (s = call_multimethod1(seq, *s); *s != nil; s = call_multimethod1(next, *s))
            {
                val = call_multimethod1(first, *s);
                ret = small_set_conj(*ret, *val);
            }
        }
        else
        {
            val = syntax_quote_val(generated, elem, env);
            ret = small_set_conj(*ret, *val);
        }
    }
    return *ret;
}

Force syntax_quote_map(Root& generated, Value m, Value env)
{
    Root ret{*EMPTY_MAP}, key, val;
    auto size = get_small_map_size(m);
    for (decltype(size) i = 0; i < size; ++i)
    {
        key = get_small_map_key(m, i);
        val = get_small_map_val(m, i);
        if (is_unquote_splicing(*key) || is_unquote_splicing(*val))
        {
            Root msg{create_string("unquote-splicing only supports lists, vectors, and sets")};
            throw_exception(new_illegal_argument(*msg));
        }
        key = syntax_quote_val(generated, *key, env);
        val = syntax_quote_val(generated, *val, env);
        ret = small_map_assoc(*ret, *key, *val);
    }
    return *ret;
}

Force syntax_quote_val(Root& generated, Value val, Value env)
{
    if (get_value_tag(val) == tag::SYMBOL)
        return syntax_quote_symbol(generated, val, env);
    if (get_value_type(val) == type::SmallVector)
        return syntax_quote_vector(generated, val, env);
    if (get_value_type(val) == type::List)
        return syntax_quote_list(generated, val, env);
    if (get_value_type(val) == type::SmallSet)
        return syntax_quote_set(generated, val, env);
    if (get_value_type(val) == type::SmallMap)
        return syntax_quote_map(generated, val, env);
    return val;
}

Force eval_syntax_quote(Value list, Value env)
{
    Root generated{*EMPTY_MAP};
    if (get_list_next(list) == nil || get_list_next(get_list_next(list)) != nil)
    {
        Root msg{create_string("syntax-quote requires exactly 1 argument")};
        throw_exception(new_illegal_argument(*msg));
    }
    return syntax_quote_val(generated, get_list_first(get_list_next(list)), env);
}

template <typename CreateFn>
Force eval_fn(Value list, Value env, CreateFn create_fn)
{
    Root lenv{(env == nil || small_map_contains(env, ENV_NS) == nil) ?
        small_map_assoc(env != nil ? env : *EMPTY_MAP, ENV_NS, lookup_var(CURRENT_NS)) :
        env};
    Root next{get_list_next(list)};
    auto name = nil;
    if (get_value_tag(get_list_first(*next)) == tag::SYMBOL)
    {
        name = get_list_first(*next);
        next = get_list_next(*next);
    }
    if (get_value_type(get_list_first(*next)) == type::SmallVector)
    {
        auto params = get_list_first(*next);
        next = get_list_next(*next);
        auto body = get_list_first(*next);
        return create_fn(*lenv, name, &params, &body, 1);
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
    return create_fn(*lenv, name, params.data(), bodies.data(), params.size());
}

Force eval_fn(Value list, Value env)
{
    return eval_fn(list, env, static_cast<Force(*)(Value, Value, const Value*, const Value*, std::uint8_t)>(create_fn));
}

Force eval_macro(Value list, Value env)
{
    return eval_fn(list, env, static_cast<Force(*)(Value, Value, const Value*, const Value*, std::uint8_t)>(create_macro));
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

Force eval_do(Value list, Value env)
{
    list = get_list_next(list);
    Root ret;
    while (list != nil)
    {
        ret = eval(get_list_first(list), env);
        list = get_list_next(list);
    }
    return *ret;
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
    while (get_value_type(*val) == type::Recur)
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

std::uint8_t find_fn_index(Value fn, std::uint8_t n, std::uint8_t public_n)
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

    throw_arity_error(get_fn_name(fn), public_n);
}

std::vector<Value> eval_args(Roots& arg_roots, Value firstVal, Value list, Value env)
{
    std::vector<Value> elems;
    elems.reserve(get_int64_value(get_list_size(list)));
    elems.push_back(firstVal);
    Roots::size_type i = 0;
    for (Root arg_list{get_list_next(list)}; *arg_list != nil; arg_list = get_list_next(*arg_list), ++i)
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
    auto n_params = va ? get_small_vector_size(params) - 1 : get_small_vector_size(params);
    auto n_fixed_params = va ? n_params - 1 : n_params;
    Root fenv{get_fn_env(fn)};
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
    auto body = get_fn_body(fn, fni);
    Root val{eval(body, *fenv)};
    while (get_value_type(*val) == type::Recur)
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

Force call_macro(Value fn, Value list, Value env)
{
    Root args{get_list_next(list)};
    if (*args == nil)
        args = *EMPTY_LIST;
    Root form{list_conj(*args, fn)};
    Root exp{macroexpand(*form, list, env)};
    return eval(*exp, env);
}

Force eval_list(Value list, Value env)
{
    if (get_int64_value(get_list_size(list)) == 0)
        return list;
    Value first = get_list_first(list);
    if (first == QUOTE)
        return eval_quote(list);
    if (first == SYNTAX_QUOTE)
        return eval_syntax_quote(list, env);
    if (first == FN)
        return eval_fn(list, env);
    if (first == MACRO)
        return eval_macro(list, env);
    if (first == DEF)
        return eval_def(list, env);
    if (first == LET)
        return eval_let(list, env);
    if (first == DO)
        return eval_do(list, env);
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
        return call_macro(*val, list, env);
    Roots arg_roots(get_int64_value(get_list_size(list)));
    auto elems = eval_args(arg_roots, *val, list, env);
    if (type == type::NativeFunction)
        return get_native_function_ptr(*val)(elems.data() + 1, elems.size() - 1);
    if (type == type::Multimethod)
        return call_multimethod(*val, elems.data() + 1, elems.size() - 1);
    if (type == type::Fn)
        return call_fn(elems, elems.size());
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

Force macroexpand1(Value val, Value form, Value env)
{
    if (get_value_type(val) != type::List || get_list_size(val) == 0)
        return val;

    auto m = get_list_first(val);
    if (get_value_type(m) != type::Macro)
        return val;

    std::vector<Value> elems;
    elems.reserve(get_int64_value(get_list_size(val)) + 2);
    elems.push_back(m);
    elems.push_back(form);
    elems.push_back(env);
    for (Root arg_list{get_list_next(val)}; *arg_list != nil; arg_list = get_list_next(*arg_list))
        elems.push_back(get_list_first(*arg_list));

    return call_fn(elems, elems.size() - 3);
}

Force macroexpand(Value val, Value form, Value env)
{
    Root exp{macroexpand1(val, form, env)};
    if (*exp != val)
        return macroexpand(*exp, *exp, env);
    return *exp;
}

Force apply(Value fn, Value args)
{
    auto seq = lookup_var(SEQ);
    auto first = lookup_var(FIRST);
    auto next = lookup_var(NEXT);

    std::uint32_t len = 0;
    for (Root s{call_multimethod1(seq, args)}; *s != nil; s = call_multimethod1(next, *s))
        ++len;
    Roots roots{len};

    std::uint32_t i = 0;
    std::vector<Value> form;
    form.push_back(fn);
    for (Root s{call_multimethod1(seq, args)}; *s != nil; s = call_multimethod1(next, *s))
    {
        roots.set(i, call_multimethod1(first, *s));
        form.push_back(roots[i]);
        ++i;
    }

    Root list{create_list(form.data(), form.size())};
    return eval(*list);
}

Force eval(Value val, Value env)
{
    if (get_value_tag(val) == tag::SYMBOL)
        return eval_symbol(val, env);
    auto type = get_value_type(val);
    if (type == type::List)
        return eval_list(val, env);
    if (type == type::SmallVector)
        return eval_vector(val, env);
    if (type == type::SmallSet)
        return eval_set(val, env);
    if (type == type::SmallMap)
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
