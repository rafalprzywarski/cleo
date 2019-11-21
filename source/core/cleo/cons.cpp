#include "cons.hpp"
#include "global.hpp"
#include "multimethod.hpp"

namespace cleo
{

Force create_cons(Value elem, Value next)
{
    return create_object2(*type::Cons, elem, next);
}

Value cons_first(Value c)
{
    return get_static_object_element(c, 0);
}

Force cons_next(Value c)
{
    return call_multimethod1(*rt::seq, get_static_object_element(c, 1));
}

Force cons_conj(Value c, Value elem)
{
    return create_cons(elem, c);
}

}
