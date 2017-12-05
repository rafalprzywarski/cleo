#include "multimethod.hpp"
#include "singleton.hpp"
#include "var.hpp"
#include "hash.hpp"
#include "equality.hpp"
#include "small_vector.hpp"
#include <unordered_map>
#include <unordered_set>

namespace cleo
{
namespace type
{
const Value MULTIMETHOD = create_symbol("cleo.core", "Multimethod");
}

class GlobalHierarchy {};
class Multimethods {};

struct Hierachy
{
    std::unordered_map<Value, std::unordered_set<Value, StdHash, StdEqualTo>, StdHash, StdEqualTo> ancestors;
};

struct Multimethod
{
    Value dispatchFn;
    std::unordered_map<Value, Value, StdHash, StdEqualTo> fns;
};

singleton<std::unordered_map<Value, Multimethod>, Multimethods> multimethods;
singleton<Hierachy, GlobalHierarchy> global_hierarchy;

Value define_multimethod(Value name, Value dispatchFn)
{
    auto multi = create_object(type::MULTIMETHOD, &name, 1);
    define(name, multi);
    (*multimethods)[name].dispatchFn = dispatchFn;
    return multi;
}

void define_method(Value name, Value dispatchVal, Value fn)
{
    (*multimethods)[name].fns[dispatchVal] = fn;
}

void derive(Value tag, Value parent)
{
    auto& h = *global_hierarchy;
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
    auto ancestors = global_hierarchy->ancestors.find(child);
    if (ancestors == end(global_hierarchy->ancestors))
        return false;
    return ancestors->second.count(ancestor) != 0;
}

Value isa(Value child, Value parent)
{
    return (
        are_equal(child, parent) != nil ||
        (get_value_type(child) == type::SMALL_VECTOR && get_value_type(parent) == type::SMALL_VECTOR && isa_vectors(child, parent)) ||
        is_ancestor(child, parent)) ? TRUE : nil;
}

Value get_method(const Multimethod& multimethod, Value dispatchVal)
{
    std::pair<Value, Value> best{nil, nil};
    for (auto& fn : multimethod.fns)
        if (isa(dispatchVal, fn.first))
        {
            if (best.first == nil || isa(fn.first, best.first))
                best = fn;
            else if (!isa(best.first, fn.first))
                throw illegal_argument();
        }
    return best.second;
}

Value get_method(Value multi, Value dispatchVal)
{
    auto name = get_object_element(multi, 0);
    auto& multimethod = multimethods->find(name)->second;
    return get_method(multimethod, dispatchVal);
}

Value call_multimethod(Value multi, const Value *args, std::uint8_t numArgs)
{
    auto name = get_object_element(multi, 0);
    auto& multimethod = multimethods->find(name)->second;
    auto dispatchVal = get_native_function_ptr(multimethod.dispatchFn)(args, numArgs);
    auto fn = get_method(multimethod, dispatchVal);
    if (fn == nil)
        throw illegal_argument();
    return get_native_function_ptr(fn)(args, numArgs);
}

}
