#pragma once
#include "value.hpp"

namespace cleo
{

Force create_persistent_hash_set();
Int64 get_persistent_hash_set_size(Value m);
Value persistent_hash_set_get(Value m, Value k);
Value persistent_hash_set_get(Value m, Value k, Value def_v);
Force persistent_hash_set_conj(Value map, Value key);
Force persistent_hash_set_disj(Value map, Value key);
Value persistent_hash_set_contains(Value m, Value k);
Value are_persistent_hash_sets_equal(Value left, Value right);
Force persistent_hash_set_seq(Value m);
Value get_persistent_hash_set_seq_first(Value s);
Force get_persistent_hash_set_seq_next(Value s);

}
