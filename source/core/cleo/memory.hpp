#pragma once
#include <cstddef>

namespace cleo
{

struct Allocation
{
    void *ptr{};
    std::size_t size{};
};

void *mem_alloc(std::size_t size);
void *mem_palloc(std::size_t size);

template <typename T>
inline T *alloc()
{
    return static_cast<T *>(mem_alloc(sizeof(T)));
}

template <typename T>
inline T *palloc()
{
    return static_cast<T *>(mem_palloc(sizeof(T)));
}

void gc();

std::size_t get_mem_used();
std::size_t get_mem_allocations();

}
