#include <cleo/equality.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

TEST(equality_test, same_instances_should_be_equal)
{
    auto fn = create_native_function([](const Value *, std::uint8_t) { return nil; });
    auto sym = create_symbol("org.xyz", "eqsym");
    auto kw = create_keyword("org.xyz", "eqkw");
    auto i = create_int64(7);
    auto flt = create_float64(3.5);
    auto s = create_string("abcd");
    auto o = create_object0(sym);

    ASSERT_TRUE(nil != are_equal(nil, nil));
    ASSERT_TRUE(nil == are_equal(nil, fn));
    ASSERT_TRUE(nil == are_equal(nil, sym));
    ASSERT_TRUE(nil == are_equal(nil, kw));
    ASSERT_TRUE(nil == are_equal(nil, i));
    ASSERT_TRUE(nil == are_equal(nil, flt));
    ASSERT_TRUE(nil == are_equal(nil, s));
    ASSERT_TRUE(nil == are_equal(nil, o));

    ASSERT_TRUE(nil == are_equal(fn, nil));
    ASSERT_TRUE(nil != are_equal(fn, fn));
    ASSERT_TRUE(nil == are_equal(fn, sym));
    ASSERT_TRUE(nil == are_equal(fn, kw));
    ASSERT_TRUE(nil == are_equal(fn, i));
    ASSERT_TRUE(nil == are_equal(fn, flt));
    ASSERT_TRUE(nil == are_equal(fn, s));
    ASSERT_TRUE(nil == are_equal(fn, o));

    ASSERT_TRUE(nil == are_equal(sym, nil));
    ASSERT_TRUE(nil == are_equal(sym, fn));
    ASSERT_TRUE(nil != are_equal(sym, sym));
    ASSERT_TRUE(nil == are_equal(sym, kw));
    ASSERT_TRUE(nil == are_equal(sym, i));
    ASSERT_TRUE(nil == are_equal(sym, flt));
    ASSERT_TRUE(nil == are_equal(sym, s));
    ASSERT_TRUE(nil == are_equal(sym, o));

    ASSERT_TRUE(nil == are_equal(kw, nil));
    ASSERT_TRUE(nil == are_equal(kw, fn));
    ASSERT_TRUE(nil == are_equal(kw, sym));
    ASSERT_TRUE(nil != are_equal(kw, kw));
    ASSERT_TRUE(nil == are_equal(kw, i));
    ASSERT_TRUE(nil == are_equal(kw, flt));
    ASSERT_TRUE(nil == are_equal(kw, s));
    ASSERT_TRUE(nil == are_equal(kw, o));

    ASSERT_TRUE(nil == are_equal(i, nil));
    ASSERT_TRUE(nil == are_equal(i, fn));
    ASSERT_TRUE(nil == are_equal(i, sym));
    ASSERT_TRUE(nil == are_equal(i, kw));
    ASSERT_TRUE(nil != are_equal(i, i));
    ASSERT_TRUE(nil == are_equal(i, flt));
    ASSERT_TRUE(nil == are_equal(i, s));
    ASSERT_TRUE(nil == are_equal(i, o));

    ASSERT_TRUE(nil == are_equal(flt, nil));
    ASSERT_TRUE(nil == are_equal(flt, fn));
    ASSERT_TRUE(nil == are_equal(flt, sym));
    ASSERT_TRUE(nil == are_equal(flt, kw));
    ASSERT_TRUE(nil == are_equal(flt, i));
    ASSERT_TRUE(nil != are_equal(flt, flt));
    ASSERT_TRUE(nil == are_equal(flt, s));
    ASSERT_TRUE(nil == are_equal(flt, o));

    ASSERT_TRUE(nil == are_equal(s, nil));
    ASSERT_TRUE(nil == are_equal(s, fn));
    ASSERT_TRUE(nil == are_equal(s, sym));
    ASSERT_TRUE(nil == are_equal(s, kw));
    ASSERT_TRUE(nil == are_equal(s, i));
    ASSERT_TRUE(nil == are_equal(s, flt));
    ASSERT_TRUE(nil != are_equal(s, s));
    ASSERT_TRUE(nil == are_equal(s, o));

    ASSERT_TRUE(nil == are_equal(o, nil));
    ASSERT_TRUE(nil == are_equal(o, fn));
    ASSERT_TRUE(nil == are_equal(o, sym));
    ASSERT_TRUE(nil == are_equal(o, kw));
    ASSERT_TRUE(nil == are_equal(o, i));
    ASSERT_TRUE(nil == are_equal(o, flt));
    ASSERT_TRUE(nil == are_equal(o, s));
    ASSERT_TRUE(nil != are_equal(o, o));
}

TEST(equality_test, should_compare_integers)
{
    ASSERT_TRUE(nil != are_equal(create_int64(10), create_int64(10)));
    ASSERT_TRUE(nil == are_equal(create_int64(10), create_int64(20)));
}

TEST(equality_test, should_compare_floats)
{
    ASSERT_TRUE(nil != are_equal(create_float64(10.25), create_float64(10.25)));
    ASSERT_TRUE(nil == are_equal(create_float64(10.25), create_float64(20.25)));
}

TEST(equality_test, should_compare_strings)
{
    ASSERT_TRUE(nil != are_equal(create_string(""), create_string("")));
    ASSERT_TRUE(nil != are_equal(create_string("abc"), create_string("abc")));
    ASSERT_TRUE(nil == are_equal(create_string("abcd"), create_string("abc")));
    ASSERT_TRUE(nil == are_equal(create_string("abcd"), create_string("abce")));
    ASSERT_TRUE(nil == are_equal(create_string(std::string("ab\0cd", 5)), create_string(std::string("ab\0ce", 5))));
}

TEST(equality_test, objects_should_not_be_equal)
{
    auto type = create_symbol("cleo.equality.test", "sometype");
    ASSERT_TRUE(nil == are_equal(create_object0(type), create_object0(type)));
}

TEST(equality_test, should_compare_small_vectors)
{
    auto type = create_symbol("cleo.equality.test", "not-vector");
    EXPECT_TRUE(nil == are_equal(svec(), create_object0(type)));
    EXPECT_TRUE(nil == are_equal(create_object0(type), svec()));

    EXPECT_TRUE(nil != are_equal(svec(), svec()));
    EXPECT_TRUE(nil == are_equal(svec(i64(10)), svec()));
    EXPECT_TRUE(nil != are_equal(svec(i64(10)), svec(i64(10))));
    EXPECT_TRUE(nil == are_equal(svec(i64(10)), svec(i64(9))));
    EXPECT_TRUE(nil != are_equal(svec(i64(10), i64(11), i64(12)), svec(i64(10), i64(11), i64(12))));
    EXPECT_TRUE(nil == are_equal(svec(i64(9), i64(11), i64(12)), svec(i64(10), i64(11), i64(12))));
    EXPECT_TRUE(nil == are_equal(svec(i64(10), i64(9), i64(12)), svec(i64(10), i64(11), i64(12))));
    EXPECT_TRUE(nil == are_equal(svec(i64(10), i64(11), i64(9)), svec(i64(10), i64(11), i64(12))));

    EXPECT_TRUE(nil != are_equal(svec(svec(i64(10), i64(11)), i64(12)), svec(svec(i64(10), i64(11)), i64(12))));
    EXPECT_TRUE(nil == are_equal(svec(svec(i64(10)), i64(11), i64(12)), svec(svec(i64(10), i64(11)), i64(12))));
}

TEST(equality_test, should_compare_sequences)
{
    ASSERT_TRUE(nil != are_equal(list(), list()));
    ASSERT_TRUE(nil == are_equal(list(i64(10), i64(20)), list(i64(1))));
    ASSERT_TRUE(nil != are_equal(list(i64(10), i64(20)), list(i64(10), i64(20))));
    ASSERT_TRUE(nil == are_equal(list(i64(10), i64(20)), list(i64(10), i64(21))));

    ASSERT_TRUE(nil != are_equal(svec(), list()));
    ASSERT_TRUE(nil == are_equal(svec(), list(i64(1))));
    ASSERT_TRUE(nil == are_equal(svec(i64(20)), list()));
    ASSERT_TRUE(nil == are_equal(svec(i64(10), i64(20)), list(i64(1))));
    ASSERT_TRUE(nil != are_equal(svec(i64(10), i64(20)), list(i64(10), i64(20))));
    ASSERT_TRUE(nil == are_equal(svec(i64(10), i64(20)), list(i64(10), i64(21))));

    ASSERT_TRUE(nil != are_equal(list(), svec()));
    ASSERT_TRUE(nil == are_equal(list(i64(10), i64(20)), svec(i64(1))));
    ASSERT_TRUE(nil != are_equal(list(i64(10), i64(20)), svec(i64(10), i64(20))));
    ASSERT_TRUE(nil == are_equal(list(i64(10), i64(20)), svec(i64(10), i64(21))));
}

}
}
