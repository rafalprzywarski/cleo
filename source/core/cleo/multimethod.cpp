#include "multimethod.hpp"
#include "singleton.hpp"
#include "var.hpp"
#include "hash.hpp"
#include "equality.hpp"
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

void define_multimethod(Value name, Value dispatchFn)
{
    define(name, create_object(type::MULTIMETHOD, &name, 1));
    (*multimethods)[name].dispatchFn = dispatchFn;
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

Value isa(Value child, Value parent)
{
    if (are_equal(child, parent))
        return TRUE;
    auto ancestors = global_hierarchy->ancestors.find(child);
    if (ancestors == end(global_hierarchy->ancestors))
        return nil;
    return ancestors->second.count(parent) ? TRUE : nil;
}

Value call_multimethod(Value multi, const Value *args, std::uint8_t numArgs)
{
    auto name = get_object_element(multi, 0);
    auto& multimethod = multimethods->find(name)->second;
    auto dispatchVal = get_native_function_ptr(multimethod.dispatchFn)(args, numArgs);
    auto fn = multimethod.fns.find(dispatchVal);
    if (fn == end(multimethod.fns))
        throw illegal_argument();
    return get_native_function_ptr(fn->second)(args, numArgs);
}

}
