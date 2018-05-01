#include <cleo/equality.hpp>
#include <cleo/global.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct equality_test : Test
{
    equality_test() : Test("cleo.equality.test") { }
};

TEST_F(equality_test, same_instances_should_be_equal)
{
    Root fn{create_native_function([](const Value *, std::uint8_t) { return force(nil); })};
    auto sym = create_symbol("org.xyz", "eqsym");
    auto kw = create_keyword("org.xyz", "eqkw");
    Root i{create_int64(7)};
    Root flt{create_float64(3.5)};
    Root s{create_string("abcd")};
    Root o{create_object0(sym)};

    ASSERT_TRUE(bool(are_equal(nil, nil)));
    ASSERT_FALSE(bool(are_equal(nil, *fn)));
    ASSERT_FALSE(bool(are_equal(nil, sym)));
    ASSERT_FALSE(bool(are_equal(nil, kw)));
    ASSERT_FALSE(bool(are_equal(nil, *i)));
    ASSERT_FALSE(bool(are_equal(nil, *flt)));
    ASSERT_FALSE(bool(are_equal(nil, *s)));
    ASSERT_FALSE(bool(are_equal(nil, *o)));

    ASSERT_FALSE(bool(are_equal(*fn, nil)));
    ASSERT_TRUE(bool(are_equal(*fn, *fn)));
    ASSERT_FALSE(bool(are_equal(*fn, sym)));
    ASSERT_FALSE(bool(are_equal(*fn, kw)));
    ASSERT_FALSE(bool(are_equal(*fn, *i)));
    ASSERT_FALSE(bool(are_equal(*fn, *flt)));
    ASSERT_FALSE(bool(are_equal(*fn, *s)));
    ASSERT_FALSE(bool(are_equal(*fn, *o)));

    ASSERT_FALSE(bool(are_equal(sym, nil)));
    ASSERT_FALSE(bool(are_equal(sym, *fn)));
    ASSERT_TRUE(bool(are_equal(sym, sym)));
    ASSERT_FALSE(bool(are_equal(sym, kw)));
    ASSERT_FALSE(bool(are_equal(sym, *i)));
    ASSERT_FALSE(bool(are_equal(sym, *flt)));
    ASSERT_FALSE(bool(are_equal(sym, *s)));
    ASSERT_FALSE(bool(are_equal(sym, *o)));

    ASSERT_FALSE(bool(are_equal(kw, nil)));
    ASSERT_FALSE(bool(are_equal(kw, *fn)));
    ASSERT_FALSE(bool(are_equal(kw, sym)));
    ASSERT_TRUE(bool(are_equal(kw, kw)));
    ASSERT_FALSE(bool(are_equal(kw, *i)));
    ASSERT_FALSE(bool(are_equal(kw, *flt)));
    ASSERT_FALSE(bool(are_equal(kw, *s)));
    ASSERT_FALSE(bool(are_equal(kw, *o)));

    ASSERT_FALSE(bool(are_equal(*i, nil)));
    ASSERT_FALSE(bool(are_equal(*i, *fn)));
    ASSERT_FALSE(bool(are_equal(*i, sym)));
    ASSERT_FALSE(bool(are_equal(*i, kw)));
    ASSERT_TRUE(bool(are_equal(*i, *i)));
    ASSERT_FALSE(bool(are_equal(*i, *flt)));
    ASSERT_FALSE(bool(are_equal(*i, *s)));
    ASSERT_FALSE(bool(are_equal(*i, *o)));

    ASSERT_FALSE(bool(are_equal(*flt, nil)));
    ASSERT_FALSE(bool(are_equal(*flt, *fn)));
    ASSERT_FALSE(bool(are_equal(*flt, sym)));
    ASSERT_FALSE(bool(are_equal(*flt, kw)));
    ASSERT_FALSE(bool(are_equal(*flt, *i)));
    ASSERT_TRUE(bool(are_equal(*flt, *flt)));
    ASSERT_FALSE(bool(are_equal(*flt, *s)));
    ASSERT_FALSE(bool(are_equal(*flt, *o)));

    ASSERT_FALSE(bool(are_equal(*s, nil)));
    ASSERT_FALSE(bool(are_equal(*s, *fn)));
    ASSERT_FALSE(bool(are_equal(*s, sym)));
    ASSERT_FALSE(bool(are_equal(*s, kw)));
    ASSERT_FALSE(bool(are_equal(*s, *i)));
    ASSERT_FALSE(bool(are_equal(*s, *flt)));
    ASSERT_TRUE(bool(are_equal(*s, *s)));
    ASSERT_FALSE(bool(are_equal(*s, *o)));

    ASSERT_FALSE(bool(are_equal(*o, nil)));
    ASSERT_FALSE(bool(are_equal(*o, *fn)));
    ASSERT_FALSE(bool(are_equal(*o, sym)));
    ASSERT_FALSE(bool(are_equal(*o, kw)));
    ASSERT_FALSE(bool(are_equal(*o, *i)));
    ASSERT_FALSE(bool(are_equal(*o, *flt)));
    ASSERT_FALSE(bool(are_equal(*o, *s)));
    ASSERT_TRUE(bool(are_equal(*o, *o)));
}

TEST_F(equality_test, should_compare_integers)
{
    Root val1, val2;
    val1 = create_int64(10);
    val2 = create_int64(10);
    ASSERT_TRUE(bool(are_equal(*val1, *val2)));
    val2 = create_int64(20);
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));
}

TEST_F(equality_test, should_compare_floats)
{
    Root val1, val2;
    val1 = create_float64(10.25);
    val2 = create_float64(10.25);
    ASSERT_TRUE(bool(are_equal(*val1, *val2)));
    val2 = create_float64(20.25);
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));
}

TEST_F(equality_test, should_compare_strings)
{
    Root val1, val2;
    val1 = create_string("");
    val2 = create_string("");
    ASSERT_TRUE(bool(are_equal(*val1, *val2)));
    val1 = create_string("abc");
    val2 = create_string("abc");
    ASSERT_TRUE(bool(are_equal(*val1, *val2)));
    val1 = create_string("abcd");
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));
    val2 = create_string("abce");
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));
    val1 = create_string(std::string("ab\0cd", 5));
    val2 = create_string(std::string("ab\0ce", 5));
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));
}

TEST_F(equality_test, objects_should_not_be_equal)
{
    auto type = create_symbol("cleo.equality.test", "sometype");
    Root val1, val2;
    val1 = create_object0(type);
    val2 = create_object0(type);
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));
}

TEST_F(equality_test, should_compare_small_vectors)
{
    Root n9, n10, n11, n12, val1, val2;
    n9 = i64(9);
    n10 = i64(10);
    n11 = i64(11);
    n12 = i64(12);
    auto type = create_symbol("cleo.equality.test", "not-vector");
    val1 = svec();
    val2 = create_object0(type);
    EXPECT_FALSE(bool(are_equal(*val1, *val2)));
    EXPECT_FALSE(bool(are_equal(*val2, *val1)));

    val1 = svec();
    val2 = svec();
    EXPECT_TRUE(bool(are_equal(*val1, *val2)));
    val1 = svec(*n10);
    EXPECT_FALSE(bool(are_equal(*val1, *val2)));
    val2 = svec(*n10);
    EXPECT_TRUE(bool(are_equal(*val1, *val2)));
    val2 = svec(*n9);
    EXPECT_FALSE(bool(are_equal(*val1, *val2)));
    val1 = svec(*n10, *n11, *n12);
    val2 = svec(*n10, *n11, *n12);
    EXPECT_TRUE(bool(are_equal(*val1, *val2)));
    val1 = svec(*n9, *n11, *n12);
    EXPECT_FALSE(bool(are_equal(*val1, *val2)));
    val1 = svec(*n10, *n9, *n12);
    EXPECT_FALSE(bool(are_equal(*val1, *val2)));
    val1 = svec(*n10, *n11, *n9);
    EXPECT_FALSE(bool(are_equal(*val1, *val2)));

    val1 = svec(*n10, *n11);
    val1 = svec(*val1, *n12);
    val2 = svec(*n10, *n11);
    val2 = svec(*val2, *n12);
    EXPECT_TRUE(bool(are_equal(*val1, *val2)));
    val1 = svec(*n10);
    val1 = svec(*val1, *n11, *n12);
    val2 = svec(*n10, *n11);
    val2 = svec(*val2, *n12);
    EXPECT_FALSE(bool(are_equal(*val1, *val2)));
}

TEST_F(equality_test, should_compare_sequences)
{
    Root n1, n10, n20, n21, val1, val2;
    n1 = i64(1);
    n10 = i64(10);
    n20 = i64(20);
    n21 = i64(21);

    val1 = list();
    val2 = list();
    ASSERT_TRUE(bool(are_equal(*val1, *val2)));
    val1 = list(*n10, *n20);
    val2 = list(*n1);
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));
    val2 = list(*n10, *n20);
    ASSERT_TRUE(bool(are_equal(*val1, *val2)));
    val2 = list(*n10, *n21);
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));

    val1 = svec();
    val2 = list();
    ASSERT_TRUE(bool(are_equal(*val1, *val2)));
    val2 = list(*n1);
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));
    val1 = svec(*n20);
    val2 = list();
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));
    val1 = svec(*n10, *n20);
    val2 = list(*n1);
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));
    val2 = list(*n10, *n20);
    ASSERT_TRUE(bool(are_equal(*val1, *val2)));
    val2 = list(*n10, *n21);
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));

    val1 = list();
    val2 = svec();
    ASSERT_TRUE(bool(are_equal(*val1, *val2)));
    val1 = list(*n10, *n20);
    val2 = svec(*n1);
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));
    val2 = svec(*n10, *n20);
    ASSERT_TRUE(bool(are_equal(*val1, *val2)));
    val2 = svec(*n10, *n21);
    ASSERT_FALSE(bool(are_equal(*val1, *val2)));
}

TEST_F(equality_test, should_compare_array_maps)
{
    Root m1, m2;
    m1 = amap();
    m2 = amap();
    EXPECT_TRUE(bool(are_equal(*m1, *m2)));

    m1 = amap(3, 4);
    m2 = amap();
    EXPECT_FALSE(bool(are_equal(*m1, *m2)));
    EXPECT_FALSE(bool(are_equal(*m2, *m1)));

    m1 = amap(10, 20, 30, 40, 50, 60);
    m2 = amap(30, 40, 50, 60, 10, 20);
    EXPECT_TRUE(bool(are_equal(*m1, *m2)));
    EXPECT_TRUE(bool(are_equal(*m2, *m1)));

    m1 = amap(10, 99, 30, 40, 50, 60);
    m2 = amap(30, 40, 50, 60, 10, 20);
    EXPECT_FALSE(bool(are_equal(*m1, *m2)));
    EXPECT_FALSE(bool(are_equal(*m2, *m1)));

    m1 = amap(10, 20, 30, 99, 50, 60);
    m2 = amap(30, 40, 50, 60, 10, 20);
    EXPECT_FALSE(bool(are_equal(*m1, *m2)));
    EXPECT_FALSE(bool(are_equal(*m2, *m1)));

    m1 = amap(10, 20, 30, 40, 50, nil);
    m2 = amap(30, 40, 10, 20);
    EXPECT_FALSE(bool(are_equal(*m1, *m2)));
    EXPECT_FALSE(bool(are_equal(*m2, *m1)));
}

TEST_F(equality_test, should_compare_maps)
{
    Root m1, m2;
    m1 = amap();
    m2 = phmap();
    EXPECT_TRUE(bool(are_equal(*m1, *m2)));

    m1 = amap(3, 4);
    m2 = phmap();
    EXPECT_FALSE(bool(are_equal(*m1, *m2)));
    EXPECT_FALSE(bool(are_equal(*m2, *m1)));

    m1 = amap(10, 20, 30, 40, 50, 60);
    m2 = phmap(30, 40, 50, 60, 10, 20);
    EXPECT_TRUE(bool(are_equal(*m1, *m2)));
    EXPECT_TRUE(bool(are_equal(*m2, *m1)));

    m1 = amap(10, 99, 30, 40, 50, 60);
    m2 = phmap(30, 40, 50, 60, 10, 20);
    EXPECT_FALSE(bool(are_equal(*m1, *m2)));
    EXPECT_FALSE(bool(are_equal(*m2, *m1)));

    m1 = amap(10, 20, 30, 99, 50, 60);
    m2 = phmap(30, 40, 50, 60, 10, 20);
    EXPECT_FALSE(bool(are_equal(*m1, *m2)));
    EXPECT_FALSE(bool(are_equal(*m2, *m1)));

    m1 = amap(10, 20, 30, 40, 50, nil);
    m2 = phmap(30, 40, 10, 20);
    EXPECT_FALSE(bool(are_equal(*m1, *m2)));
    EXPECT_FALSE(bool(are_equal(*m2, *m1)));
}

TEST_F(equality_test, should_compare_array_sets)
{
    Root s1, s2;
    s1 = aset();
    s2 = aset();
    EXPECT_TRUE(bool(are_equal(*s1, *s2)));

    s1 = aset(3, 4);
    s2 = aset();
    EXPECT_FALSE(bool(are_equal(*s1, *s2)));
    EXPECT_FALSE(bool(are_equal(*s2, *s1)));

    s1 = aset(10, 20, 30, 40, 50, 60);
    s2 = aset(30, 40, 50, 60, 10, 20);
    EXPECT_TRUE(bool(are_equal(*s1, *s2)));
    EXPECT_TRUE(bool(are_equal(*s2, *s1)));

    s1 = aset(10, 99, 30, 40, 50, 60);
    s2 = aset(30, 40, 50, 60, 10, 20);
    EXPECT_FALSE(bool(are_equal(*s1, *s2)));
    EXPECT_FALSE(bool(are_equal(*s2, *s1)));

    s1 = aset(10, 20, 30, 40, nil);
    s2 = aset(30, 40, 10, 20);
    EXPECT_FALSE(bool(are_equal(*s1, *s2)));
    EXPECT_FALSE(bool(are_equal(*s2, *s1)));
}

TEST_F(equality_test, should_compare_types)
{
    EXPECT_TRUE(bool(are_equal(*type::Int64, *type::Int64)));
    EXPECT_FALSE(bool(are_equal(*type::Int64, *type::Seqable)));
    EXPECT_FALSE(bool(are_equal(*type::Seqable, *type::Int64)));
    EXPECT_FALSE(bool(are_equal(*type::Seqable, create_symbol("cleo.core", "Seqable"))));
    EXPECT_FALSE(bool(are_equal(create_symbol("cleo.core", "Seqable"), *type::Seqable)));
    EXPECT_FALSE(bool(are_equal(*type::Int64, *type::MetaType)));
}

}
}
