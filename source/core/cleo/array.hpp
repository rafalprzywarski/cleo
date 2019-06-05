#pragma once
#include "value.hpp"

namespace cleo
{

Force create_array(const Value *elems, std::uint32_t size);
inline std::uint32_t get_array_size(Value v) { return get_object_size(v); }
inline Value get_array_elem(Value v, std::uint32_t index) { return index < get_object_size(v) ? get_object_element(v, index) : nil; }
inline Value get_array_elem_unchecked(Value v, std::uint32_t index) { return get_object_element(v, index); }
Force array_seq(Value v);
Value get_array_seq_first(Value s);
Force get_array_seq_next(Value s);
Force array_conj(Value v, Value e);
Force array_hash(Value v);

Force transient_array(Value other);
inline Int64 get_transient_array_size(Value v) { return get_object_int(v, 0); }
Value get_transient_array_elem(Value v, std::uint32_t index);
Force transient_array_conj(Value v, Value e);
Force transient_array_assoc_elem(Value v, std::uint32_t index, Value e);
Force transient_array_persistent(Value v);

}
