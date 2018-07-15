#include "compile.hpp"
#include "bytecode_fn.hpp"
#include "global.hpp"
#include "array.hpp"
#include "list.hpp"
#include "util.hpp"

namespace cleo
{

Force compile_fn(Value form, Value env)
{
    if (!get_value_type(form).is(*type::List))
        throw_compilation_error("form must be a list");
    if (get_list_first(form) != FN)
        throw_compilation_error("form must start with fn*");
    form = get_list_next(form);
    Value name;
    if (form && get_value_tag(get_list_first(form)) == tag::SYMBOL)
    {
        name = get_list_first(form);
        form = get_list_next(form);
    }
    if (form.is_nil())
        return create_bytecode_fn(name, nullptr, nullptr, 0);
    if (get_value_type(get_list_first(form)).is(*type::List))
        form = get_list_first(form);
    std::array<vm::Byte, 3> code{{vm::LDC, 0, 0}};
    Value val = get_list_first(get_list_next(form));
    Root consts{create_array(&val, 1)};
    std::vector<Int64> arities{0};
    Root rbody{create_bytecode_fn_body(*consts, nil, 0, code.data(), code.size())};
    std::vector<Value> bodies{*rbody};
    return create_bytecode_fn(name, arities.data(), bodies.data(), bodies.size());
}

}
