#include "memory.hpp"
#include "value.hpp"
#include "global.hpp"
#include <cstdlib>
#include <algorithm>
#ifndef __APPLE__
#include <malloc.h>
#endif

namespace cleo
{

namespace
{

constexpr auto OFFSET = tag::MASK + 1;

char& tag_ref(void *ptr)
{
    return *(reinterpret_cast<char *>(ptr) - OFFSET);
}

bool is_marked(void *ptr)
{
    return tag_ref(ptr) != 0;
}

void mark_value(Value val)
{
    if (val.is_nil() || is_marked(get_value_ptr(val)))
        return;
    tag_ref(get_value_ptr(val)) = 1;

    switch (get_value_tag(val))
    {
        case tag::SYMBOL:
            mark_value(get_symbol_namespace(val));
            mark_value(get_symbol_name(val));
            break;
        case tag::KEYWORD:;
            mark_value(get_keyword_namespace(val));
            mark_value(get_keyword_name(val));
            break;
        case tag::OBJECT:
            {
                mark_value(get_object_type(val));
                auto size = get_object_size(val);
                for (decltype(get_object_size(val)) i = 0; i != size; ++i)
                    mark_value(get_object_element(val, i));
            }
            break;
        default: break;
    }
}

void unmark(void *ptr)
{
    tag_ref(ptr) = 0;
}

void mark_symbols()
{
    for (auto& ns : symbols)
        for (auto& sym : ns.second)
            mark_value(sym.second);
}

void mark_keyword()
{
    for (auto& ns : keywords)
        for (auto& sym : ns.second)
            mark_value(sym.second);
}

void mark_vars()
{
    for (auto& var : vars)
        mark_value(var.second);
}

void mark_multimethods()
{
    for (auto& mm : multimethods)
    {
        mark_value(mm.second.dispatchFn);
        mark_value(mm.second.defaultDispatchVal);

        for (auto& fn : mm.second.fns)
        {
            mark_value(fn.first);
            mark_value(fn.second);
        }

        for (auto& fn : mm.second.memoized_fns)
            mark_value(fn.first);
    }
}

void mark_global_hierarchy()
{
    for (auto& entry : global_hierarchy.ancestors)
        mark_value(entry.first);
}

void mark_extra_roots()
{
    for (auto val : extra_roots)
        mark_value(val);
}

}

void *mem_alloc(std::size_t size)
{
    if (gc_counter == 0)
    {
        gc_counter = gc_frequency;
        gc();
    }
    --gc_counter;
#ifdef __APPLE__
    auto ptr = reinterpret_cast<char *>(std::malloc(OFFSET + size)) + OFFSET;
#else
    auto ptr = reinterpret_cast<char *>(memalign(tag::MASK + 1, OFFSET + size)) + OFFSET;
#endif
    unmark(ptr);
    allocations.push_back(ptr);
    if ((reinterpret_cast<std::uintptr_t>(ptr) & tag::MASK) != 0)
        std::abort();
    return ptr;
}

void mem_free(void *ptr)
{
    std::free(reinterpret_cast<char *>(ptr) - OFFSET);
}

void gc()
{
    mark_symbols();
    mark_keyword();
    mark_vars();
    mark_multimethods();
    mark_global_hierarchy();
    mark_extra_roots();

    auto middle = std::partition(begin(allocations), end(allocations), is_marked);
    std::for_each(middle, end(allocations), mem_free);
    allocations.erase(middle, end(allocations));

    for (auto ptr : allocations)
        unmark(ptr);
}

}
