#include "global.hpp"
#include "small_vector.hpp"
#include "multimethod.hpp"
#include "equality.hpp"
#include "list.hpp"
#include "print.hpp"

namespace cleo
{

std::vector<void *> allocations;
std::vector<Value> extra_roots;
unsigned gc_frequency = 64;
unsigned gc_counter = gc_frequency - 1;

std::unordered_map<std::string, std::unordered_map<std::string, Value>> symbols;
std::unordered_map<std::string, std::unordered_map<std::string, Value>> keywords;

std::unordered_map<Value, Value> vars;

std::unordered_map<Value, Multimethod> multimethods;
Hierachy global_hierarchy;

const Value TRUE = create_keyword("true");
const Value SEQ = create_symbol("cleo.core", "seq");
const Value FIRST = create_symbol("cleo.core", "first");
const Value NEXT = create_symbol("cleo.core", "next");
const Value OBJ_EQ = create_symbol("cleo.core", "obj=");
const Value PR_STR_OBJ = create_symbol("cleo.core", "pr-str-obj");
const Value QUOTE = create_symbol("quote");
const Value FN = create_symbol("fn");

namespace type
{
const Value NATIVE_FUNCTION = create_symbol("cleo.core", "NativeFunction");
const Value CONS = create_symbol("cleo.core", "Cons");
const Value LIST = create_symbol("cleo.core", "List");
const Value SMALL_VECTOR = create_symbol("cleo.core", "SmallVector");
const Value SMALL_VECTOR_SEQ = create_symbol("cleo.core", "SmallVectorSeq");
const Value SMALL_MAP = create_symbol("cleo.core", "SmallMap");
const Value MULTIMETHOD = create_symbol("cleo.core", "Multimethod");
const Value SEQUABLE = create_symbol("cleo.core", "Sequable");
const Value SEQUENCE = create_symbol("cleo.core", "Sequence");
const Value FN = create_symbol("cleo.core", "Fn");
}

const std::array<Value, 7> type_by_tag{{
    nil,
    type::NATIVE_FUNCTION,
    create_symbol("cleo.core", "Symbol"),
    create_symbol("cleo.core", "Keyword"),
    create_symbol("cleo.core", "Int64"),
    create_symbol("cleo.core", "Float64"),
    create_symbol("cleo.core", "String")
}};

namespace
{

const Root first_type{create_native_function1<get_value_type>()};

const Root equal_dispatch{create_native_function([](const Value *args, std::uint8_t numArgs)
{
    std::array<Value, 2> types{{get_value_type(args[0]), get_value_type(args[1])}};
    return create_small_vector(types.data(), types.size());
})};

const Root ret_nil{create_native_function([](const Value *, std::uint8_t)
{
    return force(nil);
})};

Value identity(Value val)
{
    return val;
}

struct Initialize
{
    Initialize()
    {
        define_multimethod(SEQ, *first_type, nil);
        define_multimethod(FIRST, *first_type, nil);
        define_multimethod(NEXT, *first_type, nil);

        derive(type::LIST, type::SEQUABLE);
        Root f;
        f = create_native_function1<list_seq>();
        define_method(SEQ, type::LIST, *f);
        f = create_native_function1<get_list_first>();
        define_method(FIRST, type::LIST, *f);
        f = create_native_function1<get_list_next>();
        define_method(NEXT, type::LIST, *f);

        derive(type::SMALL_VECTOR, type::SEQUABLE);
        f = create_native_function1<small_vector_seq>();
        define_method(SEQ, type::SMALL_VECTOR, *f);
        f = create_native_function1<get_small_vector_seq_first>();
        define_method(FIRST, type::SMALL_VECTOR_SEQ, *f);
        f = create_native_function1<get_small_vector_seq_next>();
        define_method(NEXT, type::SMALL_VECTOR_SEQ, *f);

        derive(type::SMALL_VECTOR_SEQ, type::SEQUENCE);
        derive(type::SEQUENCE, type::SEQUABLE);
        f = create_native_function1<identity>();
        define_method(SEQ, type::SEQUENCE, *f);

        define_multimethod(OBJ_EQ, *equal_dispatch, nil);
        define_method(OBJ_EQ, nil, *ret_nil);

        std::array<Value, 2> two_seq{{type::SEQUABLE, type::SEQUABLE}};
        Root v{create_small_vector(two_seq.data(), two_seq.size())};
        f = create_native_function2<are_seqables_equal>();
        define_method(OBJ_EQ, *v, *f);

        define_multimethod(PR_STR_OBJ, *first_type, nil);
        f = create_native_function1<pr_str_small_vector>();
        define_method(PR_STR_OBJ, type::SMALL_VECTOR, *f);
        f = create_native_function1<pr_str_sequable>();
        define_method(PR_STR_OBJ, type::SEQUABLE, *f);
        f = create_native_function1<pr_str_object>();
        define_method(PR_STR_OBJ, nil, *f);
    }
} initialize;

}

}
