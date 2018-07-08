#pragma once
#include <array>
#include <cleo/list.hpp>
#include <cleo/array.hpp>
#include <cleo/array_map.hpp>
#include <cleo/persistent_hash_map.hpp>
#include <cleo/array_set.hpp>
#include <cleo/global.hpp>
#include <cleo/print.hpp>
#include <cleo/namespace.hpp>
#include <cleo/var.hpp>
#include <cleo/util.hpp>
#include <cleo/fn_call.hpp>
#include <gtest/gtest.h>

#define ASSERT_EQ_VALS(ex, val) \
    ASSERT_TRUE((ex) == (val)) << "  Actual value: " << to_string(val) << "\nExpected: " << to_string(ex) << "\n"

#define EXPECT_EQ_VALS(ex, val) \
    EXPECT_TRUE((ex) == (val)) << "  Actual value: " << to_string(val) << "\nExpected: " << to_string(ex) << "\n"

#define EXPECT_EQ_VALS_ALT2(ex1, ex2, val) \
    EXPECT_TRUE((ex1) == (val) || (ex2) == (val)) << "  Actual value: " << to_string(val) << "\nExpected: " << to_string(ex1) << "\nOr: " << to_string(ex2) << "\n";

#define ASSERT_EQ_REFS(ex, val) \
    ASSERT_TRUE((val).is(ex)) << "  Actual value: " << to_string(val) << "\nExpected value: " << to_string(ex) << "\n"

#define EXPECT_EQ_REFS(ex, val) \
    EXPECT_TRUE((val).is(ex)) << "  Actual value: " << to_string(val) << "\nExpected value: " << to_string(ex) << "\n"

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

template <typename F>
struct Delayed
{
    F f;
    Delayed(F f) : f(std::move(f)) { }
};

template <typename F>
Delayed<F> delayed(F f)
{
    return {std::move(f)};
}

template <typename F>
inline Force to_value(const Delayed<F>& d)
{
    return d.f();
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

template <typename... Ts>
auto listv(const Ts&... elems)
{
    return delayed([=] { return list(elems...); });
}

inline Force svec_conj(Value vec)
{
    return force(vec);
}

template <typename T, typename... Ts>
Force svec_conj(Value vec, const T& first, const Ts&... elems)
{
    Root val{to_value(first)};
    Root nvec{array_conj(vec, *val)};
    return svec_conj(*nvec, elems...);
}

template <typename... Ts>
Force array(const Ts&... elems)
{
    Root s{create_array(nullptr, 0)};
    return svec_conj(*s, elems...);
}

template <typename... Ts>
auto arrayv(const Ts&... elems)
{
    return delayed([=] { return array(elems...); });
}

inline void push_args(Roots&, std::vector<Value>&s) { }

template <typename T, typename... Ts>
void push_args(Roots& rs, std::vector<Value>& es, const T& elem, const Ts&... elems)
{
    rs.set(es.size(), to_value(elem));
    es.push_back(rs[es.size()]);
    push_args(rs, es, elems...);
}

template <typename... Ts>
auto fn_call(Value fn, const Ts&... elems)
{
    Roots rs(1 + sizeof...(elems));
    std::vector<Value> es;
    es.reserve(1 + sizeof...(elems));
    push_args(rs, es, fn, elems...);
    return create_fn_call(es.data(), es.size());
}

template <typename... Ts>
auto fn_callv(Value fn, const Ts&... elems)
{
    return delayed([=] { return fn_call(fn, elems...); });
}

inline Force sset_conj(Value s)
{
    return force(s);
}

template <typename T, typename... Ts>
Force sset_conj(Value s, const T& first, const Ts&... elems)
{
    Root val{to_value(first)};
    Root ns{array_set_conj(s, *val)};
    return sset_conj(*ns, elems...);
}

template <typename... Ts>
Force aset(const Ts&... elems)
{
    Root vec{create_array_set()};
    return sset_conj(*vec, elems...);
}

inline Force amap()
{
    return create_array_map();
}

template <typename K, typename V, typename... Rest>
Force amap(const K& k, const V& v, const Rest&... rest)
{
    Root m{amap(rest...)};
    Root key{to_value(k)};
    Root val{to_value(v)};
    return array_map_assoc(*m, *key, *val);
}

inline Force phmap()
{
    return create_persistent_hash_map();
}

template <typename K, typename V, typename... Rest>
Force phmap(const K& k, const V& v, const Rest&... rest)
{
    Root m{phmap(rest...)};
    Root key{to_value(k)};
    Root val{to_value(v)};
    return persistent_hash_map_assoc(*m, *key, *val);
}

template <typename... Ts>
auto phmapv(const Ts&... elems)
{
    return delayed([&] { return phmap(elems...); });
}

inline Force i64(Int64 value)
{
    return create_int64(value);
}

struct Test : testing::Test
{
    Override<decltype(gc_frequency)> ovf{gc_frequency, 1};
    Override<decltype(gc_counter)> ovc{gc_counter, 1};
    std::unique_ptr<PushBindingsGuard> bindings_guard;

    Test(const std::string& ns)
    {
        Root bindings{amap(CURRENT_NS, *rt::current_ns)};
        bindings_guard.reset(new PushBindingsGuard(*bindings));
        in_ns(create_symbol(ns));
        EXPECT_TRUE(stack.empty());
    }

    ~Test()
    {
        stack.clear();
    }

    void TearDown() override
    {
        current_exception = nil;
        rt::current_ns = nil;
    }
};

}
}
