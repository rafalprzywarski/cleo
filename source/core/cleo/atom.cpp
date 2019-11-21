#include "atom.hpp"
#include "global.hpp"

namespace cleo
{

Force create_atom(Value val)
{
    return create_object1(*type::Atom, val);
}

Value atom_deref(Value atom)
{
    return get_static_object_element(atom, 0);
}

Force atom_reset(Value atom, Value val)
{
    Root oldval{atom_deref(atom)};
    set_object_element(atom, 0, val);
    return *oldval;
}

}
