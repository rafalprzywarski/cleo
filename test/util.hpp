#pragma once
#include <array>
#include <cleo/list.hpp>
#include <cleo/small_vector.hpp>

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

inline Value i64(Int64 value)
{
    return create_int64(value);
}

}
}
