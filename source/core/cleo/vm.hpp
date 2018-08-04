#pragma once
#include "value.hpp"
#include <vector>

namespace cleo
{
namespace vm
{

using Stack = std::vector<Value>;
using Byte = char;

constexpr Byte LDC = 1;
constexpr Byte LDL = 2;
constexpr Byte LDV = 3;
constexpr Byte STL = 4;
constexpr Byte SETV = 13;
constexpr Byte POP = 5;
constexpr Byte BNIL = 6;
constexpr Byte BNNIL = 7;
constexpr Byte BR = 8;
constexpr Byte CALL = 9;
constexpr Byte APPLY = 10;
constexpr Byte THROW = 11;
constexpr Byte CNIL = 12;
constexpr Byte IFN = 14;

void eval_bytecode(Stack& stack, Value constants, Value vars, std::uint32_t locals_size, const Byte *bytecode, std::uint32_t size);

}
}
