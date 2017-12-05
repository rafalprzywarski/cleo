#pragma once
#include <array>
#include <cleo/list.hpp>
#include <cleo/small_vector.hpp>

namespace cleo
{
namespace test
{

template <typename... T>
Value list(T... elems)
{
    std::array<Value, sizeof...(T)> a{{elems...}};
    return create_list(a.data(), a.size());
}

template <typename... T>
Value svec(T... elems)
{
    std::array<Value, sizeof...(T)> a{{elems...}};
    return create_small_vector(a.data(), a.size());
}

}
}
