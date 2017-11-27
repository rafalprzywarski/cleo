#pragma once
#include <array>
#include <cleo/list.hpp>

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

}
}
