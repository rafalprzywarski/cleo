#include "vm.hpp"
#include "array.hpp"
#include "var.hpp"
#include "global.hpp"
#include "eval.hpp"

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
            stack_push(get_array_elem(constants, read_u16(p + 1)));
            p += 3;
            break;
        case LDL:
            stack_push(Value{stack[stack_base + read_i16(p + 1)]});
            p += 3;
            break;
        case LDV:
            stack_push(get_var_value(get_array_elem(vars, read_u16(p + 1))));
            p += 3;
            break;
        case STL:
        {
            stack[stack_base + read_i16(p + 1)] = stack.back();
            stack_pop();
            p += 3;
            break;
        }
        case SETV:
        {
            auto var = stack[stack.size() - 3];
            set_var_root_value(var, stack[stack.size() - 2]);
            set_var_meta(var, stack.back());
            stack_pop();
            stack_pop();
            ++p;
            break;
        }
        case POP:
            stack_pop();
            ++p;
            break;
        case BNIL:
        {
            p = stack.back() ? p + 3 : br(p);
            stack_pop();
            break;
        }
        case BNNIL:
        {
            p = stack.back() ? br(p) : p + 3;
            stack_pop();
            break;
        }
        case BR:
            p = br(p);
            break;
        case CALL:
        {
            auto n = std::uint8_t(p[1]) + 1;
            auto& first = stack[stack.size() - n];
            first = call(&first, n).value();
            stack.resize(stack.size() - (n - 1));
            p += 2;
            break;
        }
        case CNIL:
            stack_push(nil);
            ++p;
            break;
        }
    }
}

}
}
