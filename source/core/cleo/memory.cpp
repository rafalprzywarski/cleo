#include "memory.hpp"
#include "value.hpp"
#include <cstdlib>

namespace cleo
{

void *mem_alloc(std::size_t size)
{
    auto ptr = std::malloc(size);
    if ((reinterpret_cast<std::uintptr_t>(ptr) & tag::MASK) != 0)
        std::abort();
    return ptr;
}

}
