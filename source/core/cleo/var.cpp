#include "var.hpp"
#include "global.hpp"
#include "error.hpp"

namespace cleo
{

void define(Value sym, Value val)
{
    vars[sym] = val;
}

Value lookup(Value sym)
{
    auto it = vars.find(sym);
    if (it == end(vars))
        throw SymbolNotFound("symbol not found");
    return it->second;
}

}
