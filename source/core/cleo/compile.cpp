#include "compile.hpp"
#include "bytecode_fn.hpp"
#include "global.hpp"
#include "array.hpp"
#include "list.hpp"

namespace cleo
{

Force compile_fn(Value form, Value env)
{
    std::array<vm::Byte, 3> code{{vm::LDC, 0, 0}};
    Value val = get_list_first(get_list_next(get_list_next(form)));
    Root consts{create_array(&val, 1)};
    Int64 arity = 0;
    Root rbody{create_bytecode_fn_body(*consts, nil, 0, code.data(), code.size())};
    Value body = *rbody;
    return create_bytecode_fn(nil, &arity, &body, 1);
}

}
