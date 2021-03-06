#include "compile.hpp"
#include "bytecode_fn.hpp"
#include "global.hpp"
#include "array.hpp"
#include "byte_array.hpp"
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

Force compile_ifn(Value form, Value parent_locals, Root& used_locals);

struct Compiler
{
    struct Scope
    {
        Value parent_locals;
        Value locals;
        std::uint16_t recur_arity{};
        std::int16_t recur_locals_index{};
        Int64 recur_start_offset{};
        std::uint16_t locals_size{};
        Int64 stack_depth{};
    };

    std::vector<vm::Byte> code;
    std::int16_t locals_size = 0;
    Root consts{*EMPTY_HASH_MAP};
    Root vars{transient_array(*EMPTY_VECTOR)};
    Root parent_local_refs;
    std::vector<Int64> et_entries;
    std::vector<Value> et_types;

    Compiler(Value parent_local_refs) : parent_local_refs(transient_array(parent_local_refs ? parent_local_refs : *EMPTY_VECTOR)) { }

    void compile_symbol(const Scope& scope, Value sym);
    void compile_const(Value c);
    Int64 add_local_ref(Value sym);
    void compile_local_ref(Value sym);
    void compile_call(Scope scope, Value val);
    void compile_apply(Scope scope, Value form);
    void compile_if(Scope scope, Value val);
    void compile_do(Scope scope, Value val);
    void compile_quote(Value form);
    void update_locals_size(Scope scope);
    Scope compile_let_bindings(Scope scope, Value bindings, Root& llocals);
    void compile_let(Scope scope, Value form);
    void compile_loop(Scope scope, Value form);
    void compile_recur(Scope scope, Value form);
    void compile_vector(Scope scope, Value val);
    void compile_hash_set(Scope scope, Value val);
    void compile_hash_map(Scope scope, Value val);
    void compile_def(Scope scope, Value form);
    void compile_fn(Scope scope, Value form);
    void compile_throw(Scope scope, Value form);
    void add_exception_handler(Int64 start, Int64 end, Int64 handler, Int64 stack_size, Value type);
    void compile_try(Scope scope, Value form);
    void compile_dot(Scope scope, Value form);
    void compile_value(Scope scope, Value val);
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

void append_LDCV(std::vector<vm::Byte>& v, std::int16_t n)
{
    append(v, vm::LDCV);
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
    auto n = get_transient_array_size(*vars);
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
    auto n = get_persistent_hash_map_size(*consts);
    if (n == MAX_CONSTS)
        throw_compilation_error("Too many constants: " + std::to_string(n + 1));
    std::array<Value, 2> atc{{get_value_type(c), c}};
    Root tc{create_array(atc.data(), atc.size())};
    if (Value index = persistent_hash_map_get(*consts, *tc))
        return get_int64_value(index);
    Root rn{create_int64(n)};
    consts = persistent_hash_map_assoc(*consts, *tc, *rn);
    return n;
}

Force serialize_consts(Value consts)
{
    auto n = get_persistent_hash_map_size(consts);
    Root serialized{create_array(nullptr, n)};
    serialized = transient_array(*serialized);
    for (Root s{persistent_hash_map_seq(consts)}; *s; s = get_persistent_hash_map_seq_next(*s))
    {
        auto kv = get_persistent_hash_map_seq_first(*s);
        transient_array_assoc_elem(*serialized, get_int64_value(get_array_elem_unchecked(kv, 1)), get_array_elem_unchecked(get_array_elem_unchecked(kv, 0), 1));
    }
    return transient_array_persistent(*serialized);
}

Int64 Compiler::add_local_ref(Value sym)
{
    auto n = get_transient_array_size(*parent_local_refs);
    for (Int64 i = 0; i < n; ++i)
        if (get_transient_array_elem(*parent_local_refs, i) == sym)
            return i;

    parent_local_refs = transient_array_conj(*parent_local_refs, sym);
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
        locals = map_assoc(*locals, name, *index);
    }
    for (Int64 i = 0; i < fixed_arity; ++i)
    {
        auto sym = get_array_elem(params, i);
        if (!get_value_type(sym).is(*type::Symbol))
            throw_compilation_error(to_string(FN) + " params must be symbols");
        if (get_symbol_namespace(sym))
            throw_compilation_error("Can't use qualified name as parameter: " + to_string(sym));
        index = create_int64(i - total_arity);
        locals = map_assoc(*locals, sym, *index);
    }
    if (arity < 0)
        locals = map_assoc(*locals, get_array_elem(params, total_arity), *NEG_ONE);
    return *locals;
}

void Compiler::compile_symbol(const Scope& scope, Value sym)
{
    if (auto index = map_get(scope.locals, sym))
        return append_LDL(code, get_int64_value(index));
    if (map_contains(scope.parent_locals, sym))
        return compile_local_ref(sym);

    auto v = maybe_resolve_var(sym);
    if (!v)
        throw_compilation_error("unable to resolve symbol: " + to_string(sym));
    if (!get_ns(namespace_symbol(get_var_name(v))).is(*rt::current_ns) && !is_var_public(v))
        throw_compilation_error("var: " + to_string(get_var_name(v)) + " is not public");
    if (is_var_macro(v))
        throw_compilation_error("Can't take value of a macro: " + to_string(v));
    auto vi = add_var(vars, v);
    append(code, is_var_dynamic(v) ? vm::LDDV : vm::LDV);
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
    append_LDCV(code, ri);
}

void Compiler::compile_call(Scope scope, Value val)
{
    scope = no_recur(scope);
    std::uint32_t n = 0;
    for (Root e{val}, v; *e; e = seq_next(*e))
    {
        v = seq_first(*e);
        compile_value(scope, *v);
        scope.stack_depth++;
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
        scope.stack_depth++;
    }
    append(code, vm::APPLY, size - 3);
}


void Compiler::compile_if(Scope scope, Value val)
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
    compile_value(scope, *then);
    auto br_offset = append_branch(code, vm::BR, 0);
    set_branch_target(code, bnil_offset, code.size());
    else_ = seq_first(*else_);
    compile_value(scope, *else_);
    set_branch_target(code, br_offset, code.size());
}

void Compiler::compile_do(Scope scope, Value val)
{
    if (seq_next(val).value().is_nil())
        return append(code, vm::CNIL);
    Root s{val}, e;
    auto no_recur_scope = no_recur(scope);
    for (s = seq_next(val); seq_next(*s).value(); s = seq_next(*s))
    {
        e = seq_first(*s);
        compile_value(no_recur_scope, *e);
        append(code, vm::POP);
    }
    e = seq_first(*s);
    compile_value(scope, *e);
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
    holder = map_assoc(scope.locals, sym, *index);
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

void Compiler::compile_let(Scope scope, Value form)
{
    Root bindings{check_let_bindings(LET, form)};
    Root llocals;
    scope = compile_let_bindings(scope, *bindings, llocals);

    Root expr{seq_next(form)};
    expr = seq_next(*expr);
    expr = seq_first(*expr);
    compile_value(scope, *expr);
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
        if (is_set(val))
        {
            Root ss{get_hash_set_const_subset(val)};
            return set_count(*ss) == set_count(val);
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
    scope.stack_depth += size - prefix_len + 2;
    for (Int64 i = prefix_len; i < size; ++i)
    {
        compile_value(scope, get_array_elem(val, i));
        append(code, vm::CALL, 2);
        scope.stack_depth--;
    }
    append(code, vm::CALL, 1);
}

Force get_hash_set_const_subset(Value val)
{
    Root ss{*EMPTY_SET};
    for (Root s{seq(val)}; *s; s = seq_next(*s))
    {
        Root e{seq_first(*s)};
        if (is_const(*e))
            ss = set_conj(*ss, *e);
    }
    return *ss;
}

void Compiler::compile_hash_set(Scope scope, Value val)
{
    scope = no_recur(scope);
    Root subset{get_hash_set_const_subset(val)};
    auto size = set_count(val);
    auto subset_size = set_count(*subset);
    if (subset_size == size)
        return compile_const(val);
    for (Int64 i = subset_size; i < size; ++i)
        compile_const(*rt::set_conj);
    compile_const(*subset);
    scope.stack_depth = size - subset_size + 1;
    for (Root s{seq(val)}; *s; s = seq_next(*s))
    {
        Root e{seq_first(*s)};
        if (!set_contains(*subset, *e))
        {
            compile_value(scope, *e);
            append(code, vm::CALL, 2);
            scope.stack_depth--;
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
            sm = map_assoc(*sm, k, v);
    }
    return *sm;
}

void Compiler::compile_hash_map(Scope scope, Value val)
{
    scope = no_recur(scope);
    Root submap{get_hash_map_const_submap(val)};
    auto size = count(val);
    auto submap_size = map_count(*submap);
    if (submap_size == size)
        return compile_const(val);
    for (Int64 i = submap_size; i < size; ++i)
        compile_const(*rt::map_assoc);
    compile_const(*submap);
    scope.stack_depth = size - submap_size + 1;
    for (Root s{seq(val)}; *s; s = seq_next(*s))
    {
        Root kv{seq_first(*s)};
        auto k = get_array_elem(*kv, 0);
        auto v = get_array_elem(*kv, 1);
        if (!map_contains(*submap, k))
        {
            compile_value(scope, k);
            scope.stack_depth++;
            compile_value(scope, v);
            append(code, vm::CALL, 3);
            scope.stack_depth -= 2;
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
        meta = eval(*meta);
        name = seq_first(*form);
        form = seq_next(*form);
    }
    if (get_value_tag(*name) != tag::SYMBOL)
        throw_compilation_error("First argument to def must be a Symbol");
    bool has_val = !form->is_nil();
    Root val{has_val ? seq_first(*form) : nil};
    if (*form && seq_next(*form).value())
        throw_compilation_error("Too many arguments to def");
    auto current_ns_name = get_symbol_name(ns_name(*rt::current_ns));
    auto sym_ns = get_symbol_namespace(*name);
    auto var = maybe_resolve_var(*name);
    if (sym_ns && sym_ns != current_ns_name)
        throw_compilation_error(var ?
                                "Can't create defs outside of current ns" :
                                "Can't refer to qualified var that doesn't exist");
    auto sym_name = get_symbol_name(*name);
    name = create_symbol(
        {get_string_ptr(current_ns_name), get_string_size(current_ns_name)},
        {get_string_ptr(sym_name), get_string_size(sym_name)});
    if (!var)
        var = define(*name, nil, *meta);
    compile_const(var);
    scope.stack_depth++;
    if (has_val)
    {
        compile_value(scope, *val);
        scope.stack_depth++;
        append(code, vm::STVV);
    }
    if (*meta)
    {
        compile_const(*meta);
        append(code, vm::STVM);
    }
}

void Compiler::compile_fn(Scope scope, Value form)
{
    Root used_locals;
    Root parent_locals{map_merge(scope.parent_locals, scope.locals)};
    Root fn{compile_ifn(form, *parent_locals, used_locals)};
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

void Compiler::add_exception_handler(Int64 start, Int64 end, Int64 handler, Int64 stack_size, Value type)
{
    et_entries.push_back(start);
    et_entries.push_back(end);
    et_entries.push_back(handler);
    et_entries.push_back(stack_size);
    et_types.push_back(type);
}

void Compiler::compile_try(Scope scope, Value form_)
{
    scope.recur_start_offset = -1;
    auto start_offset = code.size();
    Root form{seq_next(form_)};
    Root expr{*form ? seq_first(*form) : nil};
    compile_value(scope, *expr);
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
        add_exception_handler(start_offset, br_offset, code.size(), scope.stack_depth, get_var_value(type));
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
        add_exception_handler(start_offset, end_offset, code.size(), scope.stack_depth, nil);
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

void Compiler::compile_dot(Scope scope, Value form)
{
    Root obj{seq_next(form)};
    Root field{seq_next(*obj)};
    Root next{seq_next(*field)};
    obj = seq_first(*obj);
    field = seq_first(*field);
    if (*next || get_value_tag(*field) != tag::SYMBOL)
        throw_compilation_error("Malformed member expression");
    auto field_name = get_symbol_name(*field);
    if (get_string_size(field_name) < 2 || get_string_ptr(field_name)[0] != '-')
        throw_compilation_error("Malformed member expression");
    field_name = create_symbol(get_string_ptr(field_name) + 1);
    compile_value(scope, *obj);
    compile_const(field_name);
    append(code, vm::LDDF);
}

Value maybe_resolved_var_name(Value sym)
{
    if (get_value_tag(sym) != tag::SYMBOL)
        return nil;
    auto v = maybe_resolve_var(sym);
    return v ? get_var_name(v) : nil;
}

void Compiler::compile_value(Scope scope, Value val)
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
            return compile_if(scope, val);
        if (*first == DO)
            return compile_do(scope, val);
        if (*first == QUOTE)
            return compile_quote(val);
        if (*first == LET)
            return compile_let(scope, val);
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
            return compile_try(scope, val);
        if (*first == DOT)
            return compile_dot(scope, val);

        return compile_call(scope, val);
    }

    if (vtype.is(*type::Array))
        return compile_vector(scope, val);
    if (is_set(val))
        return compile_hash_set(scope, val);
    if (is_map(val))
        return compile_hash_map(scope, val);

    compile_const(val);
}

Compiler::Scope create_fn_body_scope(Value form, Value locals, Value parent_locals)
{
    Root params{seq_first(form)};
    auto recur_arity = std::uint16_t(std::abs(get_arity(*params)));
    return {parent_locals, locals, recur_arity, std::int16_t(-recur_arity)};
}

Force compile_fn_body(Value name, Value form, Value parent_locals, Root& used_locals)
{
    Compiler c(*used_locals);
    Root val{seq_next(form)};
    if (*val && seq_next(*val).value())
        throw_compilation_error("Too many forms passed to " + to_string(FN));
    val = *val ? seq_first(*val) : nil;
    Root params{seq_first(form)};
    Root locals{create_locals(name, *params)};
    auto arity = get_arity(*params);
    auto scope = create_fn_body_scope(form, *locals, parent_locals);
    c.compile_value(scope, *val);
    auto used_locals_size = get_transient_array_size(*c.parent_local_refs);
    auto consts_size = get_persistent_hash_map_size(*c.consts);
    Root consts{consts_size > 0 ? serialize_consts(*c.consts) : nil};
    Root vars{get_transient_array_size(*c.vars) > 0 ? transient_array_persistent(*c.vars) : nil};
    used_locals = used_locals_size > 0 ? transient_array_persistent(*c.parent_local_refs) : nil;
    Root exception_table{!c.et_types.empty() ? create_bytecode_fn_exception_table(c.et_entries.data(), c.et_types.data(), c.et_types.size()) : nil};
    return create_bytecode_fn_body(arity, *consts, *vars, nil, *exception_table, c.locals_size, c.code.data(), c.code.size());
}

Force create_fn(Value name, std::vector<Value> bodies, Value ast)
{
    std::sort(
        begin(bodies), end(bodies),
        [](auto& l, auto& r) { return std::abs(get_bytecode_fn_body_arity(l)) < std::abs(get_bytecode_fn_body_arity(r)); });
    return create_bytecode_fn(name, bodies.data(), bodies.size(), ast);
}

Force create_open_fn(Value name, std::vector<Value> bodies, Value ast)
{
    std::sort(
        begin(bodies), end(bodies),
        [](auto& l, auto& r) { return std::abs(get_bytecode_fn_body_arity(l)) < std::abs(get_bytecode_fn_body_arity(r)); });
    return create_open_bytecode_fn(name, bodies.data(), bodies.size(), ast);
}

Force compile_ifn(Value form, Value parent_locals, Root& used_locals)
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
        return create_bytecode_fn(name, nullptr, 0, nil);
    first = seq_first(*rest);
    Root forms{is_seq(*first) ? *rest : create_cons(*rest, nil)};
    auto count = seq_count(*forms);
    Roots rbodies(count);
    std::vector<Value> bodies;
    bodies.reserve(count);
    used_locals = nil;
    for (Int64 i = 0; i < count; ++i)
    {
        Root form{seq_first(*forms)};
        rbodies.set(i, compile_fn_body(name, *form, parent_locals, used_locals));
        Root params{seq_first(*form)};
        bodies.push_back(rbodies[i]);
        forms = seq_next(*forms);
    }
    return (*used_locals ? create_open_fn : create_fn)(name, std::move(bodies), nil);
}

Force deserialize_exception_table(Value et)
{
    if (!et)
        return nil;
    Value START_OFFSET = create_keyword("start-offset");
    Value END_OFFSET = create_keyword("end-offset");
    Value HANDLER_OFFSET = create_keyword("handler-offset");
    Value STACK_SIZE = create_keyword("stack-size");
    Value TYPE = create_keyword("type");
    Root det{transient_array(*EMPTY_VECTOR)};
    for (Int64 i = 0; i < get_bytecode_fn_exception_table_size(et); ++i)
    {
        Root soff{create_int64(get_bytecode_fn_exception_table_start_offset(et, i))};
        Root eoff{create_int64(get_bytecode_fn_exception_table_end_offset(et, i))};
        Root hoff{create_int64(get_bytecode_fn_exception_table_handler_offset(et, i))};
        Root ssize{create_int64(get_bytecode_fn_exception_table_stack_size(et, i))};
        Root entry{*EMPTY_MAP};
        entry = map_assoc(*entry, START_OFFSET, *soff);
        entry = map_assoc(*entry, END_OFFSET, *eoff);
        entry = map_assoc(*entry, HANDLER_OFFSET, *hoff);
        entry = map_assoc(*entry, STACK_SIZE, *ssize);
        entry = map_assoc(*entry, TYPE, get_bytecode_fn_exception_table_type(et, i));
        det = transient_array_conj(*det, *entry);
    }
    return transient_array_persistent(*det);
}

Force deserialize_bytecode(const vm::Byte *bc, Int64 size)
{
    Root dbc{transient_array(*EMPTY_VECTOR)};
    Root b;
    for (Int64 i = 0; i < size; ++i)
    {
        b = create_int64(bc[i] & 0xff);
        dbc = transient_array_conj(*dbc, *b);
    }
    return transient_array_persistent(*dbc);
}

}

Force compile_fn(Value form)
{
    Root used_locals;
    return compile_ifn(form, *EMPTY_MAP, used_locals);
}

Force serialize_fn(Value fn)
{
    Value ARITY = create_keyword("arity");
    Value VARARG = create_keyword("vararg");
    Value LOCALS_SIZE = create_keyword("locals-size");
    Value CONSTS = create_keyword("consts");
    Value VARS = create_keyword("vars");
    Value EXCEPTION_TABLE = create_keyword("exception-table");
    Value START_OFFSET = create_keyword("start-offset");
    Value END_OFFSET = create_keyword("end-offset");
    Value HANDLER_OFFSET = create_keyword("handler-offset");
    Value STACK_SIZE = create_keyword("stack-size");
    Value TYPE = create_keyword("type");
    Value BYTECODE = create_keyword("bytecode");
    Value fn_bodies = map_get(fn, create_keyword("bodies"));
    Value name = map_get(fn, create_keyword("name"));
    Value ast_str = map_get(fn, create_keyword("ast-str"));
    bool has_closed_locals = !map_get(fn, create_keyword("closed-parent-locals")).is_nil();
    Value dep_vars = map_get(fn, create_keyword("dep-vars"));
    Value dep_fns = map_get(fn, create_keyword("dep-fns"));
    auto body_count = count(fn_bodies);
    std::vector<Value> bodies;
    Roots body_roots(body_count);
    Root body;
    for (Root s{seq(fn_bodies)}; *s; s = seq_next(*s))
    {
        body = seq_first(*s);
        Value fn_et = map_get(*body, EXCEPTION_TABLE);
        std::vector<Int64> et_entries;
        std::vector<Value> et_types;
        for (Int64 i = 0, size = get_array_size(fn_et); i < size; ++i)
        {
            auto e = get_array_elem_unchecked(fn_et, i);
            et_entries.push_back(get_int64_value(map_get(e, START_OFFSET)));
            et_entries.push_back(get_int64_value(map_get(e, END_OFFSET)));
            et_entries.push_back(get_int64_value(map_get(e, HANDLER_OFFSET)));
            et_entries.push_back(get_int64_value(map_get(e, STACK_SIZE)));
            et_types.push_back(map_get(e, TYPE));
        }
        Root exception_table;
        if (!et_entries.empty())
            exception_table = create_bytecode_fn_exception_table(et_entries.data(), et_types.data(), et_types.size());
        Value fn_bytecode = map_get(*body, BYTECODE);
        std::vector<vm::Byte> bytecode;
        check_type(":bytecode", fn_bytecode, *type::ByteArray);
        for (Int64 i = 0, size = get_byte_array_size(fn_bytecode); i < size; ++i)
            bytecode.push_back(get_int64_value(get_byte_array_elem_unchecked(fn_bytecode, i)));
        auto arity = get_int64_value(map_get(*body, ARITY));
        arity = map_get(*body, VARARG) ? -arity : arity;
        body_roots.set(bodies.size(),
                       create_bytecode_fn_body(arity,
                                               map_get(*body, CONSTS),
                                               map_get(*body, VARS),
                                               nil,
                                               *exception_table,
                                               map_contains(*body, LOCALS_SIZE) ? get_int64_value(map_get(*body, LOCALS_SIZE)) : 0,
                                               bytecode.data(), bytecode.size()));
        bodies.push_back(body_roots[bodies.size()]);
    }

    Root sfn{(has_closed_locals ? create_open_fn : create_fn)(name, bodies, ast_str)};
    for (Root s{seq(dep_vars)}; *s; s = seq_next(*s))
    {
        Root var{seq_first(*s)};
        add_var_fn_dep(*var, *sfn);
    }
    for (Root s{seq(dep_fns)}; *s; s = seq_next(*s))
    {
        Root fn{seq_first(*s)};
        add_bytecode_fn_fn_dep(*fn, *sfn);
    }
    return *sfn;
}

Force deserialize_fn(Value fn)
{
    if (!fn)
        return nil;
    Value ARITY = create_keyword("arity");
    Value VARARG = create_keyword("vararg");
    Value LOCALS_SIZE = create_keyword("locals-size");
    Value CONSTS = create_keyword("consts");
    Value VARS = create_keyword("vars");
    Value EXCEPTION_TABLE = create_keyword("exception-table");
    Value BYTECODE = create_keyword("bytecode");
    Root dfn{*EMPTY_MAP};
    if (get_bytecode_fn_name(fn))
        dfn = map_assoc(*dfn, create_keyword("name"), get_bytecode_fn_name(fn));
    Root bodies{*EMPTY_VECTOR};
    for (Int64 i = 0; i < get_bytecode_fn_size(fn); ++i)
    {
        auto arity = get_bytecode_fn_arity(fn, i);
        Root darity{create_int64(arity < 0 ? -arity : arity)};
        Root dbody{*EMPTY_MAP};
        auto body = get_bytecode_fn_body(fn, i);
        Root locals_size{create_int64(get_bytecode_fn_body_locals_size(body))};
        if (get_bytecode_fn_body_locals_size(body) > 0)
            dbody = map_assoc(*dbody, LOCALS_SIZE, *locals_size);
        dbody = map_assoc(*dbody, ARITY, *darity);
        if (arity < 0)
            dbody = map_assoc(*dbody, VARARG, TRUE);
        if (get_bytecode_fn_body_consts(body))
            dbody = map_assoc(*dbody, CONSTS, get_bytecode_fn_body_consts(body));
        if (get_bytecode_fn_body_vars(body))
            dbody = map_assoc(*dbody, VARS, get_bytecode_fn_body_vars(body));
        Root dbs{deserialize_bytecode(get_bytecode_fn_body_bytes(body), get_bytecode_fn_body_bytes_size(body))};
        dbody = map_assoc(*dbody, BYTECODE, *dbs);
        Root det{deserialize_exception_table(get_bytecode_fn_body_exception_table(body))};
        if (*det)
            dbody = map_assoc(*dbody, EXCEPTION_TABLE, *det);
        bodies = array_conj(*bodies, *dbody);
    }
    dfn = map_assoc(*dfn, create_keyword("bodies"), *bodies);
    return *dfn;
}

}
