#include "value.hpp"

namespace cleo
{

Force create_c_fn(void *cfn, Value name, Value ret_type, Value param_types);
Force call_c_function(const Value *args, std::uint8_t num_args);
Force import_c_fn(Value libname, Value fnname, Value calling_conv, Value ret_type, Value param_types);

}
