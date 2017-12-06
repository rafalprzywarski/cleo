#include "global.hpp"
#include "small_vector.hpp"
#include "multimethod.hpp"
#include "equality.hpp"
#include "list.hpp"

namespace cleo
{

std::unordered_map<std::string, std::unordered_map<std::string, Value>> symbols;
std::unordered_map<std::string, std::unordered_map<std::string, Value>> keywords;

std::unordered_map<Value, Value> vars;

std::unordered_map<Value, Multimethod> multimethods;
Hierachy global_hierarchy;

const std::array<Value, 7> type_by_tag{{
    nil,
    create_symbol("cleo.core", "NativeFunction"),
    create_symbol("cleo.core", "Symbol"),
    create_symbol("cleo.core", "Keyword"),
    create_symbol("cleo.core", "Int64"),
    create_symbol("cleo.core", "Float64"),
    create_symbol("cleo.core", "String")
}};

const Value TRUE = create_keyword("true");
const Value SEQ = create_symbol("cleo.code", "seq");
const Value FIRST = create_symbol("cleo.code", "first");
const Value NEXT = create_symbol("cleo.code", "next");
const Value OBJ_EQ = create_symbol("cleo.core", "obj=");

namespace type
{
const Value CONS = create_symbol("cleo.core", "Cons");
const Value LIST = create_symbol("cleo.core", "List");
const Value SMALL_VECTOR = create_symbol("cleo.core", "SmallVector");
const Value SMALL_VECTOR_SEQ = create_symbol("cleo.core", "SmallVectorSeq");
const Value MULTIMETHOD = create_symbol("cleo.core", "Multimethod");
const Value SEQUABLE = create_symbol("cleo.core", "Sequable");
}

namespace
{

const Value first_type = create_native_function1<get_value_type>();

const Value equal_dispatch = create_native_function([](const Value *args, std::uint8_t numArgs)
{
    std::array<Value, 2> types{{get_value_type(args[0]), get_value_type(args[1])}};
    return create_small_vector(types.data(), types.size());
});

const Value ret_nil = create_native_function([](const Value *, std::uint8_t)
{
    return nil;
});

struct Initialize
{
    Initialize()
    {
        define_multimethod(SEQ, first_type, nil);
        define_multimethod(FIRST, first_type, nil);
        define_multimethod(NEXT, first_type, nil);

        derive(type::LIST, type::SEQUABLE);
        define_method(SEQ, type::LIST, create_native_function1<list_seq>());
        define_method(FIRST, type::LIST, create_native_function1<get_list_first>());
        define_method(NEXT, type::LIST, create_native_function1<get_list_next>());

        derive(type::SMALL_VECTOR, type::SEQUABLE);
        define_method(SEQ, type::SMALL_VECTOR, create_native_function1<small_vector_seq>());
        define_method(FIRST, type::SMALL_VECTOR_SEQ, create_native_function1<get_small_vector_seq_first>());
        define_method(NEXT, type::SMALL_VECTOR_SEQ, create_native_function1<get_small_vector_seq_next>());

        define_multimethod(OBJ_EQ, equal_dispatch, nil);
        define_method(OBJ_EQ, nil, ret_nil);

        std::array<Value, 2> two_seq{{type::SEQUABLE, type::SEQUABLE}};
        define_method(OBJ_EQ, create_small_vector(two_seq.data(), two_seq.size()), create_native_function2<are_seqables_equal>());
    }
} initialize;

}

}