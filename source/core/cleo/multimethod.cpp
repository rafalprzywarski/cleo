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

namespace cleo
{

Value define_multimethod(Value name, Value dispatchFn, Value defaultDispatchVal)
{
    Root multi{create_static_object(*type::Multimethod, name, dispatchFn, defaultDispatchVal, nil, nil)};
    return define(name, *multi);
}

void define_method(Value name, Value dispatchVal, Value fn)
{
    auto var = get_var(name);
    auto m = get_var_root_value(var);
    auto dispatch_fn = get_static_object_element(m, 1);
    auto default_dispatch_val = get_static_object_element(m, 2);
    Root fns{get_static_object_element(m, 3)};
    fns = map_assoc(*fns ? *fns : *EMPTY_MAP, dispatchVal, fn);
    Root new_m{create_static_object(*type::Multimethod, name, dispatch_fn, default_dispatch_val, *fns, nil)};
    set_var_root_value(var, *new_m);
}

void derive(Value tag, Value parent)
{
    auto& h = global_hierarchy;
    auto& parent_ancestors = h.ancestors[parent];
    for (auto& a : h.ancestors)
        if (a.second.count(tag))
        {
            a.second.insert(parent);
            a.second.insert(begin(parent_ancestors), end(parent_ancestors));
        }
    auto& ancestors = h.ancestors[tag];
    ancestors.insert(parent);
    ancestors.insert(begin(parent_ancestors), end(parent_ancestors));
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
    auto ancestors = global_hierarchy.ancestors.find(child);
    if (ancestors == end(global_hierarchy.ancestors))
        return false;
    return ancestors->second.count(ancestor) != 0;
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
    auto memoized_fns = get_static_object_element(multimethod, 4);
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
        auto default_ = map_get(fns, get_static_object_element(multimethod, 2));
        if (default_)
            best_fn = default_;
    }
    Root rmemoized_fns{map_assoc(memoized_fns, dispatchVal, best_fn)};
    set_static_object_element(multimethod, 4, *rmemoized_fns);
    return best_fn;
}

Value get_multimethod_name(Value multi)
{
    return get_static_object_element(multi, 0);
}

std::array<unsigned, 256> call_histogram{{}};

Force call_multimethod(Value multi, const Value *args, std::uint8_t numArgs)
{
    check_type("multimethod", multi, *type::Multimethod);
    auto name = get_static_object_element(multi, 0);
    std::vector<Value> vbuf;
    std::array<Value, 4> abuf;
    Value *fcall = (numArgs < abuf.size()) ?
        abuf.data() :
        (vbuf.resize(numArgs + 1), vbuf.data());
    fcall[0] = get_static_object_element(multi, 1);
    std::copy(args, args + numArgs, fcall + 1);
    Root dispatchVal{call(fcall, numArgs + 1)};
    auto fn = get_method(multi, *dispatchVal);
    if (!fn)
    {
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
