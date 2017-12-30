#pragma once
#include <array>
#include <cleo/list.hpp>
#include <cleo/small_vector.hpp>
#include <cleo/small_map.hpp>
#include <cleo/global.hpp>
#include <cleo/print.hpp>
#include <gtest/gtest.h>

#define ASSERT_EQ_VALS(ex, val) \
    ASSERT_TRUE(nil != are_equal(ex, val)) << "expected: " << to_string(ex) << ", actual: " << to_string(val);

#define EXPECT_EQ_VALS(ex, val) \
    EXPECT_TRUE(nil != are_equal(ex, val)) << "expected: " << to_string(ex) << ", actual: " << to_string(val);

#define EXPECT_EQ_REFS(ex, val) \
    EXPECT_TRUE(ex == val) << "expected: " << to_string(ex) << ", actual: " << to_string(val);

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

inline Force to_value(const std::string& s)
{
    return create_string(s);
}

inline Force to_value(Int64 n)
{
    return create_int64(n);
}

inline Force to_value(int n)
{
    return create_int64(n);
}

inline Force to_value(Value v)
{
    return force(v);
}

inline Force list()
{
    return create_list(nullptr, 0);
}

template <typename T, typename... Ts>
Force list(const T& first, const Ts&... elems)
{
    Root l{list(elems...)};
    Root val{to_value(first)};
    return list_conj(*l, *val);
}

inline Force svec_conj(Value vec)
{
    return force(vec);
}

template <typename T, typename... Ts>
Force svec_conj(Value vec, const T& first, const Ts&... elems)
{
    Root val{to_value(first)};
    Root nvec{small_vector_conj(vec, *val)};
    return svec_conj(*nvec, elems...);
}

template <typename... Ts>
Force svec(const Ts&... elems)
{
    Root vec{create_small_vector(nullptr, 0)};
    return svec_conj(*vec, elems...);
}

inline Force smap()
{
    return create_small_map();
}

template <typename K, typename V, typename... Rest>
Force smap(const K& k, const V& v, const Rest&... rest)
{
    Root m{smap(rest...)};
    Root key{to_value(k)};
    Root val{to_value(v)};
    return small_map_assoc(*m, *key, *val);
}

inline std::string to_string(Value val)
{
    Root s{pr_str(val)};
    return {get_string_ptr(*s), get_string_len(*s)};
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
