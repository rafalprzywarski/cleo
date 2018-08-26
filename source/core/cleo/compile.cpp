#include "compile.hpp"
#include "bytecode_fn.hpp"
#include "global.hpp"
#include "array.hpp"
#include "array_set.hpp"
#include "util.hpp"
#include "namespace.hpp"
#include "persistent_hash_map.hpp"
#include "eval.hpp"
#include "multimethod.hpp"
#include "cons.hpp"
#include <algorithm>

namespace cleo
{

namespace
{

static constexpr Int64 MAX_ARGS = 255;
static constexpr Int64 MAX_LOCALS = 32767;
static constexpr Int64 MAX_CONSTS = 65535;
static constexpr Int64 MAX_VARS = 65535;

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

Compiler::Scope no_recur(Compiler::Scope s)
{
    s.recur_start_offset = -1;
    return s;
}

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

void append_u16(std::vector<vm::Byte>& v, std::uint16_t n)
{
    append_i16(v, std::int16_t(n));
}

void set_i16(std::vector<vm::Byte>& v, std::size_t off, std::int16_t n)
{
    v[off] = n & 0xff;
    v[off + 1] = (n >> 8) & 0xff;
}

std::int16_t get_i16(const std::vector<vm::Byte>& v, std::size_t off)
{
    return std::int16_t(std::uint8_t(v[off]) | (std::uint16_t(std::uint8_t(v[off + 1])) << 8));
}

void append_STL(std::vector<vm::Byte>& v, std::int16_t n)
{
    append(v, vm::STL);
    append_i16(v, n);
}

void append_LDL(std::vector<vm::Byte>& v, std::int16_t n)
{
    append(v, vm::LDL);
    append_i16(v, n);
}

void append_LDC(std::vector<vm::Byte>& v, std::int16_t n)
{
    append(v, vm::LDC);
    append_u16(v, n);
}

std::int16_t cast_branch_offset(Int64 n)
{
    if (n < -32768 || n > 32767)
        throw_compilation_error("Branch out of range");
    return std::int64_t(n);
}

auto append_branch(std::vector<vm::Byte>& v, vm::Byte b, Int64 n)
{
    auto off = v.size();
    append(v, b);
    append_i16(v, cast_branch_offset(n));
    return off;
}

void set_branch_target(std::vector<vm::Byte>& v, std::size_t off, std::size_t target)
{
    auto br_off = std::ptrdiff_t(target) - std::ptrdiff_t(off) - 3;
    set_i16(v, off + 1, cast_branch_offset(br_off));
}

Int64 get_arity(Value params)
{
    Int64 size = get_array_size(params);
    return size > 1 && get_array_elem(params, size - 2) == VA ? ~(size - 2) : size;
}

Int64 add_var(Root& vars, Value v)
{
    auto n = get_int64_value(get_transient_array_size(*vars));
    if (n == MAX_VARS)
        throw_compilation_error("Too many vars: " + std::to_string(n + 1));
    for (Int64 i = 0; i < n; ++i)
        if (get_transient_array_elem(*vars, i).is(v))
            return i;

    vars = transient_array_conj(*vars, v);
    return n;
}

Int64 add_const(Root& consts, Value c)
{
    auto n = get_int64_value(get_transient_array_size(*consts));
    if (n == MAX_CONSTS)
        throw_compilation_error("Too many constants: " + std::to_string(n + 1));
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
        return append_LDL(code, get_int64_value(index));
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
    append(code, vm::LDV);
    append_u16(code, vi);
}

void Compiler::compile_const(Value c)
{
    if (c.is_nil())
        return append(code, vm::CNIL);
    auto ci = add_const(consts, c);
    append_LDC(code, ci);
}

void Compiler::compile_local_ref(Value sym)
{
    auto ri = add_local_ref(sym);
    local_ref_offsets.push_back(code.size() + 1);
    append_LDC(code, ri);
}

void Compiler::compile_call(Scope scope, Value val)
{
    scope = no_recur(scope);
    std::uint32_t n = 0;
    for (Root e{val}, v; *e; e = seq_next(*e))
    {
        v = seq_first(*e);
        compile_value(scope, *v);
        ++n;
    }
    --n;
    if (n > MAX_ARGS)
        throw_compilation_error("Too many arguments: " + std::to_string(n));
    append(code, vm::CALL, n);
}

void Compiler::compile_apply(Scope scope, Value form)
{
    scope = no_recur(scope);
    auto size = seq_count(form);
    if (size < 3)
        throw_compiletime_arity_error(APPLY, form, size - 1);
    for (Root e{seq_next(form)}, v; *e; e = seq_next(*e))
    {
        v = seq_first(*e);
        compile_value(scope, *v);
    }
    append(code, vm::APPLY, size - 3);
}


void Compiler::compile_if(Scope scope, Value val, bool wrap_try)
{
    Root cond{seq_next(val)};
    if (!*cond)
        throw_compilation_error("Too few arguments to if");
    Root then{seq_next(*cond)};
    if (!*then)
        throw_compilation_error("Too few arguments to if");
    Root else_{seq_next(*then)};
    if (*else_ && seq_next(*else_).value())
        throw_compilation_error("Too many arguments to if");
    cond = seq_first(*cond);
    compile_value(no_recur(scope), *cond);
    auto bnil_offset = append_branch(code, vm::BNIL, 0);
    then = seq_first(*then);
    compile_value(scope, *then, wrap_try);
    auto br_offset = append_branch(code, vm::BR, 0);
    set_branch_target(code, bnil_offset, code.size());
    else_ = seq_first(*else_);
    compile_value(scope, *else_, wrap_try);
    set_branch_target(code, br_offset, code.size());
}

void Compiler::compile_do(Scope scope, Value val, bool wrap_try)
{
    if (seq_next(val).value().is_nil())
        return append(code, vm::CNIL);
    Root s{val}, e;
    auto no_recur_scope = no_recur(scope);
    for (s = seq_next(val); seq_next(*s).value(); s = seq_next(*s))
    {
        e = seq_first(*s);
        compile_value(no_recur_scope, *e, wrap_try);
        append(code, vm::POP);
    }
    e = seq_first(*s);
    compile_value(scope, *e, wrap_try);
}

void Compiler::compile_quote(Value form)
{
    check_compiletime_arity(QUOTE, form, 1, seq_count(form) - 1);
    Root expr{seq_next(form)};
    expr = seq_first(*expr);
    compile_const(*expr);
}

void Compiler::update_locals_size(Scope scope)
{
    locals_size = std::max(locals_size, std::int16_t(scope.locals_size));
}

std::pair<Compiler::Scope, std::int16_t> add_local(Compiler::Scope scope, Value sym, Root& holder)
{
    if (scope.locals_size == MAX_LOCALS)
        throw_compilation_error("Too many locals: " + std::to_string(scope.locals_size + 1));
    Root index{create_int64(scope.locals_size)};
    holder = persistent_hash_map_assoc(scope.locals, sym, *index);
    scope.locals = *holder;
    scope.locals_size++;
    return {scope, scope.locals_size - 1};
}

Force check_let_bindings(Value tag, Value form)
{
    check_compiletime_arity(tag, form, 2, seq_count(form) - 1);
    Root bindings{seq_next(form)};
    bindings = seq_first(*bindings);
    if (!get_value_type(*bindings).is(*type::Array))
        throw_compilation_error("Bad binding form, expected vector");
    if (get_array_size(*bindings) % 2)
        throw_compilation_error("Bad binding form, expected matched symbol expression pairs");
    return *bindings;
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
        compile_value(no_recur(scope), get_array_elem(bindings, i + 1));
        std::tie(scope, index) = add_local(scope, sym, llocals);
        append_STL(code, index);
    }
    update_locals_size(scope);
    return scope;
}

void Compiler::compile_let(Scope scope, Value form, bool wrap_try)
{
    Root bindings{check_let_bindings(LET, form)};
    Root llocals;
    scope = compile_let_bindings(scope, *bindings, llocals);

    Root expr{seq_next(form)};
    expr = seq_next(*expr);
    expr = seq_first(*expr);
    compile_value(scope, *expr, wrap_try);
}

void Compiler::compile_loop(Scope scope, Value form)
{
    Root bindings{check_let_bindings(LOOP, form)};
    Root llocals;
    scope = compile_let_bindings(scope, *bindings, llocals);

    auto loop_locals_size = get_array_size(*bindings) / 2;
    scope.recur_arity = loop_locals_size;
    scope.recur_locals_index = scope.locals_size - loop_locals_size;
    scope.recur_start_offset = code.size();

    Root expr{seq_next(form)};
    expr = seq_next(*expr);
    expr = seq_first(*expr);
    compile_value(scope, *expr);
}

void Compiler::compile_recur(Scope scope, Value form_)
{
    if (scope.recur_start_offset < 0)
        throw_compilation_error("Can only recur from tail position");
    Root form{seq_next(form_)};
    auto size = *form ? seq_count(*form) : 0;
    if (size != scope.recur_arity)
        throw_compilation_error("Mismatched argument count to recur, expected: " + std::to_string(scope.recur_arity) +
                                " args, got: " + std::to_string(size));
    for (Root e; *form; form = seq_next(*form))
    {
        e = seq_first(*form);
        compile_value(scope, *e);
    }
    for (Int64 i = 0; i < size; ++i)
        append_STL(code, scope.recur_locals_index + (size - i - 1));
    append_branch(code, vm::BR, scope.recur_start_offset - 3 - Int64(code.size()));
}

Int64 get_vector_const_prefix_len(Value val);
Force get_hash_set_const_subset(Value val);
Force get_hash_map_const_submap(Value val);

bool is_const(Value val)
{
    auto tag = get_value_tag(val);
    if (tag == tag::OBJECT)
    {
        auto type = get_value_type(val);
        if (type.is(*type::Array))
            return get_vector_const_prefix_len(val) == get_array_size(val);
        if (type.is(*type::ArraySet))
        {
            Root ss{get_hash_set_const_subset(val)};
            return get_array_set_size(*ss) == get_array_set_size(val);
        }
        if (is_map(val))
        {
            Root sm{get_hash_map_const_submap(val)};
            return count(*sm) == count(val);
        }
        return false;
    }

    return tag != tag::SYMBOL;
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
    scope = no_recur(scope);
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
    scope = no_recur(scope);
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
    for (Root s{seq(val)}; *s; s = seq_next(*s))
    {
        Root kv{seq_first(*s)};
        auto k = get_array_elem(*kv, 0);
        auto v = get_array_elem(*kv, 1);
        if (is_const(k) && is_const(v))
            sm = persistent_hash_map_assoc(*sm, k, v);
    }
    return *sm;
}

void Compiler::compile_hash_map(Scope scope, Value val)
{
    scope = no_recur(scope);
    Root submap{get_hash_map_const_submap(val)};
    auto size = count(val);
    auto submap_size = get_persistent_hash_map_size(*submap);
    if (submap_size == size)
        return compile_const(val);
    for (Int64 i = submap_size; i < size; ++i)
        compile_const(*rt::persistent_hash_map_assoc);
    compile_const(*submap);
    for (Root s{seq(val)}; *s; s = seq_next(*s))
    {
        Root kv{seq_first(*s)};
        auto k = get_array_elem(*kv, 0);
        auto v = get_array_elem(*kv, 1);
        if (!persistent_hash_map_contains(*submap, k))
        {
            compile_value(scope, k);
            compile_value(scope, v);
            append(code, vm::CALL, 3);
        }
    }
}

void Compiler::compile_def(Scope scope, Value form_)
{
    scope = no_recur(scope);
    Root form{seq_next(form_)};
    if (!*form)
        throw_compilation_error("Too few arguments to def");
    Root name{seq_first(*form)};
    form = seq_next(*form);
    Root meta;
    if (is_map(*name))
    {
        if (!*form)
            throw_compilation_error("Too few arguments to def");
        meta = *name;
        name = seq_first(*form);
        form = seq_next(*form);
    }
    if (get_value_tag(*name) != tag::SYMBOL)
        throw_compilation_error("First argument to def must be a Symbol");
    Root val{*form ? seq_first(*form) : nil};
    if (*form && seq_next(*form).value())
        throw_compilation_error("Too many arguments to def");
    auto current_ns_name = get_symbol_name(ns_name(*rt::current_ns));
    auto sym_ns = get_symbol_namespace(*name);
    if (sym_ns && sym_ns != current_ns_name)
        throw_compilation_error(maybe_resolve_var(*name) ?
                                "Can't create defs outside of current ns" :
                                "Can't refer to qualified var that doesn't exist");
    auto sym_name = get_symbol_name(*name);
    name = create_symbol(
        {get_string_ptr(current_ns_name), get_string_len(current_ns_name)},
        {get_string_ptr(sym_name), get_string_len(sym_name)});
    auto var = define(*name, nil, *meta);
    compile_const(var);
    compile_value(scope, *val);
    compile_value(scope, *meta);
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

void Compiler::compile_throw(Scope scope, Value form_)
{
    Root form{seq_next(form_)};
    if (!*form)
        throw_compilation_error("Too few arguments to throw, expected a single value");
    Root expr{seq_first(*form)};
    compile_value(scope, *expr);
    if (seq_next(*form).value())
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

void Compiler::compile_try(Scope scope, Value form_)
{
    scope.recur_start_offset = -1;
    auto start_offset = code.size();
    Root form{seq_next(form_)};
    Root expr{*form ? seq_first(*form) : nil};
    compile_value(scope, *expr, false);
    Root handler_{*form ? seq_next(*form) : nil};
    if (!*handler_)
        return;
    if (seq_next(*handler_).value())
        throw_compilation_error("Too many expressions in " + to_string(TRY));
    handler_ = *handler_ ? seq_first(*handler_) : nil;
    Root tag{is_seq(*handler_) ? seq_first(*handler_) : nil};
    if (*tag == CATCH)
    {
        Root catch_{seq_next(*handler_)};
        if (!*catch_)
            throw_compilation_error("missing exception type in " + to_string(CATCH));
        Root type_sym{seq_first(*catch_)};
        catch_ = seq_next(*catch_);
        if (!*catch_)
            throw_compilation_error("missing exception binding in " + to_string(CATCH));
        Root local{seq_first(*catch_)};
        catch_ = seq_next(*catch_);
        if (!*catch_)
            throw_compilation_error("missing catch* body");
        Root expr{seq_first(*catch_)};
        if (seq_next(*catch_).value())
            throw_compilation_error("Too many expressions in " + to_string(CATCH) + ", expected one");
        Root rlocal;
        Int64 index{};
        std::tie(scope, index) = add_local(scope, *local, rlocal);
        update_locals_size(scope);
        auto br_offset = append_branch(code, vm::BR, 0);
        auto type = maybe_resolve_var(*type_sym);
        if (!type)
            throw_compilation_error("unable to resolve symbol: " + to_string(*type_sym));
        add_exception_handler(start_offset, br_offset, code.size(), get_var_value(type));
        append_STL(code, index);
        compile_value(scope, *expr);
        set_branch_target(code, br_offset, code.size());
    }
    else if (*tag == FINALLY)
    {
        handler_ = seq_next(*handler_);
        if (!*handler_)
            throw_compilation_error("missing " + to_string(FINALLY) + " body");
        Root expr{seq_first(*handler_)};
        if (seq_next(*handler_).value())
            throw_compilation_error("Too many expressions in " + to_string(FINALLY) + ", expected one");
        auto end_offset = code.size();
        compile_value(scope, *expr);
        append(code, vm::POP);
        auto br_offset = append_branch(code, vm::BR, 0);
        add_exception_handler(start_offset, end_offset, code.size(), nil);
        Root rlocal;
        Int64 index{};
        std::tie(scope, index) = add_local(scope, nil, rlocal);
        update_locals_size(scope);
        append_STL(code, index);
        compile_value(scope, *expr);
        append(code, vm::POP);
        append_LDL(code, index);
        append(code, vm::THROW);
        set_branch_target(code, br_offset, code.size());
    }
    else
        throw_compilation_error("expected " + to_string(CATCH) + " or " + to_string(FINALLY) + " block in " + to_string(TRY));
}

void Compiler::compile_try_wrapped(Scope scope, Value form)
{
    Root call{create_cons(form, nil)};
    call = create_cons(*EMPTY_VECTOR, *call);
    call = create_cons(FN, *call);
    call = create_cons(*call, nil);
    compile_value(scope, *call);
}

Value maybe_resolved_var_name(Value sym)
{
    if (get_value_tag(sym) != tag::SYMBOL)
        return nil;
    auto v = maybe_resolve_var(sym);
    return v ? get_var_name(v) : nil;
}

void Compiler::compile_value(Scope scope, Value val, bool wrap_try)
{
    Root xval{macroexpand(val)};
    val = *xval;

    if (get_value_tag(val) == tag::SYMBOL)
        return compile_symbol(scope, val);

    auto vtype = get_value_type(val);
    if (isa(vtype, *type::Sequence) && seq(val).value())
    {
        Root first{seq_first(val)};
        if (*first == IF)
            return compile_if(scope, val, wrap_try);
        if (*first == DO)
            return compile_do(scope, val, wrap_try);
        if (*first == QUOTE)
            return compile_quote(val);
        if (*first == LET)
            return compile_let(scope, val, wrap_try);
        if (*first == RECUR)
            return compile_recur(scope, val);
        if (*first == LOOP)
            return compile_loop(scope, val);
        if (*first == DEF)
            return compile_def(scope, val);
        if (maybe_resolved_var_name(*first) == APPLY)
            return compile_apply(scope, val);
        if (*first == FN)
            return compile_fn(scope, val);
        if (*first == THROW)
            return compile_throw(scope, val);
        if (*first == TRY)
            return wrap_try ? compile_try_wrapped(scope, val) : compile_try(scope, val);

        return compile_call(scope, val);
    }

    if (vtype.is(*type::Array))
        return compile_vector(scope, val);
    if (vtype.is(*type::ArraySet))
        return compile_hash_set(scope, val);
    if (is_map(val))
        return compile_hash_map(scope, val);

    compile_const(val);
}

Compiler::Scope create_fn_body_scope(Value form, Value env, Value locals, Value parent_locals)
{
    Root params{seq_first(form)};
    auto recur_arity = std::uint16_t(std::abs(get_arity(*params)));
    return {env, parent_locals, locals, recur_arity, std::int16_t(-recur_arity)};
}

Force compile_fn_body(Value name, Value form, Value env, Value parent_locals, Root& used_locals)
{
    Compiler c(*used_locals);
    Root val{seq_next(form)};
    if (*val && seq_next(*val).value())
        throw_compilation_error("Too many forms passed to " + to_string(FN));
    val = *val ? seq_first(*val) : nil;
    Root params{seq_first(form)};
    Root locals{create_locals(name, *params)};
    auto scope = create_fn_body_scope(form, env, *locals, parent_locals);
    c.compile_value(scope, *val, false);
    auto used_locals_size = get_int64_value(get_transient_array_size(*c.local_refs));
    auto consts_size = get_int64_value(get_transient_array_size(*c.consts));
    for (auto off : c.local_ref_offsets)
        set_i16(c.code, off, get_i16(c.code, off) + consts_size);
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
    auto size = get_int64_value(get_transient_array_size(*consts));
    if (size > MAX_CONSTS)
        throw_compilation_error("Too many constants: " + std::to_string(size));
    consts = size > 0 ? transient_array_persistent(*consts) : nil;

    return create_bytecode_fn_body(*consts, get_bytecode_fn_body_vars(body), get_bytecode_fn_body_exception_table(body), get_bytecode_fn_body_locals_size(body), get_bytecode_fn_body_bytes(body), get_bytecode_fn_body_bytes_size(body));
}

Force compile_ifn(Value form, Value env, Value parent_locals, Root& used_locals)
{
    if (!is_seq(form))
        throw_compilation_error("form must be a sequence");
    Root first{seq_first(form)};
    if (*first != FN)
        throw_compilation_error("form must start with fn*");
    Root rest{seq_next(form)};
    Value name;
    first = seq_first(*rest);
    if (*rest && get_value_tag(*first) == tag::SYMBOL)
    {
        name = *first;
        rest = seq_next(*rest);
    }
    if (rest->is_nil())
        return create_bytecode_fn(name, nullptr, nullptr, 0);
    first = seq_first(*rest);
    Root forms{is_seq(*first) ? *rest : create_cons(*rest, nil)};
    auto count = seq_count(*forms);
    Roots rbodies(count);
    std::vector<std::pair<Int64, Value>> arities_and_bodies;
    arities_and_bodies.reserve(count);
    used_locals = nil;
    for (Int64 i = 0; i < count; ++i)
    {
        Root form{seq_first(*forms)};
        rbodies.set(i, compile_fn_body(name, *form, env, parent_locals, used_locals));
        Root params{seq_first(*form)};
        arities_and_bodies.emplace_back(get_arity(*params), rbodies[i]);
        forms = seq_next(*forms);
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
