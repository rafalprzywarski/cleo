#include <cleo/fn.hpp>
#include <cleo/eval.hpp>
#include <cleo/var.hpp>
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

}
}
