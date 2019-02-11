#pragma once
#include "value.hpp"

namespace cleo
{

Force compile_fn(Value form);
Force serialize_fn(Value fn);
Force deserialize_fn(Value fn);

}
