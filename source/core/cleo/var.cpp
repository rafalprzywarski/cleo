#include "var.hpp"
#include "global.hpp"
#include "error.hpp"
#include "print.hpp"
#include "list.hpp"
#include "small_map.hpp"

namespace cleo
{

void define_var(Value sym, Value val)
{
    vars[sym] = val;
}

Value lookup_var(Value sym)
{
    if (*bindings != nil)
    {
        auto latest = get_list_first(*bindings);
        if (small_map_contains(latest, sym) != nil)
            return small_map_get(latest, sym);
    }
    auto it = vars.find(sym);
    if (it == end(vars))
    {
        Root ss{pr_str(sym)};
        Root msg{create_string("unable to resolve symbol " + std::string(get_string_ptr(*ss), get_string_len(*ss)))};
        throw_exception(new_symbol_not_found(*msg));
    }
    return it->second;
}

void push_bindings(Value bindings)
{
    auto current_bindings = *cleo::bindings;
    if (current_bindings == nil)
    {
        cleo::bindings = list_conj(*EMPTY_LIST, bindings);
        return;
    }

    Root merged{small_map_merge(get_list_first(current_bindings), bindings)};
    cleo::bindings = list_conj(current_bindings, bindings);
}

void pop_bindings()
{
    assert(*bindings != nil);
    assert(get_int64_value(get_list_size(*bindings)) != 0);

    bindings = get_list_next(*bindings);
}

void set_var(Value sym, Value val)
{
    if (*bindings == nil || small_map_contains(get_list_first(*bindings), sym) == nil)
    {
        Root ss{pr_str(sym)};
        Root msg{create_string("Can't change/establish root binding of: " + std::string(get_string_ptr(*ss), get_string_len(*ss)))};
        throw_exception(new_illegal_state(*msg));
    }
    Root latest{get_list_first(*bindings)};
    latest = small_map_assoc(*latest, sym, val);
    auto popped = get_list_next(*bindings);
    bindings = list_conj(popped == nil ? *EMPTY_LIST : popped, *latest);
}


}
