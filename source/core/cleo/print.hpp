#pragma once
#include "value.hpp"

namespace cleo
{

Force pr_str_object(Value val);
Force pr_str_array(Value val);
Force pr_str_array_set(Value s);
Force pr_str_persistent_hash_set(Value val);
Force pr_str_array_map(Value val);
Force pr_str_persistent_hash_map(Value val);
Force pr_str_seqable(Value v);

Force pr_str(Value val);

Force print_str(Value val);

}
