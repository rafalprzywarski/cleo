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
}

TEST_F(compile_test, should_fail_when_the_form_is_malformed)
{
    expect_compilation_error("10");
    expect_compilation_error("(bad [] 10)");
}

}
}
