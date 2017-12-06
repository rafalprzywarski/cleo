#include "var.hpp"
#include "global.hpp"

namespace cleo
{

void define(Value sym, Value val)
{
    vars[sym] = val;
}

Value lookup(Value sym)
{
    auto it = vars.find(sym);
    return it == end(vars) ? nil : it->second;
}

}
