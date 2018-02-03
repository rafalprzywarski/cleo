#include "var.hpp"
#include "global.hpp"
#include "error.hpp"
#include "print.hpp"
#include "list.hpp"
#include "small_map.hpp"

namespace cleo
{

Value define_var(Value sym, Value val)
{
    auto found = vars.find(sym);
    if (found != end(vars))
    {
        set_object_element(found->second, 1, val);
        return found->second;
    }
    Root var{create_object2(*type::Var, sym, val)};
    vars.insert({sym, *var});
    return *var;
}

void undefine_var(Value sym)
{
    vars[sym] = nil;
}

Value lookup_var(Value sym)
{
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
    if (!current_bindings)
    {
        cleo::bindings = list_conj(*EMPTY_LIST, bindings);
        return;
    }

    Root merged{small_map_merge(get_list_first(current_bindings), bindings)};
    cleo::bindings = list_conj(current_bindings, bindings);
}

void pop_bindings()
{
    assert(*bindings);
    assert(get_int64_value(get_list_size(*bindings)) != 0);

    bindings = get_list_next(*bindings);
}

void set_var(Value sym, Value val)
{
    if (!*bindings || !small_map_contains(get_list_first(*bindings), sym))
    {
        Root ss{pr_str(sym)};
        Root msg{create_string("Can't change/establish root binding of: " + std::string(get_string_ptr(*ss), get_string_len(*ss)))};
        throw_exception(new_illegal_state(*msg));
    }
    Root latest{get_list_first(*bindings)};
    latest = small_map_assoc(*latest, sym, val);
    auto popped = get_list_next(*bindings);
    bindings = list_conj(!popped ? *EMPTY_LIST : popped, *latest);
}

Value get_var_name(Value var)
{
    return get_object_element(var, 0);
}

Value get_var_root_value(Value var)
{
    return get_object_element(var, 1);
}

Value get_var_value(Value var)
{
    if (*bindings)
    {
        auto sym = get_var_name(var);
        auto latest = get_list_first(*bindings);
        if (small_map_contains(latest, sym))
            return small_map_get(latest, sym);
    }
    return get_var_root_value(var);
}

}
