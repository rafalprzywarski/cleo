#pragma once
#include "value.hpp"

namespace cleo
{

void define_var(Value sym, Value val);
Value lookup_var(Value sym);
void push_bindings(Value bindings);
void pop_bindings();
void set_var(Value sym, Value val);

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

}
