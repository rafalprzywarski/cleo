#include "compile.hpp"
#include "bytecode_fn.hpp"
#include "global.hpp"
#include "array.hpp"
#include "list.hpp"
#include "util.hpp"
#include "namespace.hpp"
#include "persistent_hash_map.hpp"

namespace cleo
{

namespace
{

struct Compiler
{
    std::vector<vm::Byte> code;
    std::int16_t locals_size = 0;
    Root consts{transient_array(*EMPTY_VECTOR)};
    Root vars{transient_array(*EMPTY_VECTOR)};

    void compile_symbol(Value locals, Value sym);
    void compile_const(Value c);
    void compile_call(Value locals, Value val);
    void compile_if(Value locals, Value val);
    void compile_do(Value locals, Value val);
    void compile_quote(Value form);
    void update_locals_size(Value locals);
    void compile_let(Value locals, Value form);
    void compile_value(Value locals, Value val);
};

void check_compiletime_arity(Value name, Value form, std::uint8_t num_args, std::uint8_t actual_num_args)
{
    if (num_args != actual_num_args)
        throw_compilation_error("Wrong number of args (" + std::to_string(actual_num_args) + ") passed to " + to_string(name) + ", form: " + to_string(form));
}

void append(std::vector<vm::Byte>&) { }

template <typename... Bs>
void append(std::vector<vm::Byte>& v, vm::Byte b, Bs... bs)
{
    v.push_back(b);
    append(v, bs...);
}

void append_i16(std::vector<vm::Byte>& v, std::int16_t n)
{
    append(v, n & 0xff, (n >> 8) & 0xff);
}

Int64 get_arity(Value params)
{
    Int64 size = get_array_size(params);
    return size > 1 && get_array_elem(params, size - 2) == VA ? ~(size - 2) : size;
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

Force create_locals(Value params)
{
    Root locals{create_persistent_hash_map()}, index;
    auto arity = get_arity(params);
    auto fixed_arity = arity < 0 ? ~arity : arity;
    auto total_arity = arity < 0 ? ~arity + 1 : arity;
    for (Int64 i = 0; i < fixed_arity; ++i)
    {
        index = create_int64(i - total_arity);
        locals = persistent_hash_map_assoc(*locals, get_array_elem(params, i), *index);
    }
    if (arity < 0)
        locals = persistent_hash_map_assoc(*locals, get_array_elem(params, total_arity), *NEG_ONE);
    return *locals;
}

void Compiler::compile_symbol(Value locals, Value sym)
{
    if (auto index = persistent_hash_map_get(locals, sym))
    {
        append(code, vm::LDL);
        return append_i16(code, get_int64_value(index));
    }

    auto v = maybe_resolve_var(sym);
    if (!v)
        throw_compilation_error("unable to resolve symbol: " + to_string(sym));
    auto vi = add_var(vars, v);
    append(code, vm::LDV, vi, 0);
}

void Compiler::compile_const(Value c)
{
    auto ci = add_const(consts, c);
    append(code, vm::LDC, ci, 0);
}

void Compiler::compile_call(Value locals, Value val)
{
    for (auto e = val; e; e = get_list_next(e))
        compile_value(locals, get_list_first(e));
    append(code, vm::CALL, get_list_size(val) - 1);
}

void Compiler::compile_if(Value locals, Value val)
{
    auto cond = get_list_next(val);
    compile_value(locals, get_list_first(cond));
    auto bnil_offset = code.size();
    append(code, vm::BNIL, 0, 0);
    auto then = get_list_next(cond);
    compile_value(locals, get_list_first(then));
    auto br_offset = code.size();
    append(code, vm::BR, 0, 0);
    code[bnil_offset + 1] = code.size() - bnil_offset - 3;
    auto else_ = get_list_next(then);
    compile_value(locals, else_ ? get_list_first(else_) : nil);
    code[br_offset + 1] = code.size() - br_offset - 3;
}

void Compiler::compile_do(Value locals, Value val)
{
    if (get_list_size(val) == 1)
        return append(code, vm::CNIL);

    for (val = get_list_next(val); get_list_next(val); val = get_list_next(val))
    {
        compile_value(locals, get_list_first(val));
        append(code, vm::POP);
    }
    compile_value(locals, get_list_first(val));
}

void Compiler::compile_quote(Value form)
{
    check_compiletime_arity(QUOTE, form, 1, get_list_size(form) - 1);
    compile_const(get_list_first(get_list_next(form)));
}

void Compiler::update_locals_size(Value locals)
{
    auto size = persistent_hash_map_get(locals, *ZERO, *ZERO);
    locals_size = std::max(locals_size, std::int16_t(get_int64_value(size)));
}

std::pair<Force, std::int16_t> add_local(Value locals, Value sym)
{
    auto size = persistent_hash_map_get(locals, *ZERO, *ZERO);
    Root new_locals{persistent_hash_map_assoc(locals, sym, size)};
    Root new_size{create_int64(get_int64_value(size) + 1)};
    return {persistent_hash_map_assoc(*new_locals, *ZERO, *new_size), std::int16_t(get_int64_value(size))};
}

void Compiler::compile_let(Value locals, Value form)
{
    check_compiletime_arity(LET, form, 2, get_list_size(form) - 1);
    form = get_list_next(form);
    auto bindings = get_list_first(form);
    if (!get_value_type(bindings).is(*type::Array))
        throw_compilation_error("Bad binding form, expected vector");
    if (get_array_size(bindings) % 2)
        throw_compilation_error("Bad binding form, expected matched symbol expression pairs");

    auto expr = get_list_first(get_list_next(form));
    Root llocals{locals};
    std::int16_t index{};
    for (Int64 i = 0; i < get_array_size(bindings); i += 2)
    {
        compile_value(*llocals, get_array_elem(bindings, i + 1));
        std::tie(llocals, index) = add_local(*llocals, get_array_elem(bindings, i));
        append(code, vm::STL, index, 0);
    }
    update_locals_size(*llocals);
    compile_value(*llocals, expr);
}

void Compiler::compile_value(Value locals, Value val)
{
    if (val.is_nil())
        return append(code, vm::CNIL);
    if (get_value_tag(val) == tag::SYMBOL)
        return compile_symbol(locals, val);

    if (get_value_type(val).is(*type::List) && get_list_size(val) > 0)
    {
        auto first = get_list_first(val);
        if (first == IF)
            return compile_if(locals, val);
        if (first == DO)
            return compile_do(locals, val);
        if (first == QUOTE)
            return compile_quote(val);
        if (first == LET)
            return compile_let(locals, val);

        return compile_call(locals, val);
    }

    compile_const(val);
}

Force compile_fn_body(Value form)
{
    Compiler c;
    Value val = get_list_first(get_list_next(form));
    Root locals{create_locals(get_list_first(form))};
    c.compile_value(*locals, val);
    Root consts{get_int64_value(get_transient_array_size(*c.consts)) > 0 ? transient_array_persistent(*c.consts) : nil};
    Root vars{get_int64_value(get_transient_array_size(*c.vars)) > 0 ? transient_array_persistent(*c.vars) : nil};
    return create_bytecode_fn_body(*consts, *vars, c.locals_size, c.code.data(), c.code.size());
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
