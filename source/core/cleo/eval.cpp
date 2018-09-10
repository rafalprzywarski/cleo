#include "eval.hpp"
#include "var.hpp"
#include "list.hpp"
#include "multimethod.hpp"
#include "global.hpp"
#include "error.hpp"
#include "array.hpp"
#include "namespace.hpp"
#include "reader.hpp"
#include "util.hpp"
#include "bytecode_fn.hpp"
#include "compile.hpp"
#include "cons.hpp"
#include <vector>

namespace cleo
{

namespace
{

Value symbol_var(Value sym, Value env)
{
    if (env && map_contains(env, sym))
        return nil;
    return maybe_resolve_var(sym);
}

std::pair<Value, Int64> find_bytecode_fn_body(Value fn, std::uint8_t arity, std::uint8_t public_n)
{
    auto body = bytecode_fn_find_body(fn, arity);
    if (!body.first)
        throw_arity_error(get_bytecode_fn_name(fn), public_n);
    return body;
}

Force call_bytecode_fn(const Value *elems, std::uint32_t elems_size, std::uint8_t public_n)
{
    auto fn = elems[0];
    auto body_and_arity = find_bytecode_fn_body(fn, elems_size - 1, public_n);
    auto body = body_and_arity.first;
    auto arity = body_and_arity.second;
    auto consts = get_bytecode_fn_body_consts(body);
    auto vars = get_bytecode_fn_body_vars(body);
    auto exception_table = get_bytecode_fn_body_exception_table(body);
    auto locals_size = get_bytecode_fn_body_locals_size(body);
    auto bytes = get_bytecode_fn_body_bytes(body);
    auto bytes_size = get_bytecode_fn_body_bytes_size(body);
    StackGuard guard;
    if (arity < 0)
    {
        auto rest = ~arity + 1;
        stack_push(elems, elems + rest);
        if (rest < elems_size)
        {
            stack_push(create_array(elems + rest, elems_size - rest));
            stack.back() = array_seq(stack.back()).value();
        }
        else
            stack_push(nil);
    }
    else
        stack_push(elems, elems + elems_size);
    stack_reserve(locals_size);
    vm::eval_bytecode(consts, vars, locals_size, exception_table, bytes, bytes_size);
    return stack.back();
}

Force call_fn(Value type, const Value *elems, std::uint32_t elems_size, std::uint8_t public_n)
{
    if (type.is(*type::NativeFunction))
        return get_native_function_ptr(elems[0])(elems + 1, elems_size - 1);
    if (type.is(*type::Multimethod))
        return call_multimethod(elems[0], elems + 1, elems_size - 1);
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

}

Force macroexpand1(Value form, Value env)
{
    if (!is_seq(form) || seq_count(form) == 0)
        return form;

    Root m{seq_first(form)};
    if (get_value_tag(*m) != tag::SYMBOL)
        return form;
    if (SPECIAL_SYMBOLS.count(*m))
        return form;
    auto sym_name = get_symbol_name(*m);
    if (get_string_len(sym_name) > 1 && get_string_ptr(sym_name)[get_string_len(sym_name) - 1] == '.')
    {
        auto new_name = create_symbol(std::string(get_string_ptr(sym_name), get_string_len(sym_name) - 1));
        Root expanded{seq_next(form)};
        expanded = create_cons(new_name, *expanded);
        return create_cons(NEW, *expanded);
    }
    auto var = symbol_var(*m, env);
    if (!var || !is_var_macro(var))
        return form;
    m = get_var_value(var);
    Value mtype = get_value_type(*m);
    if (!mtype.is(*type::BytecodeFn))
        return form;

    Roots relems(seq_count(form) - 1);
    std::vector<Value> elems;
    elems.reserve(seq_count(form) + 2);
    elems.push_back(*m);
    elems.push_back(form);
    elems.push_back(env);
    for (Root arg_list{seq_next(form)}, e; *arg_list; arg_list = seq_next(*arg_list))
        elems.push_back(*(e = seq_first(*arg_list)));

    return call_fn(mtype, elems.data(), elems.size(), elems.size() - 3);
}

Force macroexpand(Value form, Value env)
{
    Root exp{macroexpand1(form, env)};
    if (!exp->is(form))
        return macroexpand(*exp, env);
    return *exp;
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


Force call(const Value *vals, std::uint32_t size)
{
    return call_fn(get_value_type(vals[0]), vals, size);
}

Force eval(Value val, Value env)
{
    if (get_value_type(val).is(*type::List) && get_list_first(val) == FN)
        return compile_fn(val, env);
    std::array<Value, 3> awrap{{FN, *EMPTY_VECTOR, val}};
    Root rwrap{create_list(awrap.data(), awrap.size())};
    rwrap = compile_fn(*rwrap, env);
    auto wrap = *rwrap;
    return call_bytecode_fn(&wrap, 1, 0);
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
