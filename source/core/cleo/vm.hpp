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
constexpr Byte LDDV = 3;
constexpr Byte LDV = 16;
constexpr Byte LDDF = 15;
constexpr Byte STL = 4;
constexpr Byte STVV = 13;
constexpr Byte STVM = 17;
constexpr Byte POP = 5;
constexpr Byte BNIL = 6;
constexpr Byte BNNIL = 7;
constexpr Byte BR = 8;
constexpr Byte CALL = 9;
constexpr Byte APPLY = 10;
constexpr Byte THROW = 11;
constexpr Byte CNIL = 12;
constexpr Byte IFN = 14;

void eval_bytecode(Value constants, Value vars, std::uint32_t locals_size, Value exception_table, const Byte *bytecode, std::uint32_t size);

}
}
