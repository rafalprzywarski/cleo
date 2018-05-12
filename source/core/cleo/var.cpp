#include "var.hpp"
#include "global.hpp"
#include "error.hpp"
#include "print.hpp"
#include "list.hpp"
#include "util.hpp"

namespace cleo
{

Value define_var(Value sym, Value val, Value meta)
{
    auto found = vars.find(sym);
    if (found != end(vars))
    {
        set_object_element(found->second, 1, val);
        set_object_element(found->second, 2, meta);
        return found->second;
    }
    Root var{create_object3(*type::Var, sym, val, meta)};
    vars.insert({sym, *var});
    return *var;
}

void undefine_var(Value sym)
{
    vars[sym] = nil;
}

Value lookup_var(Value sym)
{
    auto var = lookup_var_or_nil(sym);
    if (!var)
    {
        Root ss{pr_str(sym)};
        Root msg{create_string("unable to resolve symbol " + std::string(get_string_ptr(*ss), get_string_len(*ss)))};
        throw_exception(new_symbol_not_found(*msg));
    }
    return var;
}

Value lookup_var_or_nil(Value sym)
{
    auto it = vars.find(sym);
    return (it == end(vars)) ? nil : it->second;
}

void push_bindings(Value bindings)
{
    auto current_bindings = *cleo::bindings;
    if (!current_bindings)
    {
        cleo::bindings = list_conj(*EMPTY_LIST, bindings);
        return;
    }

    Root merged{map_merge(get_list_first(current_bindings), bindings)};
    cleo::bindings = list_conj(current_bindings, *merged);
}

void pop_bindings()
{
    assert(*bindings);
    assert(get_int64_value(get_list_size(*bindings)) != 0);

    bindings = get_list_next(*bindings);
}

void set_var(Value sym, Value val)
{
    if (!*bindings || !map_contains(get_list_first(*bindings), sym))
    {
        Root ss{pr_str(sym)};
        throw_illegal_state("Can't change/establish root binding of: " + std::string(get_string_ptr(*ss), get_string_len(*ss)));
    }
    Root latest{get_list_first(*bindings)};
    latest = map_assoc(*latest, sym, val);
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
        if (map_contains(latest, sym))
            return map_get(latest, sym);
    }
    return get_var_root_value(var);
}

Value is_var_macro(Value var)
{
    auto meta = get_object_element(var, 2);
    return meta ? map_get(meta, MACRO_KEY) : nil;
}

}
