#include <cleo/multimethod.hpp>
#include <cleo/eval.hpp>
#include <cleo/var.hpp>
#include "util.hpp"
#include <gtest/gtest.h>

namespace cleo
{
namespace test
{

TEST(multimethod_test, should_dispatch_to_the_right_method)
{
    auto name = create_symbol("cleo.multimethod.test", "ab");
    auto dispatchFn = create_native_function([](const Value *args, std::uint8_t) { return args[0]; });
    define_multimethod(name, dispatchFn);
    define_method(name, create_int64(100), create_native_function([](const Value *args, std::uint8_t) { return args[1]; }));
    define_method(name, create_int64(200), create_native_function([](const Value *args, std::uint8_t) { return args[2]; }));

    auto val1 = create_int64(77);
    auto val2 = create_int64(88);
    ASSERT_TRUE(val1 == eval(list(name, create_int64(100), val1, val2)));
    ASSERT_TRUE(val2 == eval(list(name, create_int64(200), val1, val2)));
}

TEST(multimethod_test, should_fail_when_a_matching_method_does_not_exist)
{
    auto name = create_symbol("cleo.multimethod.test", "one");
    auto dispatchFn = create_native_function([](const Value *args, std::uint8_t) { return args[0]; });
    define_multimethod(name, dispatchFn);
    define_method(name, create_int64(100), create_native_function([](const Value *, std::uint8_t) { return nil; }));
    try
    {
        eval(list(name, create_int64(200)));
        FAIL() << "expected an exception";
    }
    catch (const illegal_argument& )
    {
    }
}

struct hierarchy_test : testing::Test
{
    Value a = create_keyword("cleo.multimethod.test", "a");
    Value b = create_keyword("cleo.multimethod.test", "b");
    Value c = create_keyword("cleo.multimethod.test", "c");
    Value d = create_keyword("cleo.multimethod.test", "d");
    Value e = create_keyword("cleo.multimethod.test", "e");
    Value f = create_keyword("cleo.multimethod.test", "f");
    Value g = create_keyword("cleo.multimethod.test", "g");

    Value c1 = create_keyword("cleo.multimethod.test", "c1");
    Value c2 = create_keyword("cleo.multimethod.test", "c2");
    Value c3 = create_keyword("cleo.multimethod.test", "c3");
    Value p1 = create_keyword("cleo.multimethod.test", "p1");
    Value p2 = create_keyword("cleo.multimethod.test", "p2");
    Value p3 = create_keyword("cleo.multimethod.test", "p3");
    Value gp1 = create_keyword("cleo.multimethod.test", "gp1");
};

TEST_F(hierarchy_test, isa_should_be_true_for_equal_values)
{
    ASSERT_NE(nil, isa(a, a));
    ASSERT_NE(nil, isa(create_int64(10), create_int64(10)));
}

TEST_F(hierarchy_test, isa_should_be_true_for_all_ancestors)
{
    derive(create_int64(34567243), create_int64(873456346));
    EXPECT_NE(nil, isa(create_int64(34567243), create_int64(873456346)));

    derive(b, d);

    EXPECT_NE(nil, isa(b, d));

    EXPECT_EQ(nil, isa(d, b));

    derive(b, c);

    EXPECT_NE(nil, isa(b, d));
    EXPECT_NE(nil, isa(b, c));

    EXPECT_EQ(nil, isa(d, b));
    EXPECT_EQ(nil, isa(c, b));
    EXPECT_EQ(nil, isa(c, d));
    EXPECT_EQ(nil, isa(d, c));

    derive(a, b);

    EXPECT_NE(nil, isa(b, d));
    EXPECT_NE(nil, isa(b, c));
    EXPECT_NE(nil, isa(a, d));
    EXPECT_NE(nil, isa(a, c));
    EXPECT_NE(nil, isa(a, b));

    EXPECT_EQ(nil, isa(d, b));
    EXPECT_EQ(nil, isa(c, b));
    EXPECT_EQ(nil, isa(c, d));
    EXPECT_EQ(nil, isa(d, c));
    EXPECT_EQ(nil, isa(c, a));
    EXPECT_EQ(nil, isa(d, a));
    EXPECT_EQ(nil, isa(b, a));

    derive(e, f);
    derive(e, g);
    derive(c, e);

    EXPECT_NE(nil, isa(a, c));
    EXPECT_NE(nil, isa(a, e));
    EXPECT_NE(nil, isa(a, f));
    EXPECT_NE(nil, isa(a, g));
    EXPECT_NE(nil, isa(b, c));
    EXPECT_NE(nil, isa(b, e));
    EXPECT_NE(nil, isa(b, f));
    EXPECT_NE(nil, isa(b, g));

    EXPECT_EQ(nil, isa(d, e));
    EXPECT_EQ(nil, isa(d, f));
    EXPECT_EQ(nil, isa(d, g));
    EXPECT_EQ(nil, isa(c, b));
    EXPECT_EQ(nil, isa(c, d));
    EXPECT_EQ(nil, isa(d, c));
    EXPECT_EQ(nil, isa(c, a));
    EXPECT_EQ(nil, isa(d, a));
    EXPECT_EQ(nil, isa(b, a));
}

TEST_F(hierarchy_test, isa_should_treat_small_vectors_as_tuples)
{
    derive(c1, p1);
    derive(c2, p2);
    derive(c3, p3);
    derive(p1, gp1);

    EXPECT_NE(nil, isa(svec(), svec()));
    EXPECT_EQ(nil, isa(b, svec()));
    EXPECT_EQ(nil, isa(svec(), b));

    EXPECT_NE(nil, isa(svec(c1, p2), svec(c1, p2)));
    EXPECT_EQ(nil, isa(svec(c1), svec(c1, p2)));
    EXPECT_EQ(nil, isa(svec(c1, c2), svec(c3, c2)));
    EXPECT_EQ(nil, isa(svec(c1, c2), svec(c1, c3)));
    EXPECT_EQ(nil, isa(svec(c1, c2), svec(c2, c1)));

    EXPECT_NE(nil, isa(svec(c1, c2), svec(gp1, p2)));
    EXPECT_EQ(nil, isa(svec(c1, c2), svec(p3, p2)));
    EXPECT_EQ(nil, isa(svec(c1, c2), svec(gp1, p3)));
}

}
}
