#include <cleo/multimethod.hpp>
#include <cleo/eval.hpp>
#include <cleo/var.hpp>
#include <cleo/global.hpp>
#include <cleo/error.hpp>
#include "util.hpp"
#include <gtest/gtest.h>

namespace cleo
{
namespace test
{

struct multimethod_test : Test
{
    multimethod_test() : Test("cleo.multimethod.test") {}

    static auto mk_fn()
    {
        return create_native_function([](const Value *args, std::uint8_t) { return force(nil); });
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
    Root dfn, fn1, fn2, fnnil, val1, val2, val3;
    dfn = mk_fn();
    fn1 = mk_fn();
    fn2 = mk_fn();
    fnnil = mk_fn();
    val1 = create_int64(100);
    val2 = create_int64(200);
    val3 = create_int64(300);
    auto default_ = keyword("default");

    Value multi = define_multimethod(name, *dfn, default_);
    define_method(name, *val1, *fn1);
    define_method(name, *val2, *fn2);
    define_method(name, nil, *fnnil);

    ASSERT_TRUE(fn1->is(get_method(get_var_value(multi), *val1)));
    ASSERT_TRUE(fn2->is(get_method(get_var_value(multi), *val2)));
    ASSERT_TRUE(fnnil->is(get_method(get_var_value(multi), nil)));
    ASSERT_TRUE(get_method(get_var_value(multi), *val3).is_nil());
}

TEST_F(multimethod_test, get_method_should_provide_the_default_method_if_its_defined_and_no_methods_are_matched)
{
    auto name = symbol("with-default");
    Root dfn, fn1, fn2, fnnil, val1, val2;
    dfn = mk_fn();
    fn1 = mk_fn();
    fn2 = mk_fn();
    fnnil = mk_fn();
    val1 = create_int64(100);
    val2 = create_int64(300);
    auto default_ = keyword("default");

    Value multi = define_multimethod(name, *dfn, default_);
    define_method(name, *val1, *fn1);
    define_method(name, nil, *fnnil);

    ASSERT_TRUE(get_method(get_var_value(multi), *val2).is_nil());

    define_method(name, default_, *fn2);

    ASSERT_TRUE(fn1->is(get_method(get_var_value(multi), *val1)));
    ASSERT_TRUE(fn2->is(get_method(get_var_value(multi), *val2)));
    ASSERT_TRUE(fnnil->is(get_method(get_var_value(multi), nil)));
}

TEST_F(multimethod_test, get_method_should_follow_ancestors_to_find_matches)
{
    auto name = symbol("hierarchies");
    auto ha = keyword("ha");
    auto hb = keyword("hb");
    auto hc = keyword("hc");
    Root dfn, fn1, fn2, fn3;
    dfn = mk_fn();
    fn1 = mk_fn();
    fn2 = mk_fn();
    fn3 = mk_fn();

    derive(hb, ha);
    derive(hc, hb);

    Value multi = define_multimethod(name, *dfn, nil);
    define_method(name, ha, *fn1);

    EXPECT_TRUE(fn1->is(get_method(get_var_value(multi), ha)));
    EXPECT_TRUE(fn1->is(get_method(get_var_value(multi), hb)));
    EXPECT_TRUE(fn1->is(get_method(get_var_value(multi), hc)));

    define_method(name, hb, *fn2);

    EXPECT_TRUE(fn1->is(get_method(get_var_value(multi), ha)));
    EXPECT_TRUE(fn2->is(get_method(get_var_value(multi), hb)));
    EXPECT_TRUE(fn2->is(get_method(get_var_value(multi), hc)));

    define_method(name, hc, *fn3);

    EXPECT_TRUE(fn1->is(get_method(get_var_value(multi), ha)));
    EXPECT_TRUE(fn2->is(get_method(get_var_value(multi), hb)));
    EXPECT_TRUE(fn3->is(get_method(get_var_value(multi), hc)));
}

TEST_F(multimethod_test, get_method_should_fail_when_multiple_methods_match_a_dispatch_value)
{
    auto name = symbol("bad");
    auto a = keyword("bad_a");
    auto b = keyword("bad_b");
    auto c = keyword("bad_c");
    Root dfn, fn1, fn2;
    dfn = mk_fn();
    fn1 = mk_fn();
    fn2 = mk_fn();

    derive(c, a);

    Value multi = define_multimethod(name, *dfn, nil);
    define_method(name, a, *fn1);
    define_method(name, b, *fn2);

    ASSERT_EQ_REFS(*fn1, get_method(get_var_value(multi), c));

    derive(c, b);

    try
    {
        get_method(get_var_value(multi), c);
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_REFS(*type::IllegalArgument, get_value_type(*e));
    }
}

TEST_F(multimethod_test, should_dispatch_to_the_right_method)
{
    auto name = symbol("ab");
    auto parent = keyword("dparent");
    auto child1 = keyword("dchild1");
    auto child2 = keyword("dchild2");
    Root dispatchFn, ret2nd, ret3rd;
    dispatchFn = create_native_function([](const Value *args, std::uint8_t) { return force(args[0]); });
    ret2nd = create_native_function([](const Value *args, std::uint8_t) { return force(args[1]); });
    ret3rd = create_native_function([](const Value *args, std::uint8_t) { return force(args[2]); });

    derive(child1, parent);
    derive(child2, parent);

    define_multimethod(name, *dispatchFn, nil);
    define_method(name, parent, *ret2nd);
    define_method(name, child2, *ret3rd);

    Root val1, val2, l;
    val1 = create_int64(77);
    val2 = create_int64(88);
    l = list(name, child1, *val1, *val2);
    ASSERT_TRUE(val1->is(*Root(eval(*l))));
    l = list(name, child2, *val1, *val2);
    ASSERT_TRUE(val2->is(*Root(eval(*l))));
}

TEST_F(multimethod_test, should_fail_when_a_matching_method_does_not_exist)
{
    auto name = symbol("one");
    Root dispatchFn, ret_nil, num;
    dispatchFn = create_native_function([](const Value *args, std::uint8_t) { return force(args[0]); });
    ret_nil = create_native_function([](const Value *, std::uint8_t) { return force(nil); });
    num = create_int64(100);
    define_multimethod(name, *dispatchFn, nil);
    define_method(name, *num, *ret_nil);
    Root val, l;
    val = create_int64(200);
    l = list(name, *val);
    try
    {
        eval(*l);
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_REFS(*type::IllegalArgument, get_value_type(*e));
    }
}

TEST_F(multimethod_test, get_method_should_check_for_ambiguity_after_selecting_the_best_match)
{
    for (int i = 0; i < 64; ++i)
    {
        auto name = symbol("hierarchies" + std::to_string(i));
        auto ggchild = keyword("ggchild" + std::to_string(i));
        auto gchild = keyword("gchild" + std::to_string(i));
        auto lchild = keyword("lchild" + std::to_string(i));
        auto rchild = keyword("rchild" + std::to_string(i));
        auto parent = keyword("parent" + std::to_string(i));
        Root dfn, fn1, fn2, fn3, fn4;
        dfn = mk_fn();
        fn1 = mk_fn();
        fn2 = mk_fn();
        fn3 = mk_fn();
        fn4 = mk_fn();

        derive(lchild, parent);
        derive(rchild, parent);
        derive(gchild, lchild);
        derive(gchild, rchild);
        derive(ggchild, gchild);

        Value multi = define_multimethod(name, *dfn, nil);
        define_method(name, parent, *fn1);
        define_method(name, lchild, *fn2);
        define_method(name, rchild, *fn3);
        define_method(name, gchild, *fn4);

        EXPECT_TRUE(fn1->is(get_method(get_var_value(multi), parent)));
        EXPECT_TRUE(fn2->is(get_method(get_var_value(multi), lchild)));
        EXPECT_TRUE(fn3->is(get_method(get_var_value(multi), rchild)));
        EXPECT_TRUE(fn4->is(get_method(get_var_value(multi), gchild)));
        EXPECT_TRUE(fn4->is(get_method(get_var_value(multi), ggchild)));
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

    void assert_derive_fails(const std::string& msg,  Value type, Value parent)
    {
        try
        {
            derive(type, parent);
            FAIL() << "expected an exception for " << to_string(type) << " and " << to_string(parent) << " with message: " << msg;
        }
        catch (const Exception& )
        {
            Root e{catch_exception()};
            ASSERT_EQ_REFS(*type::IllegalArgument, get_value_type(*e));
            Root emsg{illegal_argument_message(*e)};
            ASSERT_EQ_REFS(*type::UTF8String, get_value_type(*emsg));
            ASSERT_EQ(msg, std::string(get_string_ptr(*emsg), get_string_len(*emsg)));
        }
    }
};

TEST_F(hierarchy_test, isa_should_be_true_for_equal_values)
{
    ASSERT_TRUE(bool(isa(a, a)));
    Root val1, val2;
    val1 = create_int64(10);
    val2 = create_int64(10);
    ASSERT_TRUE(bool(isa(*val1, *val2)));
}

TEST_F(hierarchy_test, isa_should_be_true_for_all_ancestors)
{
    Root val1, val2;
    val1 = create_int64(34567243);
    val2 = create_int64(873456346);
    derive(*val1, *val2);
    val1 = create_int64(34567243);
    val2 = create_int64(873456346);
    EXPECT_TRUE(bool(isa(*val1, *val2)));

    derive(b, d);

    EXPECT_TRUE(bool(isa(b, d)));

    EXPECT_FALSE(bool(isa(d, b)));

    derive(b, c);

    EXPECT_TRUE(bool(isa(b, d)));
    EXPECT_TRUE(bool(isa(b, c)));

    EXPECT_FALSE(bool(isa(d, b)));
    EXPECT_FALSE(bool(isa(c, b)));
    EXPECT_FALSE(bool(isa(c, d)));
    EXPECT_FALSE(bool(isa(d, c)));

    derive(a, b);

    EXPECT_TRUE(bool(isa(b, d)));
    EXPECT_TRUE(bool(isa(b, c)));
    EXPECT_TRUE(bool(isa(a, d)));
    EXPECT_TRUE(bool(isa(a, c)));
    EXPECT_TRUE(bool(isa(a, b)));

    EXPECT_FALSE(bool(isa(d, b)));
    EXPECT_FALSE(bool(isa(c, b)));
    EXPECT_FALSE(bool(isa(c, d)));
    EXPECT_FALSE(bool(isa(d, c)));
    EXPECT_FALSE(bool(isa(c, a)));
    EXPECT_FALSE(bool(isa(d, a)));
    EXPECT_FALSE(bool(isa(b, a)));

    derive(e, f);
    derive(e, g);
    derive(c, e);

    EXPECT_TRUE(bool(isa(a, c)));
    EXPECT_TRUE(bool(isa(a, e)));
    EXPECT_TRUE(bool(isa(a, f)));
    EXPECT_TRUE(bool(isa(a, g)));
    EXPECT_TRUE(bool(isa(b, c)));
    EXPECT_TRUE(bool(isa(b, e)));
    EXPECT_TRUE(bool(isa(b, f)));
    EXPECT_TRUE(bool(isa(b, g)));

    EXPECT_FALSE(bool(isa(d, e)));
    EXPECT_FALSE(bool(isa(d, f)));
    EXPECT_FALSE(bool(isa(d, g)));
    EXPECT_FALSE(bool(isa(c, b)));
    EXPECT_FALSE(bool(isa(c, d)));
    EXPECT_FALSE(bool(isa(d, c)));
    EXPECT_FALSE(bool(isa(c, a)));
    EXPECT_FALSE(bool(isa(d, a)));
    EXPECT_FALSE(bool(isa(b, a)));
}

TEST_F(hierarchy_test, isa_should_treat_arrays_as_tuples)
{
    derive(c1, p1);
    derive(c2, p2);
    derive(c3, p3);
    derive(p1, gp1);

    Root val1, val2;
    val1 = array();
    val2 = array();
    EXPECT_TRUE(bool(isa(*val1, *val2)));
    val1 = b;
    EXPECT_FALSE(bool(isa(*val1, *val2)));
    EXPECT_FALSE(bool(isa(*val2, *val1)));

    val1 = array(c1, p2);
    val2 = array(c1, p2);
    EXPECT_TRUE(bool(isa(*val1, *val2)));
    val1 = array(c1);
    EXPECT_FALSE(bool(isa(*val1, *val2)));
    val1 = array(c1, c2);
    val2 = array(c3, c2);
    EXPECT_FALSE(bool(isa(*val1, *val2)));
    val2 = array(c1, c3);
    EXPECT_FALSE(bool(isa(*val1, *val2)));
    val2 = array(c2, c1);
    EXPECT_FALSE(bool(isa(*val1, *val2)));

    val1 = array(c1, c2);
    val2 = array(gp1, p2);
    EXPECT_TRUE(bool(isa(*val1, *val2)));
    val2 = array(p3, p2);
    EXPECT_FALSE(bool(isa(*val1, *val2)));
    val2 = array(gp1, p3);
    EXPECT_FALSE(bool(isa(*val1, *val2)));
}

TEST_F(hierarchy_test, derive_should_fail_when_one_of_the_types_is_dynamic)
{
    std::array<Value, 3> abc_names{{create_symbol("a"), create_symbol("b"), create_symbol("c")}};
    std::array<Value, 3> abd_names{{create_symbol("a"), create_symbol("b"), create_symbol("d")}};
    std::array<Value, 3> abc_types{{type::Int64, *type::UTF8String, nil}};
    std::array<Value, 3> abc_types2{{type::Int64, *type::UTF8String, *type::Float64}};
    std::array<Value, 3> ab_types2{{type::Int64, *type::Float64}};
    Root EmptyStatic{create_static_object_type("hierarchy.test", "EmptyStatic", nullptr, nullptr, 0)};
    Root EmptyStatic2{create_static_object_type("hierarchy.test", "EmptyStatic2", nullptr, nullptr, 0)};
    Root AB{create_static_object_type("hierarchy.test", "AB", abc_names.data(), abc_types.data(), 2)};
    Root AB2{create_static_object_type("hierarchy.test", "AB2", abc_names.data(), abc_types.data(), 2)};
    Root ABC{create_static_object_type("hierarchy.test", "ABC", abc_names.data(), abc_types.data(), abc_names.size())};
    Root ABC2{create_static_object_type("hierarchy.test", "ABC2", abc_names.data(), abc_types.data(), abc_names.size())};
    Root ABC3{create_static_object_type("hierarchy.test", "ABC3", abc_names.data(), abc_types.data(), abc_names.size())};
    Root ABC_diff_type{create_static_object_type("hierarchy.test", "ABC-diff-type", abc_names.data(), abc_types2.data(), abc_names.size())};
    Root AB_diff_type{create_static_object_type("hierarchy.test", "AB-diff-type", abc_names.data(), ab_types2.data(), ab_types2.size())};
    Root ABD{create_static_object_type("hierarchy.test", "ABD", abd_names.data(), abc_types.data(), abd_names.size())};
    Root Dynamic{create_dynamic_object_type("hierarchy.test", "Dynamic")};
    Root Dynamic2{create_dynamic_object_type("hierarchy.test", "Dynamic2")};
    Value keyword1 = create_keyword("hierarchy.test", "keyword1");
    Value keyword2 = create_keyword("hierarchy.test", "keyword2");

    ASSERT_NO_THROW(derive(*ABC, *AB));
    ASSERT_NO_THROW(derive(*AB, *EmptyStatic));
    ASSERT_NO_THROW(derive(*ABC, keyword1));

    assert_derive_fails("Can't derive a non-type: :hierarchy.test/keyword2 from a type: hierarchy.test/ABC2", keyword2, *ABC2);

    assert_derive_fails("Can't derive from itself: hierarchy.test/ABC2", *ABC2, *ABC2);

    assert_derive_fails("Can't derive an ancestor: hierarchy.test/AB from a derived type: hierarchy.test/ABC", *AB, *ABC);

    assert_derive_fails("Parent type: hierarchy.test/AB-diff-type is incompatible with: hierarchy.test/ABC2", *ABC2, *AB_diff_type);
    assert_derive_fails("Parent type: hierarchy.test/ABC-diff-type is incompatible with: hierarchy.test/ABC2", *ABC2, *ABC_diff_type);
    assert_derive_fails("Parent type: hierarchy.test/ABD is incompatible with: hierarchy.test/ABC2", *ABC2, *ABD);
    assert_derive_fails("Parent type: hierarchy.test/ABC2 is incompatible with: hierarchy.test/ABD", *ABD, *ABC2);
    assert_derive_fails("Parent type: hierarchy.test/ABC2 is incompatible with: hierarchy.test/AB2", *AB2, *ABC2);
    assert_derive_fails("Parent type: hierarchy.test/ABC2 is incompatible with: hierarchy.test/Dynamic", *Dynamic, *ABC2);

    assert_derive_fails("Can't derive a static type: hierarchy.test/ABC2 from a dynamic type: hierarchy.test/Dynamic", *ABC2, *Dynamic);

    ASSERT_NO_THROW(derive(*Dynamic, *EmptyStatic2));
    ASSERT_NO_THROW(derive(*ABC2, *ABC3));
    ASSERT_NO_THROW(derive(*Dynamic2, *Dynamic));
}

}
}
