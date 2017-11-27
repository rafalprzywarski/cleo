#include "multimethod.hpp"
#include "singleton.hpp"
#include "var.hpp"
#include "hash.hpp"
#include "equality.hpp"
#include <unordered_map>

namespace cleo
{
namespace type
{
extern const Value MULTIMETHOD = create_symbol("cleo.core", "Multimethod");
}

class Multimethods {};

struct Multimethod
{
    Value dispatchFn;
    std::unordered_map<Value, Value, std_hash, std_equal_to> fns;
};

singleton<std::unordered_map<Value, Multimethod>, Multimethods> multimethods;

void define_multimethod(Value name, Value dispatchFn)
{
    define(name, create_object(type::MULTIMETHOD, &name, 1));
    (*multimethods)[name].dispatchFn = dispatchFn;
}

void define_method(Value name, Value dispatchVal, Value fn)
{
    (*multimethods)[name].fns[dispatchVal] = fn;
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
