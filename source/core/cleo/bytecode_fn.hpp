#pragma once
#include "value.hpp"
#include "vm.hpp"

namespace cleo
{

struct bytecode_fn_exception_handler
{
    Int64 offset{};
    Int64 stack_size{};

    bytecode_fn_exception_handler() = default;
    bytecode_fn_exception_handler(Int64 offset, Int64 stack_size) : offset(offset), stack_size(stack_size) {}
};

Force create_bytecode_fn_exception_table(const Int64 *entries, const Value *types, std::uint32_t size);
std::uint32_t get_bytecode_fn_exception_table_size(Value et);
Int64 get_bytecode_fn_exception_table_start_offset(Value et, std::uint32_t i);
Int64 get_bytecode_fn_exception_table_end_offset(Value et, std::uint32_t i);
Int64 get_bytecode_fn_exception_table_handler_offset(Value et, std::uint32_t i);
Int64 get_bytecode_fn_exception_table_stack_size(Value et, std::uint32_t i);
Value get_bytecode_fn_exception_table_type(Value et, std::uint32_t i);
bytecode_fn_exception_handler bytecode_fn_find_exception_handler(Value et, Int64 offset, Value type);

Force create_bytecode_fn_body(Value consts, Value vars, Value exception_table, Int64 locals_size, const vm::Byte *bytes, Int64 bytes_size);
Value get_bytecode_fn_body_consts(Value body);
Value get_bytecode_fn_body_vars(Value body);
Value get_bytecode_fn_body_exception_table(Value body);
Int64 get_bytecode_fn_body_locals_size(Value body);
const vm::Byte *get_bytecode_fn_body_bytes(Value body);
Int64 get_bytecode_fn_body_bytes_size(Value body);

Force create_bytecode_fn(Value name, const Int64 *arities, const Value *bodies, std::uint8_t n);
Value get_bytecode_fn_name(Value fn);
std::uint8_t get_bytecode_fn_size(Value fn);
Int64 get_bytecode_fn_arity(Value fn, std::uint8_t i);
Value get_bytecode_fn_body(Value fn, std::uint8_t i);
Force bytecode_fn_replace_consts(Value fn, const Value *consts, Int64 n);
std::pair<Value, Int64> bytecode_fn_find_body(Value fn, std::uint8_t arity);

}
