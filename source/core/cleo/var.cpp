#include "var.hpp"
#include "global.hpp"
#include "error.hpp"
#include "print.hpp"

namespace cleo
{

void define_var(Value sym, Value val)
{
    vars[sym] = val;
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

}
