#include "multimethod.hpp"
#include "namespace.hpp"
#include "hash.hpp"
#include "equality.hpp"
#include "array.hpp"
#include "global.hpp"
#include "error.hpp"
#include "util.hpp"
#include "var.hpp"
#include "eval.hpp"
#include "array_set.hpp"

namespace cleo
{

Value define_multimethod(Value name, Value dispatchFn, Value defaultDispatchVal)
{
    Root multi{create_static_object(*type::Multimethod, dispatchFn, nil, nil, nil, defaultDispatchVal, name)};
    return define(name, *multi);
}

void define_method(Value name, Value dispatchVal, Value fn)
{
    auto var = get_var(name);
    auto m = get_var_root_value(var);
    auto dispatch_fn = get_static_object_element(m, 0);
    auto default_dispatch_val = get_static_object_element(m, 4);
    Root fns{get_static_object_element(m, 3)};
    fns = map_assoc(*fns ? *fns : *EMPTY_MAP, dispatchVal, fn);
    Root new_m{create_static_object(*type::Multimethod, dispatch_fn, nil, nil, *fns, default_dispatch_val, name)};
    set_var_root_value(var, *new_m);
}

void create_global_hierarchy()
{
    Root h{create_static_object(*type::Hierarchy, *EMPTY_MAP)};
    set_var_root_value(rt::global_hierarchy.get_var(), *h);
}

void derive(Value tag, Value parent)
{
    auto h = *rt::global_hierarchy;
    Root ancestors{get_static_object_element(h, 0)};
    auto parent_ancestors = map_get(*ancestors, parent);
    auto parent_ancestors_size = parent_ancestors ? get_array_set_size(parent_ancestors) : 0;
    for (Root s{map_seq(*ancestors)}; *s; s = map_seq_next(*s))
    {
        auto entry = map_seq_first(*s);
        Root entry_ancestors{get_array_elem(entry, 1)};
        if (*entry_ancestors && array_set_contains(*entry_ancestors, tag))
        {
            entry_ancestors = array_set_conj(*entry_ancestors, parent);
            for (decltype(parent_ancestors_size) i = 0; i != parent_ancestors_size; ++i)
                entry_ancestors = array_set_conj(*entry_ancestors, get_array_elem(parent_ancestors, i));
            ancestors = map_assoc(*ancestors, get_array_elem(entry, 0), *entry_ancestors);
        }
    }
    Root tag_ancestors{map_get(*ancestors, tag)};
    if (tag_ancestors->is_nil())
        tag_ancestors = *EMPTY_SET;
    tag_ancestors = array_set_conj(*tag_ancestors, parent);
    for (decltype(parent_ancestors_size) i = 0; i != parent_ancestors_size; ++i)
        tag_ancestors = array_set_conj(*tag_ancestors, get_array_elem(parent_ancestors, i));
        ancestors = map_assoc(*ancestors, tag, *tag_ancestors);
    Root new_h{create_static_object(*type::Hierarchy, *ancestors)};
    set_var_root_value(rt::global_hierarchy.get_var(), *new_h);
}

bool isa_vectors(Value child, Value parent)
{
    auto size = get_array_size(child);
    if (size != get_array_size(parent))
        return false;
    for (decltype(size) i = 0; i < size; ++i)
        if (!isa(get_array_elem(child, i), get_array_elem(parent, i)))
            return false;
    return true;
}

bool is_ancestor(Value child, Value ancestor)
{
    auto ancestors = map_get(get_static_object_element(*rt::global_hierarchy, 0), child);
    return ancestors && array_set_contains(ancestors, ancestor);
}

Value isa(Value child, Value parent)
{
    return (
        child == parent ||
        (get_value_type(child).is(*type::Array) && get_value_type(parent).is(*type::Array) && isa_vectors(child, parent)) ||
        is_ancestor(child, parent)) ? TRUE : nil;
}

void validate_no_ambiguity(Value multimethod, Value dispatchVal, Value selected)
{
    Value fns = get_static_object_element(multimethod, 3);
    for (Root s{map_seq(fns)}; *s; s = map_seq_next(*s))
    {
        auto type = get_array_elem(map_seq_first(*s), 0);
        if (isa(dispatchVal, type) && !isa(selected, type))
        {
            Root msg{create_string("ambiguous multimethod call")};
            throw_exception(new_illegal_argument(*msg));
        }
    }
}

Value get_method(Value multimethod, Value dispatchVal)
{
    if (!get_static_object_element(multimethod, 1).is(*rt::global_hierarchy))
    {
        set_static_object_element(multimethod, 1, *rt::global_hierarchy);
        set_static_object_element(multimethod, 2, nil);
    }
    auto memoized_fns = get_static_object_element(multimethod, 2);
    auto memoized = map_get(memoized_fns, dispatchVal);
    if (memoized)
        return memoized;
    Value best_val = *SENTINEL;
    Value best_fn = nil;
    Value fns = get_static_object_element(multimethod, 3);
    for (Root s{map_seq(fns)}; *s; s = map_seq_next(*s))
    {
        auto tf = map_seq_first(*s);
        auto val = get_array_elem(tf, 0);
        if (isa(dispatchVal, val) && (best_val.is(*SENTINEL) || isa(val, best_val)))
        {
            best_val = val;
            best_fn = get_array_elem(tf, 1);
        }
    }

    validate_no_ambiguity(multimethod, dispatchVal, best_val);

    if (best_val.is(*SENTINEL))
    {
        auto default_ = map_get(fns, get_static_object_element(multimethod, 4));
        if (default_)
            best_fn = default_;
    }
    Root rmemoized_fns{map_assoc(memoized_fns, dispatchVal, best_fn)};
    set_static_object_element(multimethod, 2, *rmemoized_fns);
    return best_fn;
}

Value get_multimethod_name(Value multi)
{
    return get_static_object_element(multi, 5);
}

std::array<unsigned, 256> call_histogram{{}};

Force call_multimethod(Value multi, const Value *args, std::uint8_t numArgs)
{
    check_type("multimethod", multi, *type::Multimethod);
    std::vector<Value> vbuf;
    std::array<Value, 4> abuf;
    Value *fcall = (numArgs < abuf.size()) ?
        abuf.data() :
        (vbuf.resize(numArgs + 1), vbuf.data());
    fcall[0] = get_static_object_element(multi, 0);
    std::copy(args, args + numArgs, fcall + 1);
    Root dispatchVal{call(fcall, numArgs + 1)};
    auto fn = get_method(multi, *dispatchVal);
    if (!fn)
    {
        auto name = get_multimethod_name(multi);
        auto mns = get_symbol_namespace(name);
        auto mname = get_symbol_name(name);
        std::string sname;
        if (mns)
        {
            sname.assign(get_string_ptr(mns), get_string_size(mns));
            sname += '/';
        }
        sname.append(get_string_ptr(mname), get_string_size(mname));
        Root msg{create_string("multimethod not matched: " + sname)};
        throw_exception(new_illegal_argument(*msg));
    }
    fcall[0] = fn;
    return call(fcall, numArgs + 1);
}

Force call_multimethod1(Value multi, Value arg)
{
    return call_multimethod(multi, &arg, 1);
}

Force call_multimethod2(Value multi, Value arg0, Value arg1)
{
    std::array<Value, 2> args{{arg0, arg1}};
    return call_multimethod(multi, args.data(), args.size());
}

Force call_multimethod3(Value multi, Value arg0, Value arg1, Value arg2)
{
    std::array<Value, 3> args{{arg0, arg1, arg2}};
    return call_multimethod(multi, args.data(), args.size());
}

}
