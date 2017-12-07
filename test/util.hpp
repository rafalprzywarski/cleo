#pragma once
#include <array>
#include <cleo/list.hpp>
#include <cleo/small_vector.hpp>
#include <cleo/global.hpp>
#include <gtest/gtest.h>

namespace cleo
{
namespace test
{

template <typename T>
class Override
{
public:
    Override(T& ref, T new_val) : ref(ref), old_val(ref)
    {
        ref = new_val;
    }
    ~Override()
    {
        ref = old_val;
    }

private:
    T& ref;
    T old_val;
};

template <typename... T>
Force list(T... elems)
{
    std::array<Value, sizeof...(T)> a{{elems...}};
    return create_list(a.data(), a.size());
}

template <typename... T>
Force svec(T... elems)
{
    std::array<Value, sizeof...(T)> a{{elems...}};
    return create_small_vector(a.data(), a.size());
}

inline Force i64(Int64 value)
{
    return create_int64(value);
}

struct Test : testing::Test
{
    Override<decltype(gc_frequency)> ovf{gc_frequency, 1};
    Override<decltype(gc_counter)> ovc{gc_counter, 1};
};

}
}
