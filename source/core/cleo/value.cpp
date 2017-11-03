#include "value.hpp"
#include "memory.hpp"
#include <cstring>

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

struct String
{
    ObjectHeader header;
    std::uint32_t len;
    char firstChar;
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

Value create_string(const char *str, std::uint32_t len)
{
    String *val = static_cast<String *>(mem_alloc(offsetof(String, firstChar) + len));
    val->header.type = type::STRING;
    val->len = len;
    std::memcpy(&val->firstChar, str, len);
    return val;
}

const char *get_string_ptr(Value val)
{
    return &static_cast<String *>(val)->firstChar;
}

std::uint32_t get_string_len(Value val)
{
    return static_cast<String *>(val)->len;
}

}
