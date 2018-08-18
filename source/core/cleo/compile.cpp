#include "compile.hpp"
#include "bytecode_fn.hpp"
#include "global.hpp"
#include "array.hpp"
#include "array_set.hpp"
#include "list.hpp"
#include "util.hpp"
#include "namespace.hpp"
#include "persistent_hash_map.hpp"
#include "eval.hpp"
#include <algorithm>

namespace cleo
{

namespace
{

Force compile_ifn(Value form, Value env, Value parent_locals, Root& used_locals);

struct Compiler
{
    struct Scope
    {
        Value env;
        Value parent_locals;
        Value locals;
        std::uint16_t recur_arity{};
        std::int16_t recur_locals_index{};
        Int64 recur_start_offset{};
        std::uint16_t locals_size{};
    };

    std::vector<vm::Byte> code;
    std::int16_t locals_size = 0;
    Root consts{transient_array(*EMPTY_VECTOR)};
    Root vars{transient_array(*EMPTY_VECTOR)};
    Root local_refs;
    std::vector<std::uint32_t> local_ref_offsets;
    std::vector<Int64> et_entries;
    std::vector<Value> et_types;

    Compiler(Value local_refs) : local_refs(transient_array(local_refs ? local_refs : *EMPTY_VECTOR)) { }

    void compile_symbol(const Scope& scope, Value sym);
    void compile_const(Value c);
    Int64 add_local_ref(Value sym);
    void compile_local_ref(Value sym);
    void compile_call(Scope scope, Value val);
    void compile_apply(Scope scope, Value form);
    void compile_if(Scope scope, Value val, bool wrap_try);
    void compile_do(Scope scope, Value val, bool wrap_try);
    void compile_quote(Value form);
    void update_locals_size(Scope scope);
    Scope compile_let_bindings(Scope scope, Value bindings, Root& llocals);
    void compile_let(Scope scope, Value form, bool wrap_try);
    void compile_loop(Scope scope, Value form);
    void compile_recur(Scope scope, Value form);
    void compile_vector(Scope scope, Value val);
    void compile_hash_set(Scope scope, Value val);
    void compile_hash_map(Scope scope, Value val);
    void compile_def(Scope scope, Value form);
    void compile_fn(Scope scope, Value form);
    void compile_throw(Scope scope, Value form);
    void add_exception_handler(Int64 start, Int64 end, Int64 handler, Value type);
    void compile_try(Scope scope, Value form);
    void compile_try_wrapped(Scope scope, Value form);
    void compile_value(Scope scope, Value val, bool wrap_try = true);
};

void throw_compiletime_arity_error(Value name, Value form, std::uint8_t actual_num_args)
{
    throw_compilation_error("Wrong number of args (" + std::to_string(actual_num_args) + ") passed to " + to_string(name) + ", form: " + to_string(form));
}

void check_compiletime_arity(Value name, Value form, std::uint8_t num_args, std::uint8_t actual_num_args)
{
    if (num_args != actual_num_args)
        throw_compiletime_arity_error(name, form, actual_num_args);
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

void append_STL(std::vector<vm::Byte>& v, std::int16_t n)
{
    append(v, vm::STL);
    append_i16(v, n);
}

void append_BR(std::vector<vm::Byte>& v, std::int16_t n)
{
    append(v, vm::BR);
    append_i16(v, n);
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
    {
        auto e = get_transient_array_elem(*consts, i);
        if (get_value_type(e).is(get_value_type(c)) && e == c)
            return i;
    }

    consts = transient_array_conj(*consts, c);
    return n;
}

Int64 Compiler::add_local_ref(Value sym)
{
    auto n = get_int64_value(get_transient_array_size(*local_refs));
    for (Int64 i = 0; i < n; ++i)
        if (get_transient_array_elem(*local_refs, i) == sym)
            return i;

    local_refs = transient_array_conj(*local_refs, sym);
    return n;
}

Force create_locals(Value name, Value params)
{
    if (!get_value_type(params).is(*type::Array))
        throw_compilation_error("Bad " + to_string(FN) + " param list, expected vector");
    Root locals{*EMPTY_MAP}, index;
    auto arity = get_arity(params);
    auto fixed_arity = arity < 0 ? ~arity : arity;
    auto total_arity = arity < 0 ? ~arity + 1 : arity;
    if (name)
    {
        index = create_int64(-(total_arity + 1));
        locals = persistent_hash_map_assoc(*locals, name, *index);
    }
    for (Int64 i = 0; i < fixed_arity; ++i)
    {
        auto sym = get_array_elem(params, i);
        if (!get_value_type(sym).is(*type::Symbol))
            throw_compilation_error(to_string(FN) + " params must be symbols");
        if (get_symbol_namespace(sym))
            throw_compilation_error("Can't use qualified name as parameter: " + to_string(sym));
        index = create_int64(i - total_arity);
        locals = persistent_hash_map_assoc(*locals, sym, *index);
    }
    if (arity < 0)
        locals = persistent_hash_map_assoc(*locals, get_array_elem(params, total_arity), *NEG_ONE);
    return *locals;
}

void Compiler::compile_symbol(const Scope& scope, Value sym)
{
    if (auto index = persistent_hash_map_get(scope.locals, sym))
    {
        append(code, vm::LDL);
        return append_i16(code, get_int64_value(index));
    }
    if (persistent_hash_map_contains(scope.parent_locals, sym))
        return compile_local_ref(sym);
    if (scope.env && map_contains(scope.env, sym))
        return compile_const(map_get(scope.env, sym));

    auto v = maybe_resolve_var(sym);
    if (!v)
        throw_compilation_error("unable to resolve symbol: " + to_string(sym));
    if (is_var_macro(v))
        throw_compilation_error("Can't take value of a macro: " + to_string(v));
    auto vi = add_var(vars, v);
    append(code, vm::LDV, vi, 0);
}

void Compiler::compile_const(Value c)
{
    if (c.is_nil())
        return append(code, vm::CNIL);
    auto ci = add_const(consts, c);
    append(code, vm::LDC, ci, 0);
}

void Compiler::compile_local_ref(Value sym)
{
    auto ri = add_local_ref(sym);
    local_ref_offsets.push_back(code.size() + 1);
    append(code, vm::LDC, ri, 0);
}

void Compiler::compile_call(Scope scope, Value val)
{
    for (auto e = val; e; e = get_list_next(e))
        compile_value(scope, get_list_first(e));
    append(code, vm::CALL, get_list_size(val) - 1);
}

void Compiler::compile_apply(Scope scope, Value form)
{
    auto size = get_list_size(form);
    if (size < 3)
        throw_compiletime_arity_error(APPLY_SPECIAL, form, size - 1);
    for (auto e = get_list_next(form); e; e = get_list_next(e))
        compile_value(scope, get_list_first(e));
    append(code, vm::APPLY, size - 3);
}


void Compiler::compile_if(Scope scope, Value val, bool wrap_try)
{
    auto cond = get_list_next(val);
    if (!cond)
        throw_compilation_error("Too few arguments to if");
    auto then = get_list_next(cond);
    if (!then)
        throw_compilation_error("Too few arguments to if");
    auto else_ = get_list_next(then);
    if (else_ && get_list_next(else_))
        throw_compilation_error("Too many arguments to if");

    compile_value(scope, get_list_first(cond));
    auto bnil_offset = code.size();
    append(code, vm::BNIL, 0, 0);
    compile_value(scope, get_list_first(then), wrap_try);
    auto br_offset = code.size();
    append_BR(code, 0);
    code[bnil_offset + 1] = code.size() - bnil_offset - 3;
    compile_value(scope, else_ ? get_list_first(else_) : nil, wrap_try);
    code[br_offset + 1] = code.size() - br_offset - 3;
}

void Compiler::compile_do(Scope scope, Value val, bool wrap_try)
{
    if (get_list_size(val) == 1)
        return append(code, vm::CNIL);

    for (val = get_list_next(val); get_list_next(val); val = get_list_next(val))
    {
        compile_value(scope, get_list_first(val), wrap_try);
        append(code, vm::POP);
    }
    compile_value(scope, get_list_first(val), wrap_try);
}

void Compiler::compile_quote(Value form)
{
    check_compiletime_arity(QUOTE, form, 1, get_list_size(form) - 1);
    compile_const(get_list_first(get_list_next(form)));
}

void Compiler::update_locals_size(Scope scope)
{
    locals_size = std::max(locals_size, std::int16_t(scope.locals_size));
}

std::pair<Compiler::Scope, std::int16_t> add_local(Compiler::Scope scope, Value sym, Root& holder)
{
    Root index{create_int64(scope.locals_size)};
    holder = persistent_hash_map_assoc(scope.locals, sym, *index);
    scope.locals = *holder;
    scope.locals_size++;
    return {scope, scope.locals_size - 1};
}

Value check_let_bindings(Value tag, Value form)
{
    check_compiletime_arity(tag, form, 2, get_list_size(form) - 1);
    auto bindings = get_list_first(get_list_next(form));
    if (!get_value_type(bindings).is(*type::Array))
        throw_compilation_error("Bad binding form, expected vector");
    if (get_array_size(bindings) % 2)
        throw_compilation_error("Bad binding form, expected matched symbol expression pairs");
    return bindings;
}

Compiler::Scope Compiler::compile_let_bindings(Scope scope, Value bindings, Root& llocals)
{
    std::int16_t index{};
    for (Int64 i = 0; i < get_array_size(bindings); i += 2)
    {
        auto sym = get_array_elem(bindings, i);
        if (!get_value_type(sym).is(*type::Symbol))
            throw_compilation_error("Unsupported binding form: " + to_string(sym));
        if (get_symbol_namespace(sym))
            throw_compilation_error("Can't let qualified name: " + to_string(sym));
        compile_value(scope, get_array_elem(bindings, i + 1));
        std::tie(scope, index) = add_local(scope, sym, llocals);
        append_STL(code, index);
    }
    update_locals_size(scope);
    return scope;
}

void Compiler::compile_let(Scope scope, Value form, bool wrap_try)
{
    auto bindings = check_let_bindings(LET, form);
    Root llocals;
    scope = compile_let_bindings(scope, bindings, llocals);

    auto expr = get_list_first(get_list_next(get_list_next(form)));
    compile_value(scope, expr, wrap_try);
}

void Compiler::compile_loop(Scope scope, Value form)
{
    auto bindings = check_let_bindings(LOOP, form);
    Root llocals;
    scope = compile_let_bindings(scope, bindings, llocals);

    auto loop_locals_size = get_array_size(bindings) / 2;
    scope.recur_arity = loop_locals_size;
    scope.recur_locals_index = scope.locals_size - loop_locals_size;
    scope.recur_start_offset = code.size();

    auto expr = get_list_first(get_list_next(get_list_next(form)));
    compile_value(scope, expr);
}

void Compiler::compile_recur(Scope scope, Value form)
{
    form = get_list_next(form);
    auto size = form ? get_list_size(form) : 0;
    if (size != scope.recur_arity)
        throw_compilation_error("Mismatched argument count to recur, expected: " + std::to_string(scope.recur_arity) +
                                " args, got: " + std::to_string(size));
    for (; form; form = get_list_next(form))
        compile_value(scope, get_list_first(form));
    for (Int64 i = 0; i < size; ++i)
        append_STL(code, scope.recur_locals_index + (size - i - 1));
    append_BR(code, scope.recur_start_offset - 3 - Int64(code.size()));
}

bool is_const(Value val)
{
    auto tag = get_value_tag(val);
    return tag != tag::SYMBOL && tag != tag::OBJECT;
}

Int64 get_vector_const_prefix_len(Value val)
{
    auto size = get_array_size(val);
    Int64 prefix_len = 0;
    while (prefix_len < size && is_const(get_array_elem(val, prefix_len)))
        ++prefix_len;
    return prefix_len;
}

Force get_vector_prefix(Value val, Int64 prefix_len)
{
    if (prefix_len == 0)
        return *EMPTY_VECTOR;
    Root prefix{transient_array(*EMPTY_VECTOR)};
    for (Int64 i = 0; i < prefix_len; ++i)
        prefix = transient_array_conj(*prefix, get_array_elem(val, i));
    return transient_array_persistent(*prefix);
}

void Compiler::compile_vector(Scope scope, Value val)
{
    auto size = get_array_size(val);
    Int64 prefix_len = get_vector_const_prefix_len(val);
    if (prefix_len == size)
        return compile_const(val);
    compile_const(*rt::transient_array_persistent);
    for (Int64 i = prefix_len; i < size; ++i)
        compile_const(*rt::transient_array_conj);
    compile_const(*rt::transient_array);
    Root prefix{get_vector_prefix(val, prefix_len)};
    compile_const(*prefix);
    append(code, vm::CALL, 1);
    for (Int64 i = prefix_len; i < size; ++i)
    {
        compile_value(scope, get_array_elem(val, i));
        append(code, vm::CALL, 2);
    }
    append(code, vm::CALL, 1);
}

Force get_hash_set_const_subset(Value val)
{
    Root ss{*EMPTY_SET};
    Int64 size = get_array_set_size(val);
    for (Int64 i = 0; i < size; ++i)
    {
        auto e = get_array_set_elem(val, i);
        if (is_const(e))
            ss = array_set_conj(*ss, e);
    }
    return *ss;
}

void Compiler::compile_hash_set(Scope scope, Value val)
{
    Root subset{get_hash_set_const_subset(val)};
    auto size = get_array_set_size(val);
    auto subset_size = get_array_set_size(*subset);
    if (subset_size == size)
        return compile_const(val);
    for (Int64 i = subset_size; i < size; ++i)
        compile_const(*rt::array_set_conj);
    compile_const(*subset);
    for (Int64 i = 0; i < size; ++i)
    {
        auto e = get_array_set_elem(val, i);
        if (!array_set_contains(*subset,  e))
        {
            compile_value(scope, e);
            append(code, vm::CALL, 2);
        }
    }
}

Force get_hash_map_const_submap(Value val)
{
    Root sm{*EMPTY_MAP};
    for (Root s{persistent_hash_map_seq(val)}; *s; s = get_persistent_hash_map_seq_next(*s))
    {
        auto kv = get_persistent_hash_map_seq_first(*s);
        auto k = get_array_elem(kv, 0);
        auto v = get_array_elem(kv, 1);
        if (is_const(k) && is_const(v))
            sm = persistent_hash_map_assoc(*sm, k, v);
    }
    return *sm;
}

void Compiler::compile_hash_map(Scope scope, Value val)
{
    Root submap{get_hash_map_const_submap(val)};
    auto size = get_persistent_hash_map_size(val);
    auto submap_size = get_persistent_hash_map_size(*submap);
    if (submap_size == size)
        return compile_const(val);
    for (Int64 i = submap_size; i < size; ++i)
        compile_const(*rt::persistent_hash_map_assoc);
    compile_const(*submap);
    for (Root s{persistent_hash_map_seq(val)}; *s; s = get_persistent_hash_map_seq_next(*s))
    {
        auto kv = get_persistent_hash_map_seq_first(*s);
        auto k = get_array_elem(kv, 0);
        auto v = get_array_elem(kv, 1);
        if (!persistent_hash_map_contains(*submap, k))
        {
            compile_value(scope, k);
            compile_value(scope, v);
            append(code, vm::CALL, 3);
        }
    }
}

void Compiler::compile_def(Scope scope, Value form)
{
    form = get_list_next(form);
    if (!form)
        throw_compilation_error("Too few arguments to def");
    auto name = get_list_first(form);
    form = get_list_next(form);
    Value meta = nil;
    if (get_value_type(name).is(*type::PersistentHashMap))
    {
        if (!form)
            throw_compilation_error("Too few arguments to def");
        meta = name;
        name = get_list_first(form);
        form = get_list_next(form);
    }
    if (get_value_tag(name) != tag::SYMBOL)
        throw_compilation_error("First argument to def must be a Symbol");
    auto val = form ? get_list_first(form) : nil;
    if (form && get_list_next(form))
        throw_compilation_error("Too many arguments to def");
    auto current_ns_name = get_symbol_name(ns_name(*rt::current_ns));
    auto sym_ns = get_symbol_namespace(name);
    if (sym_ns && sym_ns != current_ns_name)
        throw_compilation_error(maybe_resolve_var(name) ?
                                "Can't create defs outside of current ns" :
                                "Can't refer to qualified var that doesn't exist");
    auto sym_name = get_symbol_name(name);
    name = create_symbol(
        {get_string_ptr(current_ns_name), get_string_len(current_ns_name)},
        {get_string_ptr(sym_name), get_string_len(sym_name)});
    auto var = define(name, nil, meta);
    compile_const(var);
    compile_value(scope, val);
    compile_value(scope, meta);
    append(code, vm::SETV);
}

void Compiler::compile_fn(Scope scope, Value form)
{
    Root used_locals;
    Root parent_locals{map_merge(scope.parent_locals, scope.locals)};
    Root fn{compile_ifn(form, scope.env, *parent_locals, used_locals)};
    compile_const(*fn);
    if (*used_locals)
    {
        auto size = get_array_size(*used_locals);
        for (Int64 i = 0; i < size; ++i)
            compile_symbol(scope, get_array_elem(*used_locals, i));
        append(code, vm::IFN, size);
    }
}

void Compiler::compile_throw(Scope scope, Value form)
{
    form = get_list_next(form);
    if (!form)
        throw_compilation_error("Too few arguments to throw, expected a single value");
    compile_value(scope, get_list_first(form));
    if (get_list_next(form))
        throw_compilation_error("Too many arguments to throw, expected a single value");
    append(code, vm::THROW);
}

void Compiler::add_exception_handler(Int64 start, Int64 end, Int64 handler, Value type)
{
    et_entries.push_back(start);
    et_entries.push_back(end);
    et_entries.push_back(handler);
    et_types.push_back(type);
}

void Compiler::compile_try(Scope scope, Value form)
{
    auto start_offset = code.size();
    form = get_list_next(form);
    compile_value(scope, form ? get_list_first(form) : nil, false);
    auto handler_ = form ? get_list_next(form) : nil;
    if (handler_ && get_list_next(handler_))
        throw_compilation_error("Too many expressions in " + to_string(TRY));
    handler_ = handler_ ? get_list_first(handler_) : nil;
    if (!handler_)
        return;
    if (!get_value_type(handler_).is(*type::List) ||
        (get_list_first(handler_) != CATCH && get_list_first(handler_) != FINALLY))
        throw_compilation_error("expected " + to_string(CATCH) + " or " + to_string(FINALLY) + " block in " + to_string(TRY));
    auto tag = get_list_first(handler_);
    if (tag == CATCH)
    {
        auto catch_ = get_list_next(handler_);
        if (!catch_)
            throw_compilation_error("missing exception type in " + to_string(CATCH));
        auto type_sym = get_list_first(catch_);
        catch_ = get_list_next(catch_);
        if (!catch_)
            throw_compilation_error("missing exception binding in " + to_string(CATCH));
        auto local = get_list_first(catch_);
        catch_ = get_list_next(catch_);
        if (!catch_)
            throw_compilation_error("missing catch* body");
        auto expr = get_list_first(catch_);
        if (get_list_next(catch_))
            throw_compilation_error("Too many expressions in " + to_string(CATCH) + ", expected one");
        Root rlocal;
        Int64 index{};
        std::tie(scope, index) = add_local(scope, local, rlocal);
        update_locals_size(scope);
        auto br_offset = code.size();
        append_BR(code, 0);
        auto type = maybe_resolve_var(type_sym);
        if (!type)
            throw_compilation_error("unable to resolve symbol: " + to_string(type_sym));
        add_exception_handler(start_offset, br_offset, code.size(), get_var_value(type));
        append_STL(code, index);
        compile_value(scope, expr);
        code[br_offset + 1] = code.size() - (br_offset + 3);
    }
    else if (tag == FINALLY)
    {
        handler_ = get_list_next(handler_);
        if (!handler_)
            throw_compilation_error("missing " + to_string(FINALLY) + " body");
        auto expr = get_list_first(handler_);
        if (get_list_next(handler_))
            throw_compilation_error("Too many expressions in " + to_string(FINALLY) + ", expected one");
        auto end_offset = code.size();
        compile_value(scope, expr);
        append(code, vm::POP);
        auto br_offset = code.size();
        append_BR(code, 0);
        add_exception_handler(start_offset, end_offset, code.size(), nil);
        Root rlocal;
        Int64 index{};
        std::tie(scope, index) = add_local(scope, nil, rlocal);
        update_locals_size(scope);
        append_STL(code, index);
        compile_value(scope, expr);
        append(code, vm::POP);
        append(code, vm::LDL, index, 0);
        append(code, vm::THROW);
        code[br_offset + 1] = code.size() - (br_offset + 3);
    }
}

void Compiler::compile_try_wrapped(Scope scope, Value form)
{
    std::array<Value, 3> fn_elems{{FN, *EMPTY_VECTOR, form}};
    Root rfn{create_list(fn_elems.data(), fn_elems.size())};
    auto fn = *rfn;
    Root call{create_list(&fn, 1)};
    compile_value(scope, *call);
}

void Compiler::compile_value(Scope scope, Value val, bool wrap_try)
{
    Root xval{macroexpand(val)};
    val = *xval;

    if (get_value_tag(val) == tag::SYMBOL)
        return compile_symbol(scope, val);

    auto vtype = get_value_type(val);
    if (vtype.is(*type::List) && get_list_size(val) > 0)
    {
        auto first = get_list_first(val);
        if (first == IF)
            return compile_if(scope, val, wrap_try);
        if (first == DO)
            return compile_do(scope, val, wrap_try);
        if (first == QUOTE)
            return compile_quote(val);
        if (first == LET)
            return compile_let(scope, val, wrap_try);
        if (first == RECUR)
            return compile_recur(scope, val);
        if (first == LOOP)
            return compile_loop(scope, val);
        if (first == DEF)
            return compile_def(scope, val);
        if (first == APPLY_SPECIAL)
            return compile_apply(scope, val);
        if (first == FN)
            return compile_fn(scope, val);
        if (first == THROW)
            return compile_throw(scope, val);
        if (first == TRY)
            return wrap_try ? compile_try_wrapped(scope, val) : compile_try(scope, val);

        return compile_call(scope, val);
    }

    if (vtype.is(*type::Array))
        return compile_vector(scope, val);
    if (vtype.is(*type::ArraySet))
        return compile_hash_set(scope, val);
    if (vtype.is(*type::PersistentHashMap))
        return compile_hash_map(scope, val);

    compile_const(val);
}

Compiler::Scope create_fn_body_scope(Value form, Value env, Value locals, Value parent_locals)
{
    auto recur_arity = std::uint16_t(std::abs(get_arity(get_list_first(form))));
    return {env, parent_locals, locals, recur_arity, std::int16_t(-recur_arity)};
}

Force compile_fn_body(Value name, Value form, Value env, Value parent_locals, Root& used_locals)
{
    Compiler c(*used_locals);
    Value val = get_list_next(form);
    if (val && get_list_next(val))
        throw_compilation_error("Too many forms passed to " + to_string(FN));
    val = val ? get_list_first(val) : nil;
    Root locals{create_locals(name, get_list_first(form))};
    auto scope = create_fn_body_scope(form, env, *locals, parent_locals);
    c.compile_value(scope, val, false);
    auto used_locals_size = get_int64_value(get_transient_array_size(*c.local_refs));
    auto consts_size = get_int64_value(get_transient_array_size(*c.consts));
    for (auto off : c.local_ref_offsets)
        c.code[off] += consts_size;
    Root consts{consts_size > 0 ? transient_array_persistent(*c.consts) : nil};
    Root vars{get_int64_value(get_transient_array_size(*c.vars)) > 0 ? transient_array_persistent(*c.vars) : nil};
    used_locals = used_locals_size > 0 ? transient_array_persistent(*c.local_refs) : nil;
    Root exception_table{!c.et_types.empty() ? create_bytecode_fn_exception_table(c.et_entries.data(), c.et_types.data(), c.et_types.size()) : nil};
    return create_bytecode_fn_body(*consts, *vars, *exception_table, c.locals_size, c.code.data(), c.code.size());
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

Force reserve_fn_body_consts(Value body, Int64 n)
{
    if (n == 0)
        return body;
    Root consts{get_bytecode_fn_body_consts(body)};
    consts = transient_array(*consts ? *consts : *EMPTY_VECTOR);
    for (Int64 i = 0; i < n; ++i)
        consts = transient_array_conj(*consts, nil);
    consts = get_int64_value(get_transient_array_size(*consts)) > 0 ? transient_array_persistent(*consts) : nil;

    return create_bytecode_fn_body(*consts, get_bytecode_fn_body_vars(body), get_bytecode_fn_body_exception_table(body), get_bytecode_fn_body_locals_size(body), get_bytecode_fn_body_bytes(body), get_bytecode_fn_body_bytes_size(body));
}

Force compile_ifn(Value form, Value env, Value parent_locals, Root& used_locals)
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
    used_locals = nil;
    for (Int64 i = 0; i < count; ++i)
    {
        auto form = get_list_first(*forms);
        rbodies.set(i, compile_fn_body(name, form, env, parent_locals, used_locals));
        arities_and_bodies.emplace_back(get_arity(get_list_first(form)), rbodies[i]);
        forms = get_list_next(*forms);
    }
    for (Int64 i = 0; i < count; ++i)
    {
        rbodies.set(i, reserve_fn_body_consts(arities_and_bodies[i].second, get_array_size(*used_locals)));
        arities_and_bodies[i].second = rbodies[i];
    }
    return create_fn(name, std::move(arities_and_bodies));
}

}

Force compile_fn(Value form, Value env)
{
    Root used_locals;
    return compile_ifn(form, env, *EMPTY_MAP, used_locals);
}

}
