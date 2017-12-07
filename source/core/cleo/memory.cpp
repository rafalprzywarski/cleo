#include "memory.hpp"
#include "value.hpp"
#include "global.hpp"
#include <cstdlib>

namespace cleo
{

namespace
{

constexpr auto OFFSET = sizeof(void *);

char& tag_ref(void *ptr)
{
    return *(reinterpret_cast<char *>(ptr) - OFFSET);
}

void mark_value(Value val)
{
    if (val == nil)
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

bool is_not_marked(void *ptr)
{
    return tag_ref(ptr) == 0;
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
    auto ptr = reinterpret_cast<char *>(std::malloc(OFFSET + size)) + OFFSET;
    unmark(ptr);
    allocations.push_back(ptr);
    if ((reinterpret_cast<std::uintptr_t>(ptr) & tag::MASK) != 0)
        std::abort();
    return ptr;
}

void gc()
{
    for (auto val : extra_roots)
        mark_value(val);

    allocations.erase(std::remove_if(begin(allocations), end(allocations), is_not_marked), end(allocations));

    for (auto ptr : allocations)
        unmark(ptr);
}

}
