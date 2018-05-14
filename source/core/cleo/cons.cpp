#include "cons.hpp"
#include "global.cpp"

namespace cleo
{

Force create_cons(Value elem, Value next)
{
    return create_object2(*type::Cons, elem, next);
}

Value cons_first(Value c)
{
    return get_object_element(c, 0);
}

Value cons_next(Value c)
{
    return get_object_element(c, 1);
}

Force cons_conj(Value c, Value elem)
{
    return create_cons(elem, c);
}

}
