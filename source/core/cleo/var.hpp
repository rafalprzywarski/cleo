#pragma once
#include "value.hpp"

namespace cleo
{

Value define_var(Value sym, Value val, Value meta);
inline Value define_var(Value sym, Value val) { return define_var(sym, val, nil); }
void undefine_var(Value sym);
Value get_var(Value sym);
void push_bindings(Value bindings);
void pop_bindings();
void set_var(Value sym, Value val);
Value get_var_name(Value var);
Value get_var_root_value(Value var);
Value get_var_value(Value var);
Value is_var_macro(Value var);

struct PushBindingsGuard
{
    PushBindingsGuard(Value bindings)
    {
        push_bindings(bindings);
    }

    ~PushBindingsGuard()
    {
        pop_bindings();
    }

    PushBindingsGuard(const PushBindingsGuard& ) = delete;
    PushBindingsGuard& operator=(const PushBindingsGuard& ) = delete;
};

Force create_var_value_ref(Value var);
Value get_var_value_ref_value(Value ref);
Value var_value_ref_equals(Value l, Value r);
Force var_value_ref_pr_str(Value ref);

}
