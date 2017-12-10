#include "var.hpp"
#include "global.hpp"
#include "error.hpp"
#include "print.hpp"

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
    {
        Root ss{pr_str(sym)};
        throw SymbolNotFound("unable to resolve symbol " + std::string(get_string_ptr(*ss), get_string_len(*ss)));
    }
    return it->second;
}

}
