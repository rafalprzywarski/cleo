#include <cleo/eval.hpp>
#include <cleo/var.hpp>
#include <cleo/reader.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct fn_test : Test
{
    fn_test() : Test("cleo.fn.test") { }
};

TEST_F(fn_test, should_fail_when_arity_cannot_be_matched)
{
    Root fn{create_string("(fn* xyz ([] :a) ([x] :b) ([x y] :c))")};
    fn = read(*fn);
    fn = eval(*fn);
    auto k = create_keyword("k");
    Root call{list(*fn, k, k, k)};

    try
    {
        eval(*call);
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_REFS(*type::CallError, get_value_type(*e));
    }
}

TEST_F(fn_test, should_dispatch_to_the_right_arity)
{
    Root fn{create_string("(fn* xyz ([] :a) ([x] :b) ([x y] :c))")};
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
    Root fn{create_string("(fn* xyz [& a] a)")};
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
    ex = array(x, y);
    EXPECT_EQ_VALS(*ex, *val);

    fn = create_string("(fn* xyz ([a b] :a) ([a b & c] [a b c]))");
    fn = read(*fn);
    fn = eval(*fn);
    call = list(*fn, x, y);
    val = eval(*call);
    ex = create_keyword("a");
    EXPECT_EQ_VALS(*ex, *val);

    fn = create_string("(fn* xyz ([a b & c] [a b c]) ([a b] :a))");
    fn = read(*fn);
    fn = eval(*fn);
    call = list(*fn, x, y);
    val = eval(*call);
    ex = create_keyword("a");
    EXPECT_EQ_VALS(*ex, *val);

    call = list(*fn, y, x, x, y);
    val = eval(*call);
    ex = list(x, y);
    ex = array(y, x, *ex);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_EQ_REFS(*type::ArraySeq, get_value_type(get_array_elem(*val, 2)));

    fn = create_string("(fn* xyz ([a b & c] [a b c]))");
    fn = read(*fn);
    fn = eval(*fn);
    call = list(*fn, x, y);
    val = eval(*call);
    ex = array(x, y, nil);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(fn_test, should_not_bind_the_ampersand)
{
    Root call{create_string("((fn* [a & b] &) 1 2 3)")};
    call = read(*call);
    try
    {
        eval(*call);
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_VALS(*type::CompilationError, get_value_type(*e));
    }
}

TEST_F(fn_test, should_correctly_resolve_symbols_from_fns_namespace)
{
    in_ns(create_symbol("cleo.fn.resolve.test1"));
    define(create_symbol("cleo.fn.resolve.test1", "x"), create_keyword("ok"));
    Root fn{create_string("(fn* [] [x `x `y])")};
    fn = read(*fn);
    fn = eval(*fn);
    in_ns(create_symbol("cleo.fn.resolve.test2"));
    Root val{list(*fn)}, ex;
    val = eval(*val);
    ex = array(create_keyword("ok"), create_symbol("cleo.fn.resolve.test1", "x"), create_symbol("cleo.fn.resolve.test1", "y"));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(fn_test, should_correctly_resolve_symbols_from_fns_created_in_fns_from_other_namespaces)
{
    in_ns(create_symbol("cleo.fn.resolve.test1"));
    define(create_symbol("cleo.fn.resolve.test1", "x"), create_keyword("ok"));
    Root fn{create_string("(fn* [] (fn* [] [x `x `y]))")};
    fn = read(*fn);
    fn = eval(*fn);
    in_ns(create_symbol("cleo.fn.resolve.test2"));
    Root val{list(*fn)}, ex;
    val = list(*val);
    val = eval(*val);
    ex = array(create_keyword("ok"), create_symbol("cleo.fn.resolve.test1", "x"), create_symbol("cleo.fn.resolve.test1", "y"));
    EXPECT_EQ_VALS(*ex, *val);
}

}
}
