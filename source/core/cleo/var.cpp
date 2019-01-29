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
        set_var_root_value(found->second, val);
        set_var_meta(found->second, meta);
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

Value get_var(Value sym)
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

    Root merged{map_merge(get_list_first(current_bindings), bindings)};
    cleo::bindings = list_conj(current_bindings, *merged);
}

void pop_bindings()
{
    assert(*bindings);
    assert(get_list_size(*bindings) != 0);

    bindings = get_list_next(*bindings);
}

void set_var_root_value(Value var, Value val)
{
    set_object_element(var, 1, val);
}

void set_var_meta(Value var, Value meta)
{
    auto vmeta = get_var_meta(var);
    if (!vmeta)
        vmeta = *EMPTY_MAP;
    Root nmeta{meta ? meta : *EMPTY_MAP};
    if (!map_get(*nmeta, NAME_KEY))
        nmeta = map_assoc(*nmeta, NAME_KEY, map_get(vmeta, NAME_KEY));
    if (!map_get(*nmeta, NS_KEY))
        nmeta = map_assoc(*nmeta, NS_KEY, map_get(vmeta, NS_KEY));
    set_object_element(var, 2, *nmeta);
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
    auto meta = get_var_meta(var);
    return meta ? map_get(meta, MACRO_KEY) : nil;
}

Value is_var_dynamic(Value var)
{
    auto meta = get_var_meta(var);
    return meta ? map_get(meta, DYNAMIC_KEY) : nil;
}

Value is_var_public(Value var)
{
    return map_get(get_var_meta(var), PRIVATE_KEY) ? nil : TRUE;
}

Value get_var_meta(Value var)
{
    return get_object_element(var, 2);
}

}
