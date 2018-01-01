#include <cleo/fn.hpp>
#include <cleo/eval.hpp>
#include <cleo/var.hpp>
#include <cleo/reader.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct fn_test : Test {};

TEST_F(fn_test, should_eval_the_body)
{
    Root v{svec(5, 6, 7)};
    Root seq{list(SEQ, *v)};
    Root body{list(FIRST, *seq)};
    Root params{svec()};
    Root fn{create_fn(nil, nil, *params, *body)};
    Root call{list(*fn)};
    Root val{eval(*call)};
    ASSERT_TRUE(get_small_vector_elem(*v, 0) == *val);
}

TEST_F(fn_test, should_pass_the_arguments)
{
    auto s = create_symbol("s");
    auto x = create_symbol("x");
    Root seq{list(s, x)};
    Root body{list(FIRST, *seq)};
    Root params{svec(s, x)};
    Root fn{create_fn(nil, nil, *params, *body)};
    Root v{svec(5, 6, 7)};
    Root call{list(*fn, SEQ, *v)};
    Root val{eval(*call)};
    ASSERT_TRUE(get_small_vector_elem(*v, 0) == *val);
}

TEST_F(fn_test, should_use_values_from_the_env)
{
    auto s = create_symbol("s");
    auto x = create_symbol("x");
    Root v{svec(5, 6, 7)};
    Root env{smap(s, lookup(SEQ), x, *v)};
    Root seq{list(s, x)};
    Root body{list(FIRST, *seq)};
    Root params{svec()};
    Root fn{create_fn(*env, nil, *params, *body)};
    Root call{list(*fn)};
    Root val{eval(*call)};
    ASSERT_TRUE(get_small_vector_elem(*v, 0) == *val);
}

TEST_F(fn_test, should_fail_when_arity_cannot_be_matched)
{
    Root fn{create_string("(fn xyz ([] :a) ([x] :b) ([x y] :c))")};
    fn = read(*fn);
    fn = eval(*fn);
    auto k = create_keyword("k");
    Root call{list(*fn, k, k, k)};

    try
    {
        eval(*call);
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ(type::CallError, get_value_type(*e));
    }
}

TEST_F(fn_test, should_dispatch_to_the_right_arity)
{
    Root fn{create_string("(fn xyz ([] :a) ([x] :b) ([x y] :c))")};
    fn = read(*fn);
    fn = eval(*fn);
    Root call{list(*fn)};
    Root val{eval(*call)};
    EXPECT_EQ_VALS(create_keyword("a"), *val);

    auto k = create_keyword("k");
    call = list(*fn, k);
    val = eval(*call);
    EXPECT_EQ_VALS(create_keyword("b"), *val);

    call = list(*fn, k, k);
    val = eval(*call);
    EXPECT_EQ_VALS(create_keyword("c"), *val);
}

TEST_F(fn_test, should_dispatch_to_vararg)
{
    Root fn{create_string("(fn xyz [& a] a)")};
    fn = read(*fn);
    fn = eval(*fn);
    Root call{list(*fn)};
    Root val{eval(*call)};
    Root ex{nil};
    EXPECT_EQ_VALS(*ex, *val);

    auto x = create_keyword("x");
    auto y = create_keyword("y");
    call = list(*fn, x, y);
    val = eval(*call);
    ex = list(x, y);
    EXPECT_EQ_VALS(*ex, *val);

    fn = create_string("(fn xyz ([a b] :a) ([a b & c] (cleo.core/list a b c)))");
    fn = read(*fn);
    fn = eval(*fn);
    call = list(*fn, x, y);
    val = eval(*call);
    ex = create_keyword("a");
    EXPECT_EQ_VALS(*ex, *val);

    fn = create_string("(fn xyz ([a b & c] (cleo.core/list a b c)) ([a b] :a))");
    fn = read(*fn);
    fn = eval(*fn);
    call = list(*fn, x, y);
    val = eval(*call);
    ex = create_keyword("a");
    EXPECT_EQ_VALS(*ex, *val);

    call = list(*fn, y, x, x, y);
    val = eval(*call);
    ex = list(x, y);
    ex = list(y, x, *ex);
    EXPECT_EQ_VALS(*ex, *val);

    fn = create_string("(fn xyz ([a b & c] (cleo.core/list a b c)))");
    fn = read(*fn);
    fn = eval(*fn);
    call = list(*fn, x, y);
    val = eval(*call);
    ex = list(x, y, nil);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(fn_test, should_not_bind_the_ampersand)
{
    Root call{create_string("((fn [a & b] &) 1 2 3)")};
    call = read(*call);
    try
    {
        eval(*call);
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_VALS(type::SymbolNotFound, get_value_type(*e));
    }
}

}
}
