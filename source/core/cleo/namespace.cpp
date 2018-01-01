#include "namespace.hpp"
#include "global.hpp"
#include "var.hpp"
#include "error.hpp"

namespace cleo
{

Value in_ns(Value ns)
{
    if (ns != nil && get_value_tag(ns) != tag::SYMBOL)
    {
        Root msg{create_string("ns must be a symbol")};
        throw_exception(new_illegal_argument(*msg));
    }
    define(CURRENT_NS, ns);
    return nil;
}

}
