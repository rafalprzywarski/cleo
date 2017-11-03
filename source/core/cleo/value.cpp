#include "value.hpp"
#include "memory.hpp"

namespace cleo
{

struct ObjectHeader
{
    std::uint64_t type;
};

struct Integer
{
    ObjectHeader header;
    Int intVal;
};

ObjectHeader nil{type::NIL};

Type get_value_type(Value val)
{
    return static_cast<ObjectHeader *>(val)->type;
}

Value get_nil()
{
    return &nil;
}

Value create_int(std::int64_t intVal)
{
    Integer *val = alloc<Integer>();
    val->header.type = type::INT;
    val->intVal = intVal;
    return val;
}

Int get_int_value(Value val)
{
    return static_cast<Integer *>(val)->intVal;
}

}
