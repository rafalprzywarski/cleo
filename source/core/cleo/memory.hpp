#pragma once
#include <cstddef>

namespace cleo
{

void *mem_alloc(std::size_t size);

template <typename T>
inline T *alloc()
{
    return static_cast<T *>(mem_alloc(sizeof(T)));
}

}
