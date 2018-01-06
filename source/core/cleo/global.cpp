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
#include "reader.hpp"
#include "atom.hpp"

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
const Value SYNTAX_QUOTE = create_symbol("cleo.core", "syntax-quote");
const Value UNQUOTE = create_symbol("cleo.core", "unquote");
const Value UNQUOTE_SPLICING = create_symbol("cleo.core", "unquote-splicing");
const Value FN = create_symbol("fn*");
const Value MACRO = create_symbol("macro*");
const Value DEF = create_symbol("def");
const Value LET = create_symbol("let*");
const Value DO = create_symbol("do");
const Value IF = create_symbol("if");
const Value LOOP = create_symbol("loop*");
const Value RECUR = create_symbol("recur");
const Value PLUS = create_symbol("cleo.core", "+");
const Value MINUS = create_symbol("cleo.core", "-");
const Value ASTERISK = create_symbol("cleo.core", "*");
const Value LT = create_symbol("cleo.core", "<");
const Value EQ = create_symbol("cleo.core", "=");
const Value THROW = create_symbol("throw");
const Value TRY = create_symbol("try");
const Value CATCH = create_symbol("catch");
const Value FINALLY = create_symbol("finally");
const Value VA = create_symbol("&");
const Value CURRENT_NS = create_symbol("cleo.core", "*ns*");
const Value IN_NS = create_symbol("cleo.core", "in-ns");
const Value NS = create_symbol("cleo.core", "ns");
const Value LIB_PATH = create_symbol("cleo.core", "*lib-path*");
const Value ATOM = create_symbol("cleo.core", "atom");
const Value DEREF = create_symbol("cleo.core", "deref");
const Value RESET = create_symbol("cleo.core", "reset!");
const Value SWAP = create_symbol("cleo.core", "swap!");
const Value APPLY = create_symbol("cleo.core", "apply*");

namespace type
{
const Value NativeFunction = create_symbol("cleo.core", "NativeFunction");
const Value Keyword = create_symbol("cleo.core", "Keyword");
const Value List = create_symbol("cleo.core", "List");
const Value SmallVector = create_symbol("cleo.core", "SmallVector");
const Value SmallVectorSeq = create_symbol("cleo.core", "SmallVectorSeq");
const Value SmallMap = create_symbol("cleo.core", "SmallMap");
const Value SmallSet = create_symbol("cleo.core", "SmallSet");
const Value SmallSetSeq = create_symbol("cleo.core", "SmallSetSeq");
const Value Multimethod = create_symbol("cleo.core", "Multimethod");
const Value Seqable = create_symbol("cleo.core", "Seqable");
const Value Sequence = create_symbol("cleo.core", "Sequence");
const Value Callable = create_symbol("cleo.core", "Callable");
const Value Fn = create_symbol("cleo.core", "Fn");
const Value Macro = create_symbol("cleo.core", "Macro");
const Value Recur = create_symbol("cleo.core", "Recur");
const Value Atom = create_symbol("cleo.core", "Atom");
const Value Exception = create_symbol("cleo.core", "Exception");
const Value ReadError = create_symbol("cleo.core", "ReadError");
const Value CallError = create_symbol("cleo.core", "CallError");
const Value SymbolNotFound = create_symbol("cleo.core", "SymbolNotFound");
const Value IllegalArgument = create_symbol("cleo.core", "IllegalArgument");
const Value IllegalState = create_symbol("cleo.core", "IllegalState");
const Value UnexpectedEndOfInput = create_symbol("cleo.core", "UnexpectedEndOfInput");
const Value FileNotFound = create_symbol("cleo.core", "FileNotFound");
}

const std::array<Value, 7> type_by_tag{{
    nil,
    type::NativeFunction,
    create_symbol("cleo.core", "Symbol"),
    type::Keyword,
    create_symbol("cleo.core", "Int64"),
    create_symbol("cleo.core", "Float64"),
    create_symbol("cleo.core", "String")
}};

const Root EMPTY_LIST{create_list(nullptr, 0)};
const Root EMPTY_VECTOR{create_small_vector(nullptr, 0)};
const Root EMPTY_MAP{create_small_map()};

Root namespaces{*EMPTY_MAP};
Root bindings;

const Root recur{create_native_function([](const Value *args, std::uint8_t n)
{
    return create_object(type::Recur, args, n);
})};

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

Force pr_str_exception(Value e)
{
    auto get_message = lookup_var(GET_MESSAGE);
    cleo::Root msg{call_multimethod1(get_message, e)};
    cleo::Root type{cleo::pr_str(cleo::get_value_type(e))};

    return create_string(
        std::string(get_string_ptr(*type), get_string_len(*type)) + ": " +
        std::string(get_string_ptr(*msg), get_string_len(*msg)));
}

Force create_ns_macro()
{
    Root form{create_string(
        "(macro* ns [ns] (let* [list (fn* [& xs] xs)]"
        "                 (list 'do"
        "                  (list 'cleo.core/in-ns (list 'quote ns))"
        "                  (list 'cleo.core/refer ''cleo.core))))")};
    form = read(*form);
    return eval(*form);
}

Force create_swap_fn()
{
    Root form{create_string("(fn* swap!"
                            " ([a f] (do (reset! a (f (deref a))) (deref a)))"
                            " ([a f x] (do (reset! a (f (deref a) x)) (deref a)))"
                            " ([a f x y] (do (reset! a (f (deref a) x y)) (deref a))))")};
    form = read(*form);
    return eval(*form);
}

Force keyword_get(Value k, Value coll)
{
    std::array<Value, 2> args{{coll, k}};
    return call_multimethod(lookup(OBJ_CALL), args.data(), args.size());
}

Value small_vector_get(Value v, Value index)
{
    if (get_value_tag(index) != tag::INT64)
        return nil;
    auto i = get_int64_value(index);
    if (i < 0 || i >= get_small_vector_size(v))
        return nil;
    return get_small_vector_elem(v, i);
}

struct Initialize
{
    Initialize()
    {
        Root f;

        define(CURRENT_NS, create_symbol("cleo.core"));
        f = create_native_function1<in_ns>();
        define(IN_NS, *f);
        f = create_ns_macro();
        define(NS, *f);

        f = create_string(".");
        define(LIB_PATH, *f);

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

        derive(type::List, type::Seqable);
        f = create_native_function1<list_seq>();
        define_method(SEQ, type::List, *f);
        f = create_native_function1<get_list_first>();
        define_method(FIRST, type::List, *f);
        f = create_native_function1<get_list_next>();
        define_method(NEXT, type::List, *f);

        derive(type::SmallVector, type::Seqable);
        f = create_native_function1<small_vector_seq>();
        define_method(SEQ, type::SmallVector, *f);
        f = create_native_function1<get_small_vector_seq_first>();
        define_method(FIRST, type::SmallVectorSeq, *f);
        f = create_native_function1<get_small_vector_seq_next>();
        define_method(NEXT, type::SmallVectorSeq, *f);

        derive(type::SmallSet, type::Seqable);
        f = create_native_function1<small_set_seq>();
        define_method(SEQ, type::SmallSet, *f);
        f = create_native_function1<get_small_set_seq_first>();
        define_method(FIRST, type::SmallSetSeq, *f);
        f = create_native_function1<get_small_set_seq_next>();
        define_method(NEXT, type::SmallSetSeq, *f);

        derive(type::SmallVectorSeq, type::Sequence);
        derive(type::SmallSetSeq, type::Sequence);
        derive(type::Sequence, type::Seqable);
        f = create_native_function1<identity>();
        define_method(SEQ, type::Sequence, *f);

        define_multimethod(OBJ_CALL, *first_type, nil);

        derive(type::SmallSet, type::Callable);
        f = create_native_function2<small_set_get>();
        define_method(OBJ_CALL, type::SmallSet, *f);

        derive(type::SmallMap, type::Callable);
        f = create_native_function2<small_map_get>();
        define_method(OBJ_CALL, type::SmallMap, *f);

        derive(type::Keyword, type::Callable);
        f = create_native_function2<keyword_get>();
        define_method(OBJ_CALL, type::Keyword, *f);

        derive(type::SmallVector, type::Callable);
        f = create_native_function2<small_vector_get>();
        define_method(OBJ_CALL, type::SmallVector, *f);

        define_multimethod(OBJ_EQ, *equal_dispatch, nil);
        define_method(OBJ_EQ, nil, *ret_nil);

        auto define_seq_eq = [&](auto type1, auto type2)
        {
            std::array<Value, 2> two{{type1, type2}};
            Root v{create_small_vector(two.data(), two.size())};
            Root f{create_native_function2<are_seqables_equal>()};
            define_method(OBJ_EQ, *v, *f);
        };

        define_seq_eq(type::SmallVector, type::SmallVector);
        define_seq_eq(type::SmallVector, type::List);
        define_seq_eq(type::SmallVector, type::Sequence);
        define_seq_eq(type::Sequence, type::SmallVector);
        define_seq_eq(type::Sequence, type::List);
        define_seq_eq(type::Sequence, type::Sequence);
        define_seq_eq(type::List, type::SmallVector);
        define_seq_eq(type::List, type::List);
        define_seq_eq(type::List, type::Sequence);

        std::array<Value, 2> two_sets{{type::SmallSet, type::SmallSet}};
        Root v{create_small_vector(two_sets.data(), two_sets.size())};
        f = create_native_function2<are_small_sets_equal>();
        define_method(OBJ_EQ, *v, *f);

        std::array<Value, 2> two_maps{{type::SmallMap, type::SmallMap}};
        v = create_small_vector(two_maps.data(), two_maps.size());
        f = create_native_function2<are_small_maps_equal>();
        define_method(OBJ_EQ, *v, *f);

        define_multimethod(PR_STR_OBJ, *first_type, nil);
        f = create_native_function1<pr_str_small_vector>();
        define_method(PR_STR_OBJ, type::SmallVector, *f);
        f = create_native_function1<pr_str_small_set>();
        define_method(PR_STR_OBJ, type::SmallSet, *f);
        f = create_native_function1<pr_str_small_map>();
        define_method(PR_STR_OBJ, type::SmallMap, *f);
        f = create_native_function1<pr_str_seqable>();
        define_method(PR_STR_OBJ, type::Seqable, *f);
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

        derive(type::IllegalState, type::Exception);
        f = create_native_function1<new_illegal_state>();
        define(create_symbol("cleo.core", "IllegalState."), *f);
        f = create_native_function1<illegal_state_message>();
        define_method(GET_MESSAGE, type::IllegalState, *f);

        derive(type::FileNotFound, type::Exception);
        f = create_native_function1<new_file_not_found>();
        define(create_symbol("cleo.core", "FileNotFound."), *f);
        f = create_native_function1<file_not_found_message>();
        define_method(GET_MESSAGE, type::FileNotFound, *f);

        f = create_native_function1<macroexpand1>();
        define(create_symbol("cleo.core", "macroexpand-1"), *f);

        f = create_native_function1<macroexpand>();
        define(create_symbol("cleo.core", "macroexpand"), *f);

        f = create_native_function1<refer>();
        define(create_symbol("cleo.core", "refer"), *f);

        f = create_native_function1<read>();
        define(create_symbol("cleo.core", "read-string"), *f);

        f = create_native_function1<load>();
        define(create_symbol("cleo.core", "load-string"), *f);

        f = create_native_function1<require>();
        define(create_symbol("cleo.core", "require"), *f);

        f = create_native_function1<create_atom>();
        define(ATOM, *f);

        define_multimethod(DEREF, *first_type, nil);
        f = create_native_function1<atom_deref>();
        define_method(DEREF, type::Atom, *f);

        define_multimethod(RESET, *first_type, nil);
        f = create_native_function2<atom_reset>();
        define_method(RESET, type::Atom, *f);

        f = create_swap_fn();
        define(SWAP, *f);

        f = create_native_function2<apply>();
        define(APPLY, *f);
    }
} initialize;

}

}
