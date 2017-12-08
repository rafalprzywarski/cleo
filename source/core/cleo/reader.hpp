#pragma once
#include "value.hpp"
#include <stdexcept>

namespace cleo
{

struct reader_error : std::runtime_error
{
    reader_error(const std::string& msg) : std::runtime_error(msg) {}
};

Force read(Value source);

}
