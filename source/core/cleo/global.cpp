#include "global.hpp"
#include "small_vector.hpp"
#include "multimethod.hpp"
#include "equality.hpp"
#include "list.hpp"
#include "print.hpp"
#include "var.hpp"
#include "error.hpp"
#include "eval.hpp"
#include "small_set.hpp"
#include "small_map.hpp"
#include "namespace.hpp"

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

Root current_exception;

const Value TRUE = create_keyword("true");
const Value SEQ = create_symbol("cleo.core", "seq");
const Value FIRST = create_symbol("cleo.core", "first");
const Value NEXT = create_symbol("cleo.core", "next");
const Value OBJ_EQ = create_symbol("cleo.core", "obj=");
const Value OBJ_CALL = create_symbol("cleo.core", "obj-call");
const Value PR_STR_OBJ = create_symbol("cleo.core", "pr-str-obj");
const Value GET_MESSAGE = create_symbol("cleo.core", "get-message");
const Value QUOTE = create_symbol("quote");
const Value FN = create_symbol("fn");
const Value MACRO = create_symbol("macro");
const Value DEF = create_symbol("def");
const Value LET = create_symbol("let");
const Value IF = create_symbol("if");
const Value LOOP = create_symbol("loop");
const Value RECUR = create_symbol("recur");
const Value PLUS = create_symbol("+");
const Value MINUS = create_symbol("-");
const Value ASTERISK = create_symbol("*");
const Value LT = create_symbol("<");
const Value EQ = create_symbol("=");
const Value THROW = create_symbol("throw");
const Value TRY = create_symbol("try");
const Value CATCH = create_symbol("catch");
const Value FINALLY = create_symbol("finally");
const Value VA = create_symbol("&");
const Value CURRENT_NS = create_symbol("cleo.core", "*ns*");
const Value IN_NS = create_symbol("cleo.core", "in-ns");

namespace type
{
const Value NATIVE_FUNCTION = create_symbol("cleo.core", "NativeFunction");
const Value CONS = create_symbol("cleo.core", "Cons");
const Value LIST = create_symbol("cleo.core", "List");
const Value SMALL_VECTOR = create_symbol("cleo.core", "SmallVector");
const Value SMALL_VECTOR_SEQ = create_symbol("cleo.core", "SmallVectorSeq");
const Value SMALL_MAP = create_symbol("cleo.core", "SmallMap");
const Value SMALL_SET = create_symbol("cleo.core", "SmallSet");
const Value SMALL_SET_SEQ = create_symbol("cleo.core", "SmallSetSeq");
const Value MULTIMETHOD = create_symbol("cleo.core", "Multimethod");
const Value SEQABLE = create_symbol("cleo.core", "Seqable");
const Value SEQUENCE = create_symbol("cleo.core", "Sequence");
const Value Callable = create_symbol("cleo.core", "Callable");
const Value FN = create_symbol("cleo.core", "Fn");
const Value Macro = create_symbol("cleo.core", "Macro");
const Value RECUR = create_symbol("cleo.core", "Recur");
const Value Exception = create_symbol("cleo.core", "Exception");
const Value ReadError = create_symbol("cleo.core", "ReadError");
const Value CallError = create_symbol("cleo.core", "CallError");
const Value SymbolNotFound = create_symbol("cleo.core", "SymbolNotFound");
const Value IllegalArgument = create_symbol("cleo.core", "IllegalArgument");
const Value UnexpectedEndOfInput = create_symbol("cleo.core", "UnexpectedEndOfInput");
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

const Root EMPTY_LIST{create_list(nullptr, 0)};

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

Value nil_seq(Value)
{
    return nil;
}

Force add2(Value l, Value r)
{
    return create_int64(get_int64_value(l) + get_int64_value(r));
}

Force sub2(Value l, Value r)
{
    return create_int64(get_int64_value(l) - get_int64_value(r));
}

Force mult2(Value l, Value r)
{
    return create_int64(get_int64_value(l) * get_int64_value(r));
}

Force lt2(Value l, Value r)
{
    return get_int64_value(l) < get_int64_value(r) ? TRUE : nil;
}

Root recur{create_native_function([](const Value *args, std::uint8_t n)
{
    return create_object(type::RECUR, args, n);
})};

Force pr_str_exception(Value e)
{
    auto get_message = lookup(GET_MESSAGE);
    cleo::Root msg{call_multimethod1(get_message, e)};
    cleo::Root type{cleo::pr_str(cleo::get_value_type(e))};

    return create_string(
        std::string(get_string_ptr(*type), get_string_len(*type)) + ": " +
        std::string(get_string_ptr(*msg), get_string_len(*msg)));
}

Root mk_list{create_native_function([](const Value *args, std::uint8_t n)
{
    return create_list(args, n);
})};

struct Initialize
{
    Initialize()
    {
        Root f;

        define(CURRENT_NS, nil);
        f = create_native_function1<in_ns>();
        define(IN_NS, *f);

        auto undefined = create_symbol("cleo.core/-UNDEFINED-");
        define_multimethod(SEQ, *first_type, undefined);
        define_multimethod(FIRST, *first_type, undefined);
        define_multimethod(NEXT, *first_type, undefined);

        f = create_native_function1<nil_seq>();
        define_method(SEQ, nil, *f);
        f = create_native_function1<nil_seq>();
        define_method(FIRST, nil, *f);
        f = create_native_function1<nil_seq>();
        define_method(NEXT, nil, *f);

        derive(type::LIST, type::SEQABLE);
        f = create_native_function1<list_seq>();
        define_method(SEQ, type::LIST, *f);
        f = create_native_function1<get_list_first>();
        define_method(FIRST, type::LIST, *f);
        f = create_native_function1<get_list_next>();
        define_method(NEXT, type::LIST, *f);

        derive(type::SMALL_VECTOR, type::SEQABLE);
        f = create_native_function1<small_vector_seq>();
        define_method(SEQ, type::SMALL_VECTOR, *f);
        f = create_native_function1<get_small_vector_seq_first>();
        define_method(FIRST, type::SMALL_VECTOR_SEQ, *f);
        f = create_native_function1<get_small_vector_seq_next>();
        define_method(NEXT, type::SMALL_VECTOR_SEQ, *f);

        derive(type::SMALL_SET, type::SEQABLE);
        f = create_native_function1<small_set_seq>();
        define_method(SEQ, type::SMALL_SET, *f);
        f = create_native_function1<get_small_set_seq_first>();
        define_method(FIRST, type::SMALL_SET_SEQ, *f);
        f = create_native_function1<get_small_set_seq_next>();
        define_method(NEXT, type::SMALL_SET_SEQ, *f);

        derive(type::SMALL_VECTOR_SEQ, type::SEQUENCE);
        derive(type::SMALL_SET_SEQ, type::SEQUENCE);
        derive(type::SEQUENCE, type::SEQABLE);
        f = create_native_function1<identity>();
        define_method(SEQ, type::SEQUENCE, *f);

        define_multimethod(OBJ_CALL, *first_type, nil);

        derive(type::SMALL_SET, type::Callable);
        f = create_native_function2<small_set_get>();
        define_method(OBJ_CALL, type::SMALL_SET, *f);

        derive(type::SMALL_MAP, type::Callable);
        f = create_native_function2<small_map_get>();
        define_method(OBJ_CALL, type::SMALL_MAP, *f);

        define_multimethod(OBJ_EQ, *equal_dispatch, nil);
        define_method(OBJ_EQ, nil, *ret_nil);

        auto define_seq_eq = [&](auto type1, auto type2)
        {
            std::array<Value, 2> two{{type1, type2}};
            Root v{create_small_vector(two.data(), two.size())};
            Root f{create_native_function2<are_seqables_equal>()};
            define_method(OBJ_EQ, *v, *f);
        };

        define_seq_eq(type::SMALL_VECTOR, type::SMALL_VECTOR);
        define_seq_eq(type::SMALL_VECTOR, type::LIST);
        define_seq_eq(type::SMALL_VECTOR, type::SEQUENCE);
        define_seq_eq(type::SEQUENCE, type::SMALL_VECTOR);
        define_seq_eq(type::SEQUENCE, type::LIST);
        define_seq_eq(type::SEQUENCE, type::SEQUENCE);
        define_seq_eq(type::LIST, type::SMALL_VECTOR);
        define_seq_eq(type::LIST, type::LIST);
        define_seq_eq(type::LIST, type::SEQUENCE);

        std::array<Value, 2> two_sets{{type::SMALL_SET, type::SMALL_SET}};
        Root v{create_small_vector(two_sets.data(), two_sets.size())};
        f = create_native_function2<are_small_sets_equal>();
        define_method(OBJ_EQ, *v, *f);

        std::array<Value, 2> two_maps{{type::SMALL_MAP, type::SMALL_MAP}};
        v = create_small_vector(two_maps.data(), two_maps.size());
        f = create_native_function2<are_small_maps_equal>();
        define_method(OBJ_EQ, *v, *f);

        define_multimethod(PR_STR_OBJ, *first_type, nil);
        f = create_native_function1<pr_str_small_vector>();
        define_method(PR_STR_OBJ, type::SMALL_VECTOR, *f);
        f = create_native_function1<pr_str_small_set>();
        define_method(PR_STR_OBJ, type::SMALL_SET, *f);
        f = create_native_function1<pr_str_small_map>();
        define_method(PR_STR_OBJ, type::SMALL_MAP, *f);
        f = create_native_function1<pr_str_seqable>();
        define_method(PR_STR_OBJ, type::SEQABLE, *f);
        f = create_native_function1<pr_str_object>();
        define_method(PR_STR_OBJ, nil, *f);

        f = create_native_function2<add2>();
        define(PLUS, *f);
        f = create_native_function2<sub2>();
        define(MINUS, *f);
        f = create_native_function2<mult2>();
        define(ASTERISK, *f);
        f = create_native_function2<lt2>();
        define(LT, *f);
        f = create_native_function2<are_equal>();
        define(EQ, *f);

        define(RECUR, *recur);

        f = create_native_function1<pr_str_exception>();
        define_method(PR_STR_OBJ, type::Exception, *f);

        define_multimethod(GET_MESSAGE, *first_type, nil);

        derive(type::ReadError, type::Exception);
        f = create_native_function1<new_read_error>();
        define(create_symbol("cleo.core", "ReadError."), *f);
        f = create_native_function1<read_error_message>();
        define_method(GET_MESSAGE, type::ReadError, *f);

        derive(type::UnexpectedEndOfInput, type::ReadError);
        f = create_native_function0<new_unexpected_end_of_input>();
        define(create_symbol("cleo.core", "UnexpectedEndOfInput."), *f);
        f = create_native_function1<unexpected_end_of_input_message>();
        define_method(GET_MESSAGE, type::UnexpectedEndOfInput, *f);

        derive(type::CallError, type::Exception);
        f = create_native_function1<new_call_error>();
        define(create_symbol("cleo.core", "CallError."), *f);
        f = create_native_function1<call_error_message>();
        define_method(GET_MESSAGE, type::CallError, *f);

        derive(type::SymbolNotFound, type::Exception);
        f = create_native_function1<new_symbol_not_found>();
        define(create_symbol("cleo.core", "SymbolNotFound."), *f);
        f = create_native_function1<symbol_not_found_message>();
        define_method(GET_MESSAGE, type::SymbolNotFound, *f);

        derive(type::IllegalArgument, type::Exception);
        f = create_native_function1<new_illegal_argument>();
        define(create_symbol("cleo.core", "IllegalArgument."), *f);
        f = create_native_function1<illegal_argument_message>();
        define_method(GET_MESSAGE, type::IllegalArgument, *f);

        f = create_native_function1<macroexpand1>();
        define(create_symbol("cleo.core", "macroexpand-1"), *f);

        f = create_native_function1<macroexpand>();
        define(create_symbol("cleo.core", "macroexpand"), *f);

        define(create_symbol("cleo.core", "list"), *mk_list);
    }
} initialize;

}

}
