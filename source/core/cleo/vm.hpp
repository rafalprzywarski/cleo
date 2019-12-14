#pragma once
#include "value.hpp"
#include <vector>

namespace cleo
{
namespace vm
{

using Stack = std::vector<Value>;
using IntStack = std::vector<Int64>;
using Byte = char;

constexpr Byte CNIL = 0x00;

constexpr Byte POP = 0x01;

constexpr Byte LDC = 0x10;
constexpr Byte LDL = 0x11;
constexpr Byte LDDV = 0x12;
constexpr Byte LDV = 0x13;
constexpr Byte LDDF = 0x14;
constexpr Byte LDSF = 0x15;

constexpr Byte STL = 0x20;
constexpr Byte STVV = 0x21;
constexpr Byte STVM = 0x22;
constexpr Byte STVB = 0x23;
constexpr Byte STDF = 0x24;
constexpr Byte STSF = 0x25;

constexpr Byte BR = 0x30;
constexpr Byte BNIL = 0x31;
constexpr Byte BNNIL = 0x32;

constexpr Byte CALL = 0x40;
constexpr Byte APPLY = 0x41;

constexpr Byte THROW = 0x48;

constexpr Byte IFN = 0x50;

constexpr Byte UBXI64 = 0x80;
constexpr Byte BXI64 =  0x81;
constexpr Byte ADDI64 = 0x82;

void eval_bytecode(Value constants, Value vars, std::uint32_t locals_size, Value exception_table, const Byte *bytecode, std::uint32_t size);

}
}
