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
    compile_test() : Test("cleo.compile.test") {}
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
}

TEST_F(compile_test, should_fail_when_the_form_is_malformed)
{
    expect_compilation_error("10");
    expect_compilation_error("(bad [] 10)");
    expect_compilation_error("(fn* [] xyz)");
}

}
}
