#include "vm.hpp"
#include "array.hpp"
#include "var.hpp"

namespace cleo
{
namespace vm
{

static_assert(std::is_same<std::int8_t, signed char>::value, "bytes must be 8-bit, 2's complement");

namespace
{

std::uint16_t read_u16(const Byte *p)
{
    return std::uint8_t(p[0]) | std::uint16_t(std::uint8_t(p[1])) << 8;
}

std::int16_t read_i16(const Byte *p)
{
    return std::int16_t(read_u16(p));
}

const Byte *br(const Byte *p)
{
    return p + (3 + read_i16(p + 1));
}

Value pop(Stack& stack)
{
    auto val = stack.back();
    stack.pop_back();
    return val;
}

}

void eval_bytecode(Stack& stack, Value constants, Value vars, std::uint32_t locals_size, const Byte *bytecode, std::uint32_t size)
{
    auto p = bytecode;
    auto endp = p + size;
    auto stack_base = stack.size() - locals_size;
    while (p != endp)
    {
        switch (*p)
        {
        case LDC:
            stack.push_back(get_array_elem(constants, read_u16(p + 1)));
            p += 3;
            break;
        case LDL:
            stack.push_back(Value{stack[stack_base + read_i16(p + 1)]});
            p += 3;
            break;
        case LDV:
            stack.push_back(get_var_value(get_array_elem(vars, read_u16(p + 1))));
            p += 3;
            break;
        case STL:
        {
            auto val = pop(stack);
            stack[stack_base + read_i16(p + 1)] = val;
            p += 3;
            break;
        }
        case POP:
            stack.pop_back();
            ++p;
            break;
        case BNIL:
        {
            auto c = pop(stack);
            p = c ? p + 3 : br(p);
            break;
        }
        case BNNIL:
        {
            auto c = pop(stack);
            p = c ? br(p) : p + 3;
            break;
        }
        case BR:
            p = br(p);
            break;
        }
    }
}

}
}
