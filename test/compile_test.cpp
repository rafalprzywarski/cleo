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
};

TEST_F(compile_test, should_compile_functions_returning_constants)
{
    Root form{read_str("(fn [] 27)")};
    Root fn{compile_fn(*form, nil)};
    ASSERT_EQ_VALS(*type::BytecodeFn, get_value_type(*fn));
    ASSERT_EQ(1u, get_bytecode_fn_size(*fn));
    ASSERT_EQ(0u, get_bytecode_fn_arity(*fn, 0));
    auto body = get_bytecode_fn_body(*fn, 0);
    ASSERT_EQ_VALS(nil, get_bytecode_fn_body_vars(body));
    ASSERT_EQ(0u, get_bytecode_fn_body_locals_size(body));
    ASSERT_EQ(b(vm::LDC, 0, 0), bc(body));
    Root consts{array(27)};
    ASSERT_EQ_VALS(*consts, get_bytecode_fn_body_consts(body));
}

}
}
