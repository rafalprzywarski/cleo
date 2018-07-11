#pragma once
#include "value.hpp"
#include "vm.hpp"

namespace cleo
{

Force create_bytecode_fn_body(Value consts, Value vars, Int64 locals_size, const vm::Byte *bytes, Int64 bytes_size);
Value get_bytecode_fn_body_consts(Value body);
Value get_bytecode_fn_body_vars(Value body);
Int64 get_bytecode_fn_body_locals_size(Value body);
const vm::Byte *get_bytecode_fn_body_bytes(Value body);
Int64 get_bytecode_fn_body_bytes_size(Value body);

Force create_bytecode_fn(Value name, const Int64 *arities, const Value *bodies, std::uint8_t n);
Value get_bytecode_fn_name(Value fn);
std::uint8_t get_bytecode_fn_size(Value fn);
Int64 get_bytecode_fn_arity(Value fn, std::uint8_t i);
Value get_bytecode_fn_body(Value fn, std::uint8_t i);

}
