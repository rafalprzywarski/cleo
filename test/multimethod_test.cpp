#include <cleo/multimethod.hpp>
#include <cleo/eval.hpp>
#include <cleo/var.hpp>
#include "util.hpp"
#include <gtest/gtest.h>

namespace cleo
{
namespace test
{

struct multimethod_test : testing::Test
{
    static auto mk_fn()
    {
        return create_native_function([](const Value *args, std::uint8_t) { return nil; });
    }

    static auto symbol(const std::string& name)
    {
        return create_symbol("cleo.multimethod.test", name);
    }

    static auto keyword(const std::string& name)
    {
        return create_keyword("cleo.multimethod.test", name);
    }
};

TEST_F(multimethod_test, get_method_should_provide_the_method_for_a_dispatch_value_or_nil)
{
    auto name = symbol("simple");
    auto fn1 = mk_fn();
    auto fn2 = mk_fn();

    Value multi = define_multimethod(name, mk_fn(), nil);
    define_method(name, create_int64(100), fn1);
    define_method(name, create_int64(200), fn2);

    ASSERT_EQ(fn1, get_method(multi, create_int64(100)));
    ASSERT_EQ(fn2, get_method(multi, create_int64(200)));
    ASSERT_EQ(nil, get_method(multi, create_int64(300)));
}

TEST_F(multimethod_test, get_method_should_provide_the_default_method_if_its_defined_and_no_methods_are_matched)
{
    auto name = symbol("with-default");
    auto fn1 = mk_fn();
    auto fn2 = mk_fn();
    auto default_ = keyword("default");

    Value multi = define_multimethod(name, mk_fn(), default_);
    define_method(name, create_int64(100), fn1);

    ASSERT_EQ(nil, get_method(multi, create_int64(300)));

    define_method(name, default_, fn2);

    ASSERT_EQ(fn1, get_method(multi, create_int64(100)));
    ASSERT_EQ(fn2, get_method(multi, create_int64(300)));
}

TEST_F(multimethod_test, get_method_should_follow_ancestors_to_find_matches)
{
    auto name = symbol("hierarchies");
    auto ha = keyword("ha");
    auto hb = keyword("hb");
    auto hc = keyword("hc");
    auto fn1 = mk_fn();
    auto fn2 = mk_fn();
    auto fn3 = mk_fn();

    derive(hb, ha);
    derive(hc, hb);

    Value multi = define_multimethod(name, mk_fn(), nil);
    define_method(name, ha, fn1);

    EXPECT_EQ(fn1, get_method(multi, ha));
    EXPECT_EQ(fn1, get_method(multi, hb));
    EXPECT_EQ(fn1, get_method(multi, hc));

    define_method(name, hb, fn2);

    EXPECT_EQ(fn1, get_method(multi, ha));
    EXPECT_EQ(fn2, get_method(multi, hb));
    EXPECT_EQ(fn2, get_method(multi, hc));

    define_method(name, hc, fn3);

    EXPECT_EQ(fn1, get_method(multi, ha));
    EXPECT_EQ(fn2, get_method(multi, hb));
    EXPECT_EQ(fn3, get_method(multi, hc));
}

TEST_F(multimethod_test, get_method_should_fail_when_multiple_methods_match_a_dispatch_value)
{
    auto name = symbol("bad");
    auto a = keyword("bad_a");
    auto b = keyword("bad_b");
    auto c = keyword("bad_c");
    auto fn1 = mk_fn();
    auto fn2 = mk_fn();

    derive(c, a);
    derive(c, b);

    Value multi = define_multimethod(name, mk_fn(), nil);
    define_method(name, a, fn1);
    define_method(name, b, fn2);

    EXPECT_THROW(get_method(multi, c), illegal_argument);
}

TEST_F(multimethod_test, should_dispatch_to_the_right_method)
{
    auto name = symbol("ab");
    auto parent = keyword("dparent");
    auto child1 = keyword("dchild1");
    auto child2 = keyword("dchild2");
    auto dispatchFn = create_native_function([](const Value *args, std::uint8_t) { return args[0]; });

    derive(child1, parent);
    derive(child2, parent);

    define_multimethod(name, dispatchFn, nil);
    define_method(name, parent, create_native_function([](const Value *args, std::uint8_t) { return args[1]; }));
    define_method(name, child2, create_native_function([](const Value *args, std::uint8_t) { return args[2]; }));

    auto val1 = create_int64(77);
    auto val2 = create_int64(88);
    ASSERT_TRUE(val1 == eval(list(name, child1, val1, val2)));
    ASSERT_TRUE(val2 == eval(list(name, child2, val1, val2)));
}

TEST_F(multimethod_test, should_fail_when_a_matching_method_does_not_exist)
{
    auto name = symbol("one");
    auto dispatchFn = create_native_function([](const Value *args, std::uint8_t) { return args[0]; });
    define_multimethod(name, dispatchFn, nil);
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

struct hierarchy_test : multimethod_test
{
    Value a = keyword("a");
    Value b = keyword("b");
    Value c = keyword("c");
    Value d = keyword("d");
    Value e = keyword("e");
    Value f = keyword("f");
    Value g = keyword("g");

    Value c1 = keyword("c1");
    Value c2 = keyword("c2");
    Value c3 = keyword("c3");
    Value p1 = keyword("p1");
    Value p2 = keyword("p2");
    Value p3 = keyword("p3");
    Value gp1 = keyword("gp1");
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
