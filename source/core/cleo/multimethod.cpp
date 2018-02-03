#include "multimethod.hpp"
#include "namespace.hpp"
#include "hash.hpp"
#include "equality.hpp"
#include "small_vector.hpp"
#include "global.hpp"
#include "error.hpp"
#include "util.hpp"
#include "var.hpp"

namespace cleo
{

Value define_multimethod(Value name, Value dispatchFn, Value defaultDispatchVal)
{
    Root multi{create_object(*type::Multimethod, &name, 1)};
    define(name, *multi);
    auto& desc = multimethods[name];
    desc.dispatchFn = dispatchFn;
    desc.defaultDispatchVal = defaultDispatchVal;
    return *multi;
}

void define_method(Value name, Value dispatchVal, Value fn)
{
    multimethods[name].fns[dispatchVal] = fn;
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
    auto size = get_small_vector_size(child);
    if (size != get_small_vector_size(parent))
        return false;
    for (decltype(size) i = 0; i < size; ++i)
        if (!isa(get_small_vector_elem(child, i), get_small_vector_elem(parent, i)))
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
        are_equal(child, parent) ||
        (get_value_type(child).is(*type::SmallVector) && get_value_type(parent).is(*type::SmallVector) && isa_vectors(child, parent)) ||
        is_ancestor(child, parent)) ? TRUE : nil;
}

Value get_method(const Multimethod& multimethod, Value dispatchVal)
{
    std::pair<Value, Value> best{nil, nil};
    for (auto& fn : multimethod.fns)
        if (isa(dispatchVal, fn.first))
        {
            if (!best.first || isa(fn.first, best.first))
                best = fn;
            else if (!isa(best.first, fn.first))
            {
                Root msg{create_string("ambiguous multimethod call")};
                throw_exception(new_illegal_argument(*msg));
            }
        }

    if (!best.first)
    {
        auto default_ = multimethod.fns.find(multimethod.defaultDispatchVal);
        if (default_ != end(multimethod.fns))
            return default_->second;
    }
    return best.second;
}

Value get_method(Value multi, Value dispatchVal)
{
    auto name = get_object_element(multi, 0);
    auto& multimethod = multimethods.find(name)->second;
    return get_method(multimethod, dispatchVal);
}

Force call_multimethod(Value multi, const Value *args, std::uint8_t numArgs)
{
    check_type("multimethod", multi, *type::Multimethod);
    auto name = get_object_element(multi, 0);
    auto& multimethod = multimethods.find(name)->second;
    Root dispatchVal{get_native_function_ptr(multimethod.dispatchFn)(args, numArgs)};
    auto fn = get_method(multimethod, *dispatchVal);
    if (!fn)
    {
        auto mns = get_symbol_namespace(name);
        auto mname = get_symbol_name(name);
        std::string sname;
        if (mns)
        {
            sname.assign(get_string_ptr(mns), get_string_len(mns));
            sname += '/';
        }
        sname.append(get_string_ptr(mname), get_string_len(mname));
        Root msg{create_string("multimethod not matched: " + sname)};
        throw_exception(new_illegal_argument(*msg));
    }
    return get_native_function_ptr(fn)(args, numArgs);
}

Force call_multimethod1(Value multi, Value arg)
{
    return call_multimethod(multi, &arg, 1);
}

}
