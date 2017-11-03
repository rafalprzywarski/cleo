#include "memory.hpp"
#include <cstdlib>

namespace cleo
{

void *mem_alloc(std::size_t size)
{
    return std::malloc(size);
}

}
