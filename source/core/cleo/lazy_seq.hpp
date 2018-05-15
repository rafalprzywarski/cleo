#pragma once
#include "value.hpp"

namespace cleo
{

Force create_lazy_seq(Value fn);
Force lazy_seq_seq(Value ls);
Force lazy_seq_first(Value ls);
Force lazy_seq_next(Value ls);
Force lazy_seq_conj(Value ls, Value elem);

}
