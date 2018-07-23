#include <cleo/compile.hpp>
#include "util.hpp"
#include <cleo/eval.hpp>
#include <cleo/reader.hpp>
#include <cleo/global.hpp>
#include <cleo/bytecode_fn.hpp>

namespace cleo
{
namespace test
{

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

    void expect_compilation_error(std::string form_str)
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
        }
    }

    Force compile_fn(const std::string& s)
    {
        Root form{read_str(s)};
        return cleo::compile_fn(*form, nil);
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
    void expect_body_with_locals_consts_vars_and_bytecode(Value fn, std::uint8_t index, std::uint32_t locals_size,  Consts constsv, Vars varsv, std::vector<vm::Byte> code)
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
}

}
}