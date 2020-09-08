#pragma once
#include "value.hpp"

namespace cleo
{

Force create_byte_array(const Value *elems, Int64 size);
inline Int64 get_byte_array_size(Value v) { return get_dynamic_object_int(v, 0); }
inline Value get_byte_array_elem_unchecked(Value v, Int64 index) { return create_int48(get_dynamic_object_int_byte(v, index + sizeof(Int64))); }
inline Value get_byte_array_elem(Value v, Int64 index) { return index >= 0 && index < get_byte_array_size(v) ? get_byte_array_elem_unchecked(v, index) : nil; }
Force byte_array_seq(Value v);
Value get_byte_array_seq_first(Value s);
Force get_byte_array_seq_next(Value s);
Force byte_array_conj(Value v, Value e);
Force byte_array_pop(Value v);
Force byte_array_hash(Value v);

Force transient_byte_array(Value other);
inline Int64 get_transient_byte_array_size(Value v) { return get_dynamic_object_int(v, 0); }
Value get_transient_byte_array_elem(Value v, Int64 index);
Force transient_byte_array_conj(Value v, Value e);
Force transient_byte_array_pop(Value v);
Force transient_byte_array_assoc_elem(Value v, Int64 index, Value e);
Force transient_byte_array_persistent(Value v);

}
