#include "memory.hpp"
#include "value.hpp"
#include "global.hpp"
#include <cstdlib>
#include <algorithm>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <chrono>

namespace cleo
{

namespace
{

constexpr std::uintptr_t OFFSET = sizeof(ValueBits);

char& tag_ref(void *ptr)
{
    return *(reinterpret_cast<char *>(ptr) - OFFSET);
}

bool is_ptr_marked(void *ptr)
{
    return tag_ref(ptr) != 0;
}

bool is_marked(const Allocation& a)
{
    return is_ptr_marked(a.ptr);
}

void mark_value(Value val)
{
    static std::vector<Value> vals;
    assert(vals.empty());
    vals.push_back(val);
    while (!vals.empty())
    {
        val = vals.back();
        vals.pop_back();
        if (val.is_nil() || !is_value_ptr(val) || is_ptr_marked(get_value_ptr(val)))
            continue;
        tag_ref(get_value_ptr(val)) = 1;

        switch (get_value_tag(val))
        {
            case tag::SYMBOL:
                vals.push_back(get_symbol_namespace(val));
                vals.push_back(get_symbol_name(val));
                break;
            case tag::KEYWORD:;
                vals.push_back(get_keyword_namespace(val));
                vals.push_back(get_keyword_name(val));
                break;
            case tag::OBJECT:
                {
                    vals.push_back(get_object_type(val));
                    auto size = get_object_size(val);
                    for (decltype(get_object_size(val)) i = 0; i != size; ++i)
                        if (is_object_dynamic(val))
                            vals.push_back(get_dynamic_object_element(val, i));
                        else
                            if (is_object_element_value(val, i))
                                vals.push_back(get_static_object_element(val, i));
                }
                break;
            case tag::OBJECT_TYPE:
                {
                    auto size = get_object_type_field_count(val);
                    for (decltype(size) i = 0; i != size; ++i)
                        vals.push_back(get_object_type_field_type(val, i));
                }
                break;
            default: break;
        }
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

void mark_stack()
{
    for (auto val : stack)
        mark_value(val);
}

std::int64_t get_time()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
}

template <typename FwdIt>
std::pair<std::size_t, std::vector<std::pair<std::size_t, unsigned>>> collect_alloc_stats(FwdIt first, FwdIt last)
{
    if (!gc_log)
        return {};
    std::unordered_map<std::size_t, unsigned> sf;
    std::size_t total = 0;
    for (auto a = first; a != last; ++a)
    {
        sf[a->size]++;
        total += a->size;
    }

    std::vector<std::pair<std::size_t, unsigned>> ssf{begin(sf), end(sf)};
    std::sort(begin(ssf), end(ssf));
    return {total, std::move(ssf)};
}

    void log_stats(const std::pair<std::size_t, std::vector<std::pair<std::size_t, unsigned>>>& stats, std::int64_t t0, std::int64_t t1, std::int64_t t2, std::int64_t t3, std::int64_t t4)
{
    if (!gc_log)
        return;
    *gc_log << "freed " << stats.first << " bytes\nalloc size/count:";

    for (auto& sf : stats.second)
        *gc_log << " " << sf.first << ": " << sf.second;
    *gc_log << "\nGC time: " << ((t1 - t0) + (t4 - t2)) << " ms, freeing time: " << (t3 - t2) << "ms, total time: " << (t4 - t0) << " ms" << std::endl;
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
    auto ptr = reinterpret_cast<char *>(std::malloc(OFFSET + size));
#else
    auto ptr = reinterpret_cast<char *>(memalign(sizeof(ValueBits), OFFSET + size));
#endif
    if (ptr == nullptr)
        std::abort();
    ptr += OFFSET;
    unmark(ptr);
    allocations.push_back({ptr, size + OFFSET});
    return ptr;
}

void mem_free(const Allocation& a)
{
    std::free(reinterpret_cast<char *>(a.ptr) - OFFSET);
}

void gc()
{
    auto t0 = get_time();
    mark_symbols();
    mark_keyword();
    mark_vars();
    mark_multimethods();
    mark_global_hierarchy();
    mark_extra_roots();
    mark_stack();

    auto middle = std::partition(begin(allocations), end(allocations), is_marked);
    auto t1 = get_time();
    auto stats = collect_alloc_stats(middle, end(allocations));
    auto t2 = get_time();
    std::for_each(middle, end(allocations), mem_free);
    auto t3 = get_time();
    allocations.erase(middle, end(allocations));

    for (auto a : allocations)
        unmark(a.ptr);

    auto t4 = get_time();
    log_stats(stats, t0, t1, t2, t3, t4);
}

std::size_t get_mem_used()
{
    std::size_t total = 0;
    for (auto a : allocations)
        total += a.size;
    return total;
}

std::size_t get_mem_allocations()
{
    return allocations.size();
}

}
