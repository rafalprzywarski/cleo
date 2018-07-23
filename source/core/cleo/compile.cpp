#include "compile.hpp"
#include "bytecode_fn.hpp"
#include "global.hpp"
#include "array.hpp"
#include "list.hpp"
#include "util.hpp"
#include "namespace.hpp"

namespace cleo
{

namespace
{

void append(std::vector<vm::Byte>&) { }

template <typename... Bs>
void append(std::vector<vm::Byte>& v, vm::Byte b, Bs... bs)
{
    v.push_back(b);
    append(v, bs...);
}

Int64 get_arity(Value params)
{
    Int64 size = get_array_size(params);
    return size > 1 && get_array_elem(params, size - 2) == VA ? ~(size - 2) : size;
}

Int64 find_param(Value params, Value param)
{
    auto size = get_array_size(params);
    for (std::uint32_t i = 0; i < size; ++i)
        if (get_array_elem(params, i) == param)
            return i;
    return -1;
}

Int64 add_var(Root& vars, Value v)
{
    auto n = get_int64_value(get_transient_array_size(*vars));
    for (Int64 i = 0; i < n; ++i)
        if (get_transient_array_elem(*vars, i).is(v))
            return i;

    vars = transient_array_conj(*vars, v);
    return n;
}

Int64 add_const(Root& consts, Value c)
{
    auto n = get_int64_value(get_transient_array_size(*consts));
    for (Int64 i = 0; i < n; ++i)
        if (get_transient_array_elem(*consts, i) == c)
            return i;

    consts = transient_array_conj(*consts, c);
    return n;
}

Force normalize_params(Value params)
{
    auto arity = get_arity(params);
    if (arity >= 0)
        return params;
    std::vector<Value> nparams;
    nparams.reserve(~arity + 1);
    for (Int64 i = 0; i < ~arity; ++i)
        nparams.push_back(get_array_elem(params, i));
    nparams.push_back(get_array_elem(params, ~arity + 1));
    return create_array(nparams.data(), nparams.size());
}

void compile_symbol(std::vector<vm::Byte>& code, Root& vars, Value nparams, Value sym)
{
    auto i = find_param(nparams, sym);
    if (i >= 0)
    {
        auto arity = get_array_size(nparams);
        append(code, vm::LDL, i - arity, -1);
    }
    else
    {
        auto v = maybe_resolve_var(sym);
        if (!v)
            throw_compilation_error("unable to resolve symbol: " + to_string(sym));
        auto vi = add_var(vars, v);
        append(code, vm::LDV, vi, 0);
    }
}

void compile_const(std::vector<vm::Byte>& code, Root& consts, Value c)
{
    auto ci = add_const(consts, c);
    append(code, vm::LDC, ci, 0);
}

void compile_value(std::vector<vm::Byte>& code, Root& consts, Root& vars, Value nparams, Value val)
{
    if (get_value_tag(val) == tag::SYMBOL)
        return compile_symbol(code, vars, nparams, val);

    if (get_value_type(val).is(*type::List) && get_list_size(val) > 0)
    {
        for (auto e = val; e; e = get_list_next(e))
            compile_value(code, consts, vars, nparams, get_list_first(e));
        append(code, vm::CALL, get_list_size(val) - 1);
        return;
    }

    compile_const(code, consts, val);
}

Force compile_fn_body(Value form)
{
    std::vector<vm::Byte> code;
    Value val = get_list_first(get_list_next(form));
    Root consts{transient_array(*EMPTY_VECTOR)};
    Root vars{transient_array(*EMPTY_VECTOR)};
    Root nparams{normalize_params(get_list_first(form))};
    compile_value(code, consts, vars, *nparams, val);
    consts = get_int64_value(get_transient_array_size(*consts)) > 0 ? transient_array_persistent(*consts) : nil;
    vars = get_int64_value(get_transient_array_size(*vars)) > 0 ? transient_array_persistent(*vars) : nil;
    return create_bytecode_fn_body(*consts, *vars, 0, code.data(), code.size());
}

Force create_fn(Value name, std::vector<std::pair<Int64, Value>> arities_and_bodies)
{
    std::sort(
        begin(arities_and_bodies), end(arities_and_bodies),
        [](auto& l, auto& r) { return std::abs(l.first) < std::abs(r.first); });
    std::vector<Int64> arities;
    arities.reserve(arities_and_bodies.size());
    std::vector<Value> bodies;
    bodies.reserve(arities_and_bodies.size());
    for (auto& ab : arities_and_bodies)
    {
        arities.push_back(ab.first);
        bodies.push_back(ab.second);
    }
    return create_bytecode_fn(name, arities.data(), bodies.data(), bodies.size());
}

}

Force compile_fn(Value form, Value env)
{
    if (!get_value_type(form).is(*type::List))
        throw_compilation_error("form must be a list");
    if (get_list_first(form) != FN)
        throw_compilation_error("form must start with fn*");
    form = get_list_next(form);
    Value name;
    if (form && get_value_tag(get_list_first(form)) == tag::SYMBOL)
    {
        name = get_list_first(form);
        form = get_list_next(form);
    }
    if (form.is_nil())
        return create_bytecode_fn(name, nullptr, nullptr, 0);
    Root forms{get_value_type(get_list_first(form)).is(*type::List) ? form : create_list(&form, 1)};
    auto count = get_list_size(*forms);
    Roots rbodies(count);
    std::vector<std::pair<Int64, Value>> arities_and_bodies;
    arities_and_bodies.reserve(count);
    for (Int64 i = 0; i < count; ++i)
    {
        auto form = get_list_first(*forms);
        rbodies.set(i, compile_fn_body(form));
        arities_and_bodies.emplace_back(get_arity(get_list_first(form)), rbodies[i]);
        forms = get_list_next(*forms);
    }
    return create_fn(name, std::move(arities_and_bodies));
}

}
