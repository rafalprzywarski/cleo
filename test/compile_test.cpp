#include <cleo/compile.hpp>
#include "util.hpp"
#include <cleo/eval.hpp>
#include <cleo/reader.hpp>
#include <cleo/global.hpp>
#include <cleo/bytecode_fn.hpp>
#include <gmock/gmock.h>

namespace cleo
{
namespace test
{

using testing::AnyOf;

struct compile_test : Test
{
    Value a_var;
    compile_test() : Test("cleo.compile.test")
    {
        a_var = define(create_symbol("cleo.compile.test", "a-var"), nil);
    }
    Force read_str(const std::string& s)
    {
        Root ss{create_string(s)};
        return read(*ss);
    }

    std::vector<vm::Byte> bc(Value body)
    {
        return std::vector<vm::Byte>(get_bytecode_fn_body_bytes(body), get_bytecode_fn_body_bytes(body) + get_bytecode_fn_body_bytes_size(body));;
    }

    template <typename... Ts>
    std::vector<vm::Byte> b(Ts... bytes) { return {static_cast<vm::Byte>(bytes)...}; }

    void expect_compilation_error(std::string form_str, std::string msg = {})
    {
        Root form{read_str(form_str)};
        try
        {
            cleo::compile_fn(*form, nil);
            FAIL() << "expected compilation failure for: " << form_str;
        }
        catch (Exception const& )
        {
            Root e{catch_exception()};
            ASSERT_EQ_REFS(*type::CompilationError, get_value_type(*e));
            if (!msg.empty())
            {
                Root expected_msg{create_string(msg)};
                Root actual_msg{compilation_error_message(*e)};
                ASSERT_EQ_VALS(*expected_msg, *actual_msg);
            }
        }
    }

    template <typename Env>
    Force compile_fn(const std::string& s, Env envv)
    {
        Root env{to_value(envv)};
        Root form{read_str(s)};
        return cleo::compile_fn(*form, *env);
    }

    Force compile_fn(const std::string& s)
    {
        return compile_fn(s, nil);
    }

    void expect_fn_with_arities(Value fn, std::vector<Int64> arities)
    {
        ASSERT_EQ_VALS(*type::BytecodeFn, get_value_type(fn));
        ASSERT_EQ(arities.size(), get_bytecode_fn_size(fn));
        for (std::uint8_t i = 0; i < get_bytecode_fn_size(fn); ++i)
            EXPECT_EQ(arities[i], get_bytecode_fn_arity(fn, i)) << "index: " << unsigned(i);;
    }

    void expect_body_with_bytecode(Value fn, std::uint8_t index, std::vector<vm::Byte> code)
    {
        auto body = get_bytecode_fn_body(fn, index);
        ASSERT_EQ_REFS(*type::BytecodeFnBody, get_value_type(body));
        EXPECT_EQ_VALS(nil, get_bytecode_fn_body_consts(body));
        EXPECT_EQ_VALS(nil, get_bytecode_fn_body_vars(body));
        EXPECT_EQ(0u, get_bytecode_fn_body_locals_size(body));
        EXPECT_EQ(code, bc(body));
    }

    template <typename Consts>
    void expect_body_with_consts_and_bytecode(Value fn, std::uint8_t index, Consts constsv, std::vector<vm::Byte> code)
    {
        auto body = get_bytecode_fn_body(fn, index);
        ASSERT_EQ_REFS(*type::BytecodeFnBody, get_value_type(body));
        Root consts{to_value(constsv)};
        EXPECT_EQ_VALS(*consts, get_bytecode_fn_body_consts(body));
        EXPECT_EQ_REFS(*type::Array, get_value_type(get_bytecode_fn_body_consts(body)));
        EXPECT_EQ_VALS(nil, get_bytecode_fn_body_vars(body));
        EXPECT_EQ(0u, get_bytecode_fn_body_locals_size(body));
        EXPECT_EQ(code, bc(body));
    }

    template <typename Consts>
    void expect_body_with_locals_consts_and_bytecode(Value fn, std::uint8_t index, std::uint32_t locals_size, Consts constsv, std::vector<vm::Byte> code)
    {
        auto body = get_bytecode_fn_body(fn, index);
        ASSERT_EQ_REFS(*type::BytecodeFnBody, get_value_type(body));
        Root consts{to_value(constsv)};
        EXPECT_EQ_VALS(*consts, get_bytecode_fn_body_consts(body));
        EXPECT_EQ_REFS(*type::Array, get_value_type(get_bytecode_fn_body_consts(body)));
        EXPECT_EQ_VALS(nil, get_bytecode_fn_body_vars(body));
        EXPECT_EQ(locals_size, get_bytecode_fn_body_locals_size(body));
        EXPECT_EQ(code, bc(body));
    }

    Value get_fn_const(Value fn, std::uint8_t index, Int64 cindex)
    {
        auto consts = get_bytecode_fn_body_consts(get_bytecode_fn_body(fn, index));
        if (!get_value_type(consts).is(*type::Array))
            return nil;
        return get_array_elem(consts, cindex);
    }

    template <typename Vars>
    void expect_body_with_vars_and_bytecode(Value fn, std::uint8_t index, Vars varsv, std::vector<vm::Byte> code)
    {
        auto body = get_bytecode_fn_body(fn, index);
        ASSERT_EQ_REFS(*type::BytecodeFnBody, get_value_type(body));
        EXPECT_EQ_VALS(nil, get_bytecode_fn_body_consts(body));
        Root vars{to_value(varsv)};
        EXPECT_EQ_VALS(*vars, get_bytecode_fn_body_vars(body));
        EXPECT_EQ_REFS(*type::Array, get_value_type(get_bytecode_fn_body_vars(body)));
        EXPECT_EQ(0u, get_bytecode_fn_body_locals_size(body));
        EXPECT_EQ(code, bc(body));
    }

    template <typename Consts, typename Vars>
    void expect_body_with_consts_vars_and_bytecode(Value fn, std::uint8_t index, Consts constsv, Vars varsv, std::vector<vm::Byte> code)
    {
        auto body = get_bytecode_fn_body(fn, index);
        ASSERT_EQ_REFS(*type::BytecodeFnBody, get_value_type(body));
        Root consts{to_value(constsv)};
        Root vars{to_value(varsv)};
        EXPECT_EQ_VALS(*consts, get_bytecode_fn_body_consts(body));
        EXPECT_EQ_REFS(*type::Array, get_value_type(get_bytecode_fn_body_consts(body)));
        EXPECT_EQ_VALS(*vars, get_bytecode_fn_body_vars(body));
        EXPECT_EQ_REFS(*type::Array, get_value_type(get_bytecode_fn_body_vars(body)));
        EXPECT_EQ(0u, get_bytecode_fn_body_locals_size(body));
        EXPECT_EQ(code, bc(body));
    }

    template <typename Consts, typename Vars>
    void expect_body_with_locals_consts_vars_and_bytecode(Value fn, std::uint8_t index, std::uint32_t locals_size, Consts constsv, Vars varsv, std::vector<vm::Byte> code)
    {
        auto body = get_bytecode_fn_body(fn, index);
        ASSERT_EQ_REFS(*type::BytecodeFnBody, get_value_type(body));
        Root consts{to_value(constsv)};
        Root vars{to_value(varsv)};
        EXPECT_EQ_VALS(*consts, get_bytecode_fn_body_consts(body));
        EXPECT_EQ_REFS(*type::Array, get_value_type(get_bytecode_fn_body_consts(body)));
        EXPECT_EQ_VALS(*vars, get_bytecode_fn_body_vars(body));
        EXPECT_EQ_REFS(*type::Array, get_value_type(get_bytecode_fn_body_vars(body)));
        EXPECT_EQ(locals_size, get_bytecode_fn_body_locals_size(body));
        EXPECT_EQ(code, bc(body));
    }

    void expect_body_with_locals_and_bytecode(Value fn, std::uint8_t index, std::uint32_t locals_size, std::vector<vm::Byte> code)
    {
        auto body = get_bytecode_fn_body(fn, index);
        ASSERT_EQ_REFS(*type::BytecodeFnBody, get_value_type(body));
        EXPECT_EQ_VALS(nil, get_bytecode_fn_body_consts(body));
        EXPECT_EQ_VALS(nil, get_bytecode_fn_body_vars(body));
        EXPECT_EQ(locals_size, get_bytecode_fn_body_locals_size(body));
        EXPECT_EQ(code, bc(body));
    }

    std::vector<vm::Byte> subvec(const std::vector<vm::Byte>& v, std::size_t start)
    {
        return {v.begin() + start, v.end()};
    }
};

TEST_F(compile_test, should_compile_functions_returning_constants)
{
    Root fn{compile_fn("(fn* [] 27)")};
    ASSERT_NO_FATAL_FAILURE(expect_fn_with_arities(*fn, {0}));
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(27), b(vm::LDC, 0, 0));
}

TEST_F(compile_test, should_compile_functions_with_multiple_arities)
{
    Root fn{compile_fn("(fn*)")};
    expect_fn_with_arities(*fn, {});
    EXPECT_EQ_VALS(nil, get_bytecode_fn_name(*fn));

    fn = compile_fn("(fn* some)");
    expect_fn_with_arities(*fn, {});
    EXPECT_EQ_VALS(create_symbol("some"), get_bytecode_fn_name(*fn));

    fn = compile_fn("(fn* ([] 13))");
    ASSERT_NO_FATAL_FAILURE(expect_fn_with_arities(*fn, {0}));
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(13), b(vm::LDC, 0, 0));
    EXPECT_EQ_VALS(nil, get_bytecode_fn_name(*fn));

    fn = compile_fn("(fn* some ([] 13))");
    ASSERT_NO_FATAL_FAILURE(expect_fn_with_arities(*fn, {0}));
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(13), b(vm::LDC, 0, 0));
    EXPECT_EQ_VALS(create_symbol("some"), get_bytecode_fn_name(*fn));

    fn = compile_fn("(fn* some ([a b c] 13) ([] 14) ([a b] 15))");
    ASSERT_NO_FATAL_FAILURE(expect_fn_with_arities(*fn, {0, 2, 3}));
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(14), b(vm::LDC, 0, 0));
    expect_body_with_consts_and_bytecode(*fn, 1, arrayv(15), b(vm::LDC, 0, 0));
    expect_body_with_consts_and_bytecode(*fn, 2, arrayv(13), b(vm::LDC, 0, 0));
    EXPECT_EQ_VALS(create_symbol("some"), get_bytecode_fn_name(*fn));

    fn = compile_fn("(fn* some ([& a] 11))");
    ASSERT_NO_FATAL_FAILURE(expect_fn_with_arities(*fn, {~Int64(0)}));
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(11), b(vm::LDC, 0, 0));

    fn = compile_fn("(fn* some ([a & b] 11) ([a] 12) ([] 13))");
    ASSERT_NO_FATAL_FAILURE(expect_fn_with_arities(*fn, {0, 1, ~Int64(1)}));
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(13), b(vm::LDC, 0, 0));
    expect_body_with_consts_and_bytecode(*fn, 1, arrayv(12), b(vm::LDC, 0, 0));
    expect_body_with_consts_and_bytecode(*fn, 2, arrayv(11), b(vm::LDC, 0, 0));

    fn = compile_fn("(fn* some ([a b & c] 11) ([a] 12))");
    ASSERT_NO_FATAL_FAILURE(expect_fn_with_arities(*fn, {1, ~Int64(2)}));
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(12), b(vm::LDC, 0, 0));
    expect_body_with_consts_and_bytecode(*fn, 1, arrayv(11), b(vm::LDC, 0, 0));
}

TEST_F(compile_test, should_compile_functions_returning_parameters)
{
    Root fn{compile_fn("(fn* [a] a)")};
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, vm::Byte(-1), vm::Byte(-1)));

    fn = compile_fn("(fn* ([a b] a) ([a b c] c))");
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, vm::Byte(-2), vm::Byte(-1)));
    expect_body_with_bytecode(*fn, 1, b(vm::LDL, vm::Byte(-1), vm::Byte(-1)));

    fn = compile_fn("(fn* [a b & c] a)");
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, vm::Byte(-3), vm::Byte(-1)));

    fn = compile_fn("(fn* [a b & c] b)");
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, vm::Byte(-2), vm::Byte(-1)));

    fn = compile_fn("(fn* [a b & c] c)");
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, vm::Byte(-1), vm::Byte(-1)));
}

TEST_F(compile_test, should_compile_functions_returning_vars)
{
    in_ns(create_symbol("cleo.compile.vars.test"));
    auto x = define(create_symbol("cleo.compile.vars.test", "x"), create_keyword(":abc"));
    Root fn{compile_fn("(fn* [] x)")};
    expect_body_with_vars_and_bytecode(*fn, 0, arrayv(x), b(vm::LDV, 0, 0));
}

TEST_F(compile_test, should_compile_functions_calling_functions)
{
    in_ns(create_symbol("cleo.compile.fns.test"));
    auto f = define(create_symbol("cleo.compile.fns.test", "f"), nil);
    auto g = define(create_symbol("cleo.compile.fns.test", "g"), nil);
    auto h = define(create_symbol("cleo.compile.fns.test", "h"), nil);
    Root fn{compile_fn("(fn* [f] (f))")};
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, -1, -1,
                                        vm::CALL, 0));

    fn = compile_fn("(fn* [f a b] (f a b))");
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, -3, -1,
                                        vm::LDL, -2, -1,
                                        vm::LDL, -1, -1,
                                        vm::CALL, 2));

    fn = compile_fn("(fn* [b f c a] (f a b c))");
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, -3, -1,
                                        vm::LDL, -1, -1,
                                        vm::LDL, -4, -1,
                                        vm::LDL, -2, -1,
                                        vm::CALL, 3));

    fn = compile_fn("(fn* [x] (f x g h))");
    expect_body_with_vars_and_bytecode(*fn, 0, arrayv(f, g, h), b(vm::LDV, 0, 0,
                                                                  vm::LDL, -1, -1,
                                                                  vm::LDV, 1, 0,
                                                                  vm::LDV, 2, 0,
                                                                  vm::CALL, 3));

    fn = compile_fn("(fn* [] (f f g h h g))");
    expect_body_with_vars_and_bytecode(*fn, 0, arrayv(f, g, h), b(vm::LDV, 0, 0,
                                                                  vm::LDV, 0, 0,
                                                                  vm::LDV, 1, 0,
                                                                  vm::LDV, 2, 0,
                                                                  vm::LDV, 2, 0,
                                                                  vm::LDV, 1, 0,
                                                                  vm::CALL, 5));

    fn = compile_fn("(fn* [f] (f 10 20 20 30 20 30 10))");
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(10, 20, 30), b(vm::LDL, -1, -1,
                                                                       vm::LDC, 0, 0,
                                                                       vm::LDC, 1, 0,
                                                                       vm::LDC, 1, 0,
                                                                       vm::LDC, 2, 0,
                                                                       vm::LDC, 1, 0,
                                                                       vm::LDC, 2, 0,
                                                                       vm::LDC, 0, 0,
                                                                       vm::CALL, 7));

    fn = compile_fn("(fn* [x] (f (g x (h 10 10) (h 20))))");
    expect_body_with_consts_vars_and_bytecode(*fn, 0,
                                              arrayv(10, 20),
                                              arrayv(f, g, h),
                                              b(vm::LDV, 0, 0,
                                                vm::LDV, 1, 0,
                                                vm::LDL, -1, -1,
                                                vm::LDV, 2, 0,
                                                vm::LDC, 0, 0,
                                                vm::LDC, 0, 0,
                                                vm::CALL, 2,
                                                vm::LDV, 2, 0,
                                                vm::LDC, 1, 0,
                                                vm::CALL, 1,
                                                vm::CALL, 3,
                                                vm::CALL, 1));
}

TEST_F(compile_test, should_compile_empty_lists_to_empty_list_constants)
{
    Root fn{compile_fn("(fn* [] ())")};
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(*EMPTY_LIST), b(vm::LDC, 0, 0));

    fn = compile_fn("(fn* [f] (f ()))");
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(*EMPTY_LIST), b(vm::LDL, -1, -1,
                                                                        vm::LDC, 0, 0,
                                                                        vm::CALL, 1));
}

TEST_F(compile_test, should_compile_functions_with_nil_body)
{
    Root fn(compile_fn("(fn* [] nil)"));
    expect_body_with_bytecode(*fn, 0, b(vm::CNIL));
}

TEST_F(compile_test, should_compile_functions_with_if_blocks)
{
    Root fn{compile_fn("(fn* [a b c] (if a b c))")};
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, -3, -1,
                                        vm::BNIL, 6, 0,
                                        vm::LDL, -2, -1,
                                        vm::BR, 3, 0,
                                        vm::LDL, -1, -1));

    fn = compile_fn("(fn* [a b c] (if (a) (b) (c)))");
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, -3, -1,
                                        vm::CALL, 0,
                                        vm::BNIL, 8, 0,
                                        vm::LDL, -2, -1,
                                        vm::CALL, 0,
                                        vm::BR, 5, 0,
                                        vm::LDL, -1, -1,
                                        vm::CALL, 0));
    fn = compile_fn("(fn* [a b c] (if a (if b (if c 101 102) 103) 104))");
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(101, 102, 103, 104),
                                         b(vm::LDL, -3, -1,
                                           vm::BNIL, 30, 0,
                                           vm::LDL, -2, -1,
                                           vm::BNIL, 18, 0,
                                           vm::LDL, -1, -1,
                                           vm::BNIL, 6, 0,
                                           vm::LDC, 0, 0,
                                           vm::BR, 3, 0,
                                           vm::LDC, 1, 0,
                                           vm::BR, 3, 0,
                                           vm::LDC, 2, 0,
                                           vm::BR, 3, 0,
                                           vm::LDC, 3, 0));

    fn = compile_fn("(fn* [x] (if a-var x))");
    expect_body_with_vars_and_bytecode(*fn, 0, arrayv(a_var), b(vm::LDV, 0, 0,
                                                                vm::BNIL, 6, 0,
                                                                vm::LDL, -1, -1,
                                                                vm::BR, 1, 0,
                                                                vm::CNIL));
}

TEST_F(compile_test, should_compile_do_blocks)
{
    Root fn{compile_fn("(fn* [] (do))")};
    expect_body_with_bytecode(*fn, 0, b(vm::CNIL));

    fn = compile_fn("(fn* [x] (do (x)))");
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, -1, -1,
                                        vm::CALL, 0));

    fn = compile_fn("(fn* [x y z] (do (x 10) (y a-var) (z)))");
    expect_body_with_consts_vars_and_bytecode(*fn, 0, arrayv(10), arrayv(a_var),
                                              b(vm::LDL, -3, -1,
                                                vm::LDC, 0, 0,
                                                vm::CALL, 1,
                                                vm::POP,
                                                vm::LDL, -2, -1,
                                                vm::LDV, 0, 0,
                                                vm::CALL, 1,
                                                vm::POP,
                                                vm::LDL, -1, -1,
                                                vm::CALL, 0));
}

TEST_F(compile_test, should_compile_quote)
{
    Root fn{compile_fn("(fn* [a] (quote (a-var xyz 10)))")};
    auto a_var_sym = create_symbol("a-var");
    auto xyz = create_symbol("xyz");
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(listv(a_var_sym, xyz, 10)), b(vm::LDC, 0, 0));
}

TEST_F(compile_test, should_compile_let_forms)
{
    Root fn{compile_fn("(fn* [a] (let* [] (a 10 a-var)))")};
    expect_body_with_consts_vars_and_bytecode(*fn, 0, arrayv(10), arrayv(a_var),
                                              b(vm::LDL, -1, -1,
                                                vm::LDC, 0, 0,
                                                vm::LDV, 0, 0,
                                                vm::CALL, 2));

    fn = compile_fn("(fn* [a] (let* [x a] x))");
    expect_body_with_locals_and_bytecode(*fn, 0, 1, b(vm::LDL, -1, -1,
                                                      vm::STL, 0, 0,
                                                      vm::LDL, 0, 0));

    fn = compile_fn("(fn* [x] (let* [x x] x))");
    expect_body_with_locals_and_bytecode(*fn, 0, 1, b(vm::LDL, -1, -1,
                                                      vm::STL, 0, 0,
                                                      vm::LDL, 0, 0));

    fn = compile_fn("(fn* [a b] (let* [x b y a-var z 10] (z x y)))");
    expect_body_with_locals_consts_vars_and_bytecode(*fn, 0, 3, arrayv(10), arrayv(a_var),
                                                     b(vm::LDL, -1, -1,
                                                       vm::STL, 0, 0,
                                                       vm::LDV, 0, 0,
                                                       vm::STL, 1, 0,
                                                       vm::LDC, 0, 0,
                                                       vm::STL, 2, 0,
                                                       vm::LDL, 2, 0,
                                                       vm::LDL, 0, 0,
                                                       vm::LDL, 1, 0,
                                                       vm::CALL, 2));

    fn = compile_fn("(fn* [a] (let* [x (let* [y (let* [z a] z)] y)] x))");
    expect_body_with_locals_and_bytecode(*fn, 0, 1, b(vm::LDL, -1, -1,
                                                      vm::STL, 0, 0,
                                                      vm::LDL, 0, 0,
                                                      vm::STL, 0, 0,
                                                      vm::LDL, 0, 0,
                                                      vm::STL, 0, 0,
                                                      vm::LDL, 0, 0));

    fn = compile_fn("(fn* [a] (let* [x a y x] (let* [x y z x] (let* [w z] w))))");
    expect_body_with_locals_and_bytecode(*fn, 0, 5, b(vm::LDL, -1, -1,
                                                      vm::STL, 0, 0,
                                                      vm::LDL, 0, 0,
                                                      vm::STL, 1, 0,
                                                      vm::LDL, 1, 0,
                                                      vm::STL, 2, 0,
                                                      vm::LDL, 2, 0,
                                                      vm::STL, 3, 0,
                                                      vm::LDL, 3, 0,
                                                      vm::STL, 4, 0,
                                                      vm::LDL, 4, 0));

    fn = compile_fn("(fn* [a] (if a (let* [x a y a] (x y)) (let* [x a] x)))");
    expect_body_with_locals_and_bytecode(*fn, 0, 2, b(vm::LDL, -1, -1,
                                                      vm::BNIL, 23, 0,
                                                      vm::LDL, -1, -1,
                                                      vm::STL, 0, 0,
                                                      vm::LDL, -1, -1,
                                                      vm::STL, 1, 0,
                                                      vm::LDL, 0, 0,
                                                      vm::LDL, 1, 0,
                                                      vm::CALL, 1,
                                                      vm::BR, 9, 0,
                                                      vm::LDL, -1, -1,
                                                      vm::STL, 0, 0,
                                                      vm::LDL, 0, 0));

    fn = compile_fn("(fn* [a] (let* [a00 nil a01 nil a02 nil a03 nil a04 nil a05 nil a06 nil a07 nil\n"
                    "                a08 nil a09 nil a0a nil a0b nil a0c nil a0d nil a0e nil a0f nil\n"
                    "                a10 nil a11 nil a12 nil a13 nil a14 nil a15 nil a16 nil a17 nil\n"
                    "                a18 nil a19 nil a1a nil a1b nil a1c nil a1d nil a1e nil a1f nil\n"
                    "                a20 nil a21 nil a22 nil a23 nil a24 nil a25 nil a26 nil a27 nil\n"
                    "                a28 nil a29 nil a2a nil a2b nil a2c nil a2d nil a2e nil a2f nil\n"
                    "                a30 nil a31 nil a32 nil a33 nil a34 nil a35 nil a36 nil a37 nil\n"
                    "                a38 nil a39 nil a3a nil a3b nil a3c nil a3d nil a3e nil a3f nil\n"
                    "                a40 nil a41 nil a42 nil a43 nil a44 nil a45 nil a46 nil a47 nil\n"
                    "                a48 nil a49 nil a4a nil a4b nil a4c nil a4d nil a4e nil a4f nil\n"
                    "                a50 nil a51 nil a52 nil a53 nil a54 nil a55 nil a56 nil a57 nil\n"
                    "                a58 nil a59 nil a5a nil a5b nil a5c nil a5d nil a5e nil a5f nil\n"
                    "                a60 nil a61 nil a62 nil a63 nil a64 nil a65 nil a66 nil a67 nil\n"
                    "                a68 nil a69 nil a6a nil a6b nil a6c nil a6d nil a6e nil a6f nil\n"
                    "                a70 nil a71 nil a72 nil a73 nil a74 nil a75 nil a76 nil a77 nil\n"
                    "                a78 nil a79 nil a7a nil a7b nil a7c nil a7d nil a7e nil a7f nil\n"
                    "                a80 nil a81 nil a82 nil a83 nil a84 nil a85 nil a86 nil a87 nil\n"
                    "                a88 nil a89 nil a8a nil a8b nil a8c nil a8d nil a8e nil a8f nil\n"
                    "                a90 nil a91 nil a92 nil a93 nil a94 nil a95 nil a96 nil a97 nil\n"
                    "                a98 nil a99 nil a9a nil a9b nil a9c nil a9d nil a9e nil a9f nil\n"
                    "                aa0 nil aa1 nil aa2 nil aa3 nil aa4 nil aa5 nil aa6 nil aa7 nil\n"
                    "                aa8 nil aa9 nil aaa nil aab nil aac nil aad nil aae nil aaf nil\n"
                    "                ab0 nil ab1 nil ab2 nil ab3 nil ab4 nil ab5 nil ab6 nil ab7 nil\n"
                    "                ab8 nil ab9 nil aba nil abb nil abc nil abd nil abe nil abf nil\n"
                    "                ac0 nil ac1 nil ac2 nil ac3 nil ac4 nil ac5 nil ac6 nil ac7 nil\n"
                    "                ac8 nil ac9 nil aca nil acb nil acc nil acd nil ace nil acf nil\n"
                    "                ad0 nil ad1 nil ad2 nil ad3 nil ad4 nil ad5 nil ad6 nil ad7 nil\n"
                    "                ad8 nil ad9 nil ada nil adb nil adc nil add nil ade nil adf nil\n"
                    "                ae0 nil ae1 nil ae2 nil ae3 nil ae4 nil ae5 nil ae6 nil ae7 nil\n"
                    "                ae8 nil ae9 nil aea nil aeb nil aec nil aed nil aee nil aef nil\n"
                    "                af0 nil af1 nil af2 nil af3 nil af4 nil af5 nil af6 nil af7 nil\n"
                    "                af8 nil af9 nil afa nil afb nil afc nil afd nil afe nil aff nil\n"
                    "                a100 a] a100))\n");
    EXPECT_EQ(257u, get_bytecode_fn_body_locals_size(get_bytecode_fn_body(*fn, 0)));
    auto code = bc(get_bytecode_fn_body(*fn, 0));
    ASSERT_GE(code.size(), 4 * 256);
    EXPECT_EQ(b(vm::LDL, -1, -1, vm::STL, 0, 1, vm::LDL, 0, 1), subvec(code, 4 * 256));
}

TEST_F(compile_test, should_compile_functions_with_recur)
{
    Root fn{compile_fn("(fn* [] (recur))")};

    expect_body_with_bytecode(*fn, 0, b(vm::BR, -3, -1));

    fn = compile_fn("(fn* [x] (recur 10))");

    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(10),
                                         b(vm::LDC, 0, 0,
                                           vm::STL, -1, -1,
                                           vm::BR, -9, -1));

    fn = compile_fn("(fn* [x y z] (recur a-var z x))");
    expect_body_with_vars_and_bytecode(*fn, 0, arrayv(a_var),
                                       b(vm::LDV, 0, 0,
                                         vm::LDL, -1, -1,
                                         vm::LDL, -3, -1,
                                         vm::STL, -1, -1,
                                         vm::STL, -2, -1,
                                         vm::STL, -3, -1,
                                         vm::BR, -21, -1));

    fn = compile_fn("(fn* [f x] (if x (recur f (f x))))");
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, -1, -1,
                                        vm::BNIL, 23, 0,
                                        vm::LDL, -2, -1,
                                        vm::LDL, -2, -1,
                                        vm::LDL, -1, -1,
                                        vm::CALL, 1,
                                        vm::STL, -1, -1,
                                        vm::STL, -2, -1,
                                        vm::BR, -26, -1,
                                        vm::BR, 1, 0,
                                        vm::CNIL));

    fn = compile_fn("(fn* [x & xs] (recur 1 2))");
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(1, 2),
                                         b(vm::LDC, 0, 0,
                                           vm::LDC, 1, 0,
                                           vm::STL, -1, -1,
                                           vm::STL, -2, -1,
                                           vm::BR, -15, -1));
}

TEST_F(compile_test, should_compile_functions_with_loop)
{
    Root fn{compile_fn("(fn* [a] (loop* [] (a 10 a-var)))")};
    expect_body_with_consts_vars_and_bytecode(*fn, 0, arrayv(10), arrayv(a_var),
                                              b(vm::LDL, -1, -1,
                                                vm::LDC, 0, 0,
                                                vm::LDV, 0, 0,
                                                vm::CALL, 2));

    fn = compile_fn("(fn* [a b] (loop* [x b y a-var z 10] (z x y)))");
    expect_body_with_locals_consts_vars_and_bytecode(*fn, 0, 3, arrayv(10), arrayv(a_var),
                                                     b(vm::LDL, -1, -1,
                                                       vm::STL, 0, 0,
                                                       vm::LDV, 0, 0,
                                                       vm::STL, 1, 0,
                                                       vm::LDC, 0, 0,
                                                       vm::STL, 2, 0,
                                                       vm::LDL, 2, 0,
                                                       vm::LDL, 0, 0,
                                                       vm::LDL, 1, 0,
                                                       vm::CALL, 2));

    fn = compile_fn("(fn* [] (loop* [] (recur)))");
    expect_body_with_bytecode(*fn, 0, b(vm::BR, -3, -1));

    fn = compile_fn("(fn* [a b f] (do (f) (loop* [x a y b] (recur y x))))");
    expect_body_with_locals_and_bytecode(*fn, 0, 2, b(vm::LDL, -1, -1,
                                                      vm::CALL, 0,
                                                      vm::POP,
                                                      vm::LDL, -3, -1,
                                                      vm::STL, 0, 0,
                                                      vm::LDL, -2, -1,
                                                      vm::STL, 1, 0,
                                                      vm::LDL, 1, 0,
                                                      vm::LDL, 0, 0,
                                                      vm::STL, 1, 0,
                                                      vm::STL, 0, 0,
                                                      vm::BR, -15, -1));

    fn = compile_fn("(fn* [x] (let* [a x] (loop* [b a a a] (recur a x))))");
    expect_body_with_locals_and_bytecode(*fn, 0, 3, b(vm::LDL, -1, -1,
                                                      vm::STL, 0, 0,
                                                      vm::LDL, 0, 0,
                                                      vm::STL, 1, 0,
                                                      vm::LDL, 0, 0,
                                                      vm::STL, 2, 0,
                                                      vm::LDL, 2, 0,
                                                      vm::LDL, -1, -1,
                                                      vm::STL, 2, 0,
                                                      vm::STL, 1, 0,
                                                      vm::BR, -15, -1));

    fn = compile_fn("(fn* [a b] (loop* [x a] (loop* [y b] (recur x))))");
    expect_body_with_locals_and_bytecode(*fn, 0, 2, b(vm::LDL, -2, -1,
                                                      vm::STL, 0, 0,
                                                      vm::LDL, -1, -1,
                                                      vm::STL, 1, 0,
                                                      vm::LDL, 0, 0,
                                                      vm::STL, 1, 0,
                                                      vm::BR, -9, -1));
}

TEST_F(compile_test, should_compile_vectors)
{
    Root fn{compile_fn("(fn* [] [])")};
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(*EMPTY_VECTOR), b(vm::LDC, 0, 0));

    fn = compile_fn("(fn* [] [5 6 7])");
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(arrayv(5, 6, 7)), b(vm::LDC, 0, 0));

    fn = compile_fn("(fn* [x] [3 4 x])");
    expect_body_with_consts_and_bytecode(*fn, 0,
                                         arrayv(*rt::transient_array_persistent,
                                                *rt::transient_array_conj,
                                                *rt::transient_array,
                                                arrayv(3, 4)),
                                         b(vm::LDC, 0, 0,
                                           vm::LDC, 1, 0,
                                           vm::LDC, 2, 0,
                                           vm::LDC, 3, 0,
                                           vm::CALL, 1,
                                           vm::LDL, -1, -1,
                                           vm::CALL, 2,
                                           vm::CALL, 1));

    fn = compile_fn("(fn* [x y z] [z x y])");
    expect_body_with_consts_and_bytecode(*fn, 0,
                                         arrayv(*rt::transient_array_persistent,
                                                *rt::transient_array_conj,
                                                *rt::transient_array,
                                                *EMPTY_VECTOR),
                                         b(vm::LDC, 0, 0,
                                           vm::LDC, 1, 0,
                                           vm::LDC, 1, 0,
                                           vm::LDC, 1, 0,
                                           vm::LDC, 2, 0,
                                           vm::LDC, 3, 0,
                                           vm::CALL, 1,
                                           vm::LDL, -1, -1,
                                           vm::CALL, 2,
                                           vm::LDL, -3, -1,
                                           vm::CALL, 2,
                                           vm::LDL, -2, -1,
                                           vm::CALL, 2,
                                           vm::CALL, 1));

    fn = compile_fn("(fn* [f] [2 4.0 \"x\" :k (f 3)])");
    expect_body_with_consts_and_bytecode(*fn, 0,
                                         arrayv(*rt::transient_array_persistent,
                                                *rt::transient_array_conj,
                                                *rt::transient_array,
                                                arrayv(2, 4.0, "x", create_keyword("k")),
                                                3),
                                         b(vm::LDC, 0, 0,
                                           vm::LDC, 1, 0,
                                           vm::LDC, 2, 0,
                                           vm::LDC, 3, 0,
                                           vm::CALL, 1,
                                           vm::LDL, -1, -1,
                                           vm::LDC, 4, 0,
                                           vm::CALL, 1,
                                           vm::CALL, 2,
                                           vm::CALL, 1));
}

TEST_F(compile_test, should_compile_hash_sets)
{
    Root fn{compile_fn("(fn* [] #{})")};
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(*EMPTY_SET), b(vm::LDC, 0, 0));

    fn = compile_fn("(fn* [] #{5 6 7})");
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(asetv(5, 6, 7)), b(vm::LDC, 0, 0));

    fn = compile_fn("(fn* [x] #{3 x 4})");
    expect_body_with_consts_and_bytecode(*fn, 0,
                                         arrayv(*rt::array_set_conj,
                                                asetv(3, 4)),
                                         b(vm::LDC, 0, 0,
                                           vm::LDC, 1, 0,
                                           vm::LDL, -1, -1,
                                           vm::CALL, 2));

    fn = compile_fn("(fn* [x y z] #{z x y})");
    expect_body_with_consts_and_bytecode(*fn, 0,
                                         arrayv(*rt::array_set_conj,
                                                *EMPTY_SET),
                                         b(vm::LDC, 0, 0,
                                           vm::LDC, 0, 0,
                                           vm::LDC, 0, 0,
                                           vm::LDC, 1, 0,
                                           vm::LDL, -1, -1,
                                           vm::CALL, 2,
                                           vm::LDL, -3, -1,
                                           vm::CALL, 2,
                                           vm::LDL, -2, -1,
                                           vm::CALL, 2));

    fn = compile_fn("(fn* [f] #{2 4.0 \"x\" (f 3) :k})");
    expect_body_with_consts_and_bytecode(*fn, 0,
                                         arrayv(*rt::array_set_conj,
                                                asetv(2, 4.0, "x", create_keyword("k")),
                                                3),
                                         b(vm::LDC, 0, 0,
                                           vm::LDC, 1, 0,
                                           vm::LDL, -1, -1,
                                           vm::LDC, 2, 0,
                                           vm::CALL, 1,
                                           vm::CALL, 2));
}

TEST_F(compile_test, should_compile_hash_maps)
{
    Root fn{compile_fn("(fn* [] {})")};
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(*EMPTY_MAP), b(vm::LDC, 0, 0));

    fn = compile_fn("(fn* [] {5 6 7 8})");
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(phmapv(5, 6, 7, 8)), b(vm::LDC, 0, 0));

    fn = compile_fn("(fn* [x y] {3 4 5 6 x y})");
    expect_body_with_consts_and_bytecode(*fn, 0,
                                         arrayv(*rt::persistent_hash_map_assoc,
                                                phmapv(3, 4, 5, 6)),
                                         b(vm::LDC, 0, 0,
                                           vm::LDC, 1, 0,
                                           vm::LDL, -2, -1,
                                           vm::LDL, -1, -1,
                                           vm::CALL, 3));

    fn = compile_fn("(fn* [x] {3 4 x 6})");
    expect_body_with_consts_and_bytecode(*fn, 0,
                                         arrayv(*rt::persistent_hash_map_assoc,
                                                phmapv(3, 4),
                                                6),
                                         b(vm::LDC, 0, 0,
                                           vm::LDC, 1, 0,
                                           vm::LDL, -1, -1,
                                           vm::LDC, 2, 0,
                                           vm::CALL, 3));

    fn = compile_fn("(fn* [x] {3 4 5 x})");
    expect_body_with_consts_and_bytecode(*fn, 0,
                                         arrayv(*rt::persistent_hash_map_assoc,
                                                phmapv(3, 4),
                                                5),
                                         b(vm::LDC, 0, 0,
                                           vm::LDC, 1, 0,
                                           vm::LDC, 2, 0,
                                           vm::LDL, -1, -1,
                                           vm::CALL, 3));

    fn = compile_fn("(fn* [x y] {3 4 5 x y 6})");
    ASSERT_THAT(bc(get_bytecode_fn_body(*fn, 0)), AnyOf(b(vm::LDC, 0, 0,
                                                          vm::LDC, 0, 0,
                                                          vm::LDC, 1, 0,
                                                          vm::LDC, 2, 0,
                                                          vm::LDL, -2, -1,
                                                          vm::CALL, 3,
                                                          vm::LDL, -1, -1,
                                                          vm::LDC, 3, 0,
                                                          vm::CALL, 3),
                                                        b(vm::LDC, 0, 0,
                                                          vm::LDC, 0, 0,
                                                          vm::LDC, 1, 0,
                                                          vm::LDL, -1, -1,
                                                          vm::LDC, 3, 0,
                                                          vm::CALL, 3,
                                                          vm::LDC, 2, 0,
                                                          vm::LDL, -2, -1,
                                                          vm::CALL, 3)));
}

TEST_F(compile_test, should_expand_macros)
{
    in_ns(create_symbol("cleo.compile.macro.test"));
    auto plus = get_var(PLUS);
    Root add{compile_fn("(fn* [&form &env x y] `(cleo.core/+ ~x ~y))")};
    Root meta{amap(MACRO_KEY, TRUE)};
    define(create_symbol("cleo.compile.macro.test", "add"), *add, *meta);
    Root fn{compile_fn("(fn* [] (add (add 1 2) (add 3 4)))")};
    expect_body_with_consts_vars_and_bytecode(*fn, 0,
                                              arrayv(1, 2, 3, 4),
                                              arrayv(plus),
                                              b(vm::LDV, 0, 0,
                                                vm::LDV, 0, 0,
                                                vm::LDC, 0, 0,
                                                vm::LDC, 1, 0,
                                                vm::CALL, 2,
                                                vm::LDV, 0, 0,
                                                vm::LDC, 2, 0,
                                                vm::LDC, 3, 0,
                                                vm::CALL, 2,
                                                vm::CALL, 2));

    Root nilf{compile_fn("(fn* [&form &env x] nil)")};
    define(create_symbol("cleo.compile.macro.test", "nilf"), *nilf, *meta);
    fn = compile_fn("(fn* [] (nilf 10))");
    expect_body_with_bytecode(*fn, 0, b(vm::CNIL));
}

TEST_F(compile_test, should_compile_def)
{
    in_ns(create_symbol("cleo.compile.def.test"));
    Root fn{compile_fn("(fn* [] (def x 10))")};
    auto v = get_var(create_symbol("cleo.compile.def.test", "x"));
    EXPECT_EQ_REFS(v, resolve_var(create_symbol("x")));
    expect_body_with_consts_vars_and_bytecode(*fn, 0, arrayv(10), arrayv(v), b(vm::LDV, 0, 0, vm::LDC, 0, 0, vm::CNIL, vm::SETV));

    fn = compile_fn("(fn* [f z] (def y (f z)))");
    expect_body_with_vars_and_bytecode(*fn, 0, arrayv(get_var(create_symbol("cleo.compile.def.test", "y"))),
                                       b(vm::LDV, 0, 0,
                                         vm::LDL, -2, -1,
                                         vm::LDL, -1, -1,
                                         vm::CALL, 1,
                                         vm::CNIL,
                                         vm::SETV));

    fn = compile_fn("(fn* [] (def {10 20} z 13))");
    Root meta{phmap(10, 20)};
    v = get_var(create_symbol("cleo.compile.def.test", "z"));
    EXPECT_EQ_VALS(*meta, get_var_meta(v));
    expect_body_with_consts_vars_and_bytecode(*fn, 0, arrayv(13, *meta), arrayv(v), b(vm::LDV, 0, 0, vm::LDC, 0, 0, vm::LDC, 1, 0, vm::SETV));

    fn = compile_fn("(fn* [] (def w))");
    v = get_var(create_symbol("cleo.compile.def.test", "w"));
    expect_body_with_vars_and_bytecode(*fn, 0, arrayv(v), b(vm::LDV, 0, 0, vm::CNIL, vm::CNIL, vm::SETV));

    v = define(create_symbol("cleo.compile.def.test", "ex"), nil, nil);
    fn = compile_fn("(fn* [] (def ex))");
    expect_body_with_vars_and_bytecode(*fn, 0, arrayv(v), b(vm::LDV, 0, 0, vm::CNIL, vm::CNIL, vm::SETV));

    fn = compile_fn("(fn* [] (def cleo.compile.def.test/nv))");
    v = get_var(create_symbol("cleo.compile.def.test", "nv"));
    expect_body_with_vars_and_bytecode(*fn, 0, arrayv(v), b(vm::LDV, 0, 0, vm::CNIL, vm::CNIL, vm::SETV));
}

TEST_F(compile_test, should_compile_functions_applying_functions)
{
    in_ns(create_symbol("cleo.compile.apply.test"));
    auto f = define(create_symbol("cleo.compile.apply.test", "f"), nil);
    auto g = define(create_symbol("cleo.compile.apply.test", "g"), nil);
    auto h = define(create_symbol("cleo.compile.apply.test", "h"), nil);
    Root fn{compile_fn("(fn* [f] (apply* f nil))")};
    expect_body_with_bytecode(*fn, 0, b(vm::LDL, -1, -1,
                                        vm::CNIL,
                                        vm::APPLY, 0));

    fn = compile_fn("(fn* [x] (apply* f x g h))");
    expect_body_with_vars_and_bytecode(*fn, 0, arrayv(f, g, h), b(vm::LDV, 0, 0,
                                                                  vm::LDL, -1, -1,
                                                                  vm::LDV, 1, 0,
                                                                  vm::LDV, 2, 0,
                                                                  vm::APPLY, 2));

    fn = compile_fn("(fn* [x] (apply* f (apply* g x (h 10 10) (h 20))))");
    expect_body_with_consts_vars_and_bytecode(*fn, 0,
                                              arrayv(10, 20),
                                              arrayv(f, g, h),
                                              b(vm::LDV, 0, 0,
                                                vm::LDV, 1, 0,
                                                vm::LDL, -1, -1,
                                                vm::LDV, 2, 0,
                                                vm::LDC, 0, 0,
                                                vm::LDC, 0, 0,
                                                vm::CALL, 2,
                                                vm::LDV, 2, 0,
                                                vm::LDC, 1, 0,
                                                vm::CALL, 1,
                                                vm::APPLY, 2,
                                                vm::APPLY, 0));
}

TEST_F(compile_test, should_compile_functions_creating_functions)
{
    Root fn{compile_fn("(fn* [] (fn* [] 10))")};
    auto inner_fn = get_fn_const(*fn, 0, 0);
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(inner_fn), b(vm::LDC, 0, 0));
    expect_body_with_consts_and_bytecode(inner_fn, 0, arrayv(10), b(vm::LDC, 0, 0));

    fn = compile_fn("(fn* [x y] (fn* [] (x y)))");
    inner_fn = get_fn_const(*fn, 0, 0);
    expect_body_with_consts_and_bytecode(*fn, 0,
                                         arrayv(inner_fn),
                                         b(vm::LDC, 0, 0,
                                           vm::LDL, -2, -1,
                                           vm::LDL, -1, -1,
                                           vm::IFN, 2));
    expect_body_with_consts_and_bytecode(inner_fn, 0,
                                         arrayv(nil, nil),
                                         b(vm::LDC, 0, 0,
                                           vm::LDC, 1, 0,
                                           vm::CALL, 1));

    fn = compile_fn("(fn* [x y] (fn* [] (x 10 y y x 20 30)))");
    inner_fn = get_fn_const(*fn, 0, 0);
    expect_body_with_consts_and_bytecode(*fn, 0,
                                         arrayv(inner_fn),
                                         b(vm::LDC, 0, 0,
                                           vm::LDL, -2, -1,
                                           vm::LDL, -1, -1,
                                           vm::IFN, 2));
    expect_body_with_consts_and_bytecode(inner_fn, 0,
                                         arrayv(10, 20, 30, nil, nil),
                                         b(vm::LDC, 3, 0,
                                           vm::LDC, 0, 0,
                                           vm::LDC, 4, 0,
                                           vm::LDC, 4, 0,
                                           vm::LDC, 3, 0,
                                           vm::LDC, 1, 0,
                                           vm::LDC, 2, 0,
                                           vm::CALL, 6));

    fn = compile_fn("(fn* [x y] (let* [z x] (fn* [] (z x))))");
    inner_fn = get_fn_const(*fn, 0, 0);
    expect_body_with_locals_consts_and_bytecode(*fn, 0, 1,
                                                arrayv(inner_fn),
                                                b(vm::LDL, -2, -1,
                                                  vm::STL, 0, 0,
                                                  vm::LDC, 0, 0,
                                                  vm::LDL, 0, 0,
                                                  vm::LDL, -2, -1,
                                                  vm::IFN, 2));
    expect_body_with_consts_and_bytecode(inner_fn, 0,
                                         arrayv(nil, nil),
                                         b(vm::LDC, 0, 0,
                                           vm::LDC, 1, 0,
                                           vm::CALL, 1));

    fn = compile_fn("(fn* [x y a-var] (fn* [x] (let* [y 5] (x y a-var))))");
    inner_fn = get_fn_const(*fn, 0, 0);
    expect_body_with_consts_and_bytecode(*fn, 0, arrayv(inner_fn), b(vm::LDC, 0, 0, vm::LDL, -1, -1, vm::IFN, 1));
    expect_body_with_locals_consts_and_bytecode(inner_fn, 0, 1,
                                                arrayv(5, nil),
                                                b(vm::LDC, 0, 0,
                                                  vm::STL, 0, 0,
                                                  vm::LDL, -1, -1,
                                                  vm::LDL, 0, 0,
                                                  vm::LDC, 1, 0,
                                                  vm::CALL, 2));
}

TEST_F(compile_test, should_fail_when_the_form_is_malformed)
{
    expect_compilation_error("10");
    expect_compilation_error("(bad [] 10)");
    expect_compilation_error("(fn* [] xyz)");

    expect_compilation_error("(fn* [] (quote))");
    expect_compilation_error("(fn* [] (quote 10 20))");

    expect_compilation_error("(fn* [] (let*))");
    expect_compilation_error("(fn* [] (let* () nil))");
    expect_compilation_error("(fn* [] (let* [a 10 b] nil))");
    expect_compilation_error("(fn* [] (let* [x x] nil))");

    expect_compilation_error("(fn* [] (loop*))");
    expect_compilation_error("(fn* [] (loop* () nil))");
    expect_compilation_error("(fn* [] (loop* [a 10 b] nil))");
    expect_compilation_error("(fn* [] (loop* [x x] nil))");
    expect_compilation_error("(fn* [x] (loop* [] (recur 1)))");
    expect_compilation_error("(fn* [] (loop* [x y] (recur)))");
    expect_compilation_error("(fn* [] (loop* [x y] (recur 1)))");
    expect_compilation_error("(fn* [] (loop* [x y] (recur 1 2 3)))");

    expect_compilation_error("(fn* [] (recur 1))");
    expect_compilation_error("(fn* [x y] (recur 1))");
    expect_compilation_error("(fn* [x y] (recur 1 2 3))");
    expect_compilation_error("(fn* [x & y] (recur 1))");
    expect_compilation_error("(fn* [x & y] (recur 1 2 3))");

    expect_compilation_error("(fn* [] (def))", "Too few arguments to def");
    expect_compilation_error("(fn* [] (def {2 3}))", "Too few arguments to def");
    expect_compilation_error("(fn* [] (def x y))");
    expect_compilation_error("(fn* [] (def x 10 30))", "Too many arguments to def");
    expect_compilation_error("(fn* [] (def {} x 10 30))", "Too many arguments to def");
    expect_compilation_error("(fn* [] (def 10 20))", "First argument to def must be a Symbol");
    expect_compilation_error("(fn* [] (def 10 x))", "First argument to def must be a Symbol");
    define(create_symbol("cleo.compile.def.test.other", "ex"), nil, nil);
    expect_compilation_error("(fn* [] (def cleo.compile.def.test.other/ex))", "Can't create defs outside of current ns");
    expect_compilation_error("(fn* [] (def cleo.compile.def.test.other/nex))", "Can't refer to qualified var that doesn't exist");

    expect_compilation_error("(fn* [] (apply*))", "Wrong number of args (0) passed to apply*, form: (apply*)");
    expect_compilation_error("(fn* [f] (apply* f))", "Wrong number of args (1) passed to apply*, form: (apply* f)");
}

}
}
