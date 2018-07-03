#include "vm.hpp"
#include "array.hpp"

namespace cleo
{
namespace vm
{

namespace
{

std::uint16_t read_u16(const Byte *p)
{
    return std::uint8_t(p[0]) | std::uint16_t(std::uint8_t(p[1])) << 8;
}

}

void eval_bytecode(Stack& stack, Value constants, Value vars, std::uint32_t locals_size, const Byte *bytecode, std::uint32_t size)
{
    auto p = bytecode;
    auto endp = p + size;
    while (p != endp)
    {
        switch (*p)
        {
        case LDC:
            stack.push_back(get_array_elem(constants, read_u16(p + 1)));
            p += 3;
            break;
        case LDV:
            stack.push_back(get_var_value(get_array_elem(vars, read_u16(p + 1))));
            p += 3;
            break;
        case POP:
            stack.pop_back();
            ++p;
            break;
        }
    }
}

}
}
