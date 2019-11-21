#include "global.hpp"
#include "cons.hpp"
#include "eval.hpp"
#include "multimethod.hpp"

namespace cleo
{

Force create_lazy_seq(Value fn)
{
    return create_object2(*type::LazySeq, fn, nil);
}

Value lazy_seq_seq(Value ls)
{
    auto fn = get_static_object_element(ls, 0);
    if (!fn)
        return get_static_object_element(ls, 1);
    Root s{call(&fn, 1)};
    s = call_multimethod1(*rt::seq, *s);
    set_object_element(ls, 1, *s);
    set_object_element(ls, 0, nil);
    return *s;
}

Force lazy_seq_first(Value ls)
{
    auto s = lazy_seq_seq(ls);
    return call_multimethod1(*rt::first, s);
}

Force lazy_seq_next(Value ls)
{
    auto s = lazy_seq_seq(ls);
    return call_multimethod1(*rt::next, s);
}

Force lazy_seq_conj(Value ls, Value elem)
{
    auto s = lazy_seq_seq(ls);
    return cons_conj(s, elem);
}

}
