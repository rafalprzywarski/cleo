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
#include "util.hpp"
#include <iostream>

namespace cleo
{

std::vector<void *> allocations;
std::vector<Value> extra_roots;
unsigned gc_frequency = 64;
unsigned gc_counter = gc_frequency - 1;

std::unordered_map<std::string, std::unordered_map<std::string, Value>> symbols;
std::unordered_map<std::string, std::unordered_map<std::string, Value>> keywords;

std::unordered_map<Value, Value, std::hash<Value>, StdIs> vars;

std::unordered_map<Value, Multimethod, std::hash<Value>, StdIs> multimethods;
Hierachy global_hierarchy;

Root current_exception;

const Value TRUE = create_keyword("true");
const Value SEQ = create_symbol("cleo.core", "seq");
const Value FIRST = create_symbol("cleo.core", "first");
const Value NEXT = create_symbol("cleo.core", "next");
const Value CONJ = create_symbol("cleo.core", "conj");
const Value ASSOC = create_symbol("cleo.core", "assoc");
const Value OBJ_EQ = create_symbol("cleo.core", "obj=");
const Value OBJ_CALL = create_symbol("cleo.core", "obj-call");
const Value PRINT_READABLY = create_symbol("cleo.core", "*print-readably*");
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
const Value TRY = create_symbol("try*");
const Value CATCH = create_symbol("catch*");
const Value FINALLY = create_symbol("finally*");
const Value VA = create_symbol("&");
const Value CURRENT_NS = create_symbol("cleo.core", "*ns*");
const Value ENV_NS = create_symbol("cleo.core", "*env-ns*");
const Value IN_NS = create_symbol("cleo.core", "in-ns");
const Value NS = create_symbol("cleo.core", "ns");
const Value LIB_PATH = create_symbol("cleo.core", "*lib-path*");
const Value ATOM = create_symbol("cleo.core", "atom");
const Value DEREF = create_symbol("cleo.core", "deref");
const Value RESET = create_symbol("cleo.core", "reset!");
const Value SWAP = create_symbol("cleo.core", "swap!");
const Value APPLY = create_symbol("cleo.core", "apply*");
const Value FORM = create_symbol("&form");
const Value ENV = create_symbol("&env");
const Value CLEO_CORE = create_symbol("cleo.core");
const Value NEW = create_symbol("cleo.core", "new");
const Root ZERO{create_int64(0)};

const std::unordered_set<Value, std::hash<Value>, StdIs> SPECIAL_SYMBOLS{
    QUOTE,
    SYNTAX_QUOTE,
    FN,
    MACRO,
    DEF,
    LET,
    DO,
    IF,
    LOOP,
    RECUR,
    THROW,
    TRY,
    CATCH,
    FINALLY,
    VA
};

namespace
{

const Value META_TYPE = create_symbol("cleo.core", "MetaType");

Force create_meta_type()
{
    Root obj{create_object1(nil, META_TYPE)};
    set_object_type(*obj, *obj);
    return force(*obj);
}

Value get_type_name(Value type)
{
    return get_object_element(type, 0);
}

}

namespace type
{
const Root MetaType{create_meta_type()};
}

namespace
{

Force create_type(const std::string& ns, const std::string& name)
{
    Root obj{create_object1(*type::MetaType, create_symbol(ns, name))};
    return *obj;
}

}

namespace type
{
const Root Int64{create_type("cleo.core", "Int64")};
const Root Float64{create_type("cleo.core", "Float64")};
const Root String{create_type("cleo.core", "String")};
const Root NativeFunction{create_type("cleo.core", "NativeFunction")};
const Root Symbol{create_type("cleo.core", "Symbol")};
const Root Keyword{create_type("cleo.core", "Keyword")};
const Root Var{create_type("cleo.core", "Var")};
const Root List{create_type("cleo.core", "List")};
const Root SmallVector{create_type("cleo.core", "SmallVector")};
const Root SmallVectorSeq{create_type("cleo.core", "SmallVectorSeq")};
const Root SmallMap{create_type("cleo.core", "SmallMap")};
const Root SmallSet{create_type("cleo.core", "SmallSet")};
const Root SmallSetSeq{create_type("cleo.core", "SmallSetSeq")};
const Root Multimethod{create_type("cleo.core", "Multimethod")};
const Root Seqable{create_type("cleo.core", "Seqable")};
const Root Sequence{create_type("cleo.core", "Sequence")};
const Root Callable{create_type("cleo.core", "Callable")};
const Root Fn{create_type("cleo.core", "Fn")};
const Root Macro{create_type("cleo.core", "Macro")};
const Root Recur{create_type("cleo.core", "Recur")};
const Root Atom{create_type("cleo.core", "Atom")};
const Root Exception{create_type("cleo.core", "Exception")};
const Root ReadError{create_type("cleo.core", "ReadError")};
const Root CallError{create_type("cleo.core", "CallError")};
const Root SymbolNotFound{create_type("cleo.core", "SymbolNotFound")};
const Root IllegalArgument{create_type("cleo.core", "IllegalArgument")};
const Root IllegalState{create_type("cleo.core", "IllegalState")};
const Root UnexpectedEndOfInput{create_type("cleo.core", "UnexpectedEndOfInput")};
const Root FileNotFound{create_type("cleo.core", "FileNotFound")};
const Root ArithmeticException{create_type("cleo.core", "ArithmeticException")};
}

const std::array<Value, 7> type_by_tag{{
    nil,
    *type::NativeFunction,
    *type::Symbol,
    *type::Keyword,
    *type::Int64,
    *type::Float64,
    *type::String
}};

const Root EMPTY_LIST{create_list(nullptr, 0)};
const Root EMPTY_VECTOR{create_small_vector(nullptr, 0)};
const Root EMPTY_SET{create_small_set()};
const Root EMPTY_MAP{create_small_map()};

Root namespaces{*EMPTY_MAP};
Root bindings;

Int64 next_id = 0;

Int64 gen_id()
{
    return next_id++;
}

const Root recur{create_native_function([](const Value *args, std::uint8_t n)
{
    return create_object(*type::Recur, args, n);
})};

namespace rt
{

const DynamicVar current_ns = define_var(CURRENT_NS, nil);
const DynamicVar lib_path = define_var(LIB_PATH, nil);
const StaticVar obj_eq = define_var(OBJ_EQ, nil);
const StaticVar obj_call = define_var(OBJ_CALL, nil);
const DynamicVar print_readably = define_var(PRINT_READABLY, nil);
const StaticVar pr_str_obj = define_var(PR_STR_OBJ, nil);
const StaticVar first = define_var(FIRST, nil);
const StaticVar next = define_var(NEXT, nil);
const StaticVar seq = define_var(SEQ, nil);
const StaticVar get_message = define_var(GET_MESSAGE, nil);

}

namespace
{

const Value MACROEXPAND1 = create_symbol("cleo.core", "macroexpand-1");
const Value MACROEXPAND = create_symbol("cleo.core", "macroexpand");
const Value REFER = create_symbol("cleo.core", "refer");
const Value READ_STRING = create_symbol("cleo.core", "read-string");
const Value LOAD_STRING = create_symbol("cleo.core", "load-string");
const Value REQUIRE = create_symbol("cleo.core", "require");
const Value TYPE = create_symbol("cleo.core", "type");
const Value KEYWORD = get_type_name(*type::Keyword);


const Root first_type{create_native_function([](const Value *args, std::uint8_t num_args) -> Force
{
    return (num_args < 1) ? nil : get_value_type(args[0]);
})};

const Root first_arg{create_native_function([](const Value *args, std::uint8_t num_args) -> Force
{
    return (num_args < 1) ? nil : args[0];
})};

const Root equal_dispatch{create_native_function([](const Value *args, std::uint8_t num_args)
{
    check_arity(OBJ_EQ, 2, num_args);
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

void check_ints(Value l, Value r)
{
    if (get_value_tag(l) != tag::INT64 || get_value_tag(r) != tag::INT64)
    {
        Root msg{create_string("expected Int64")};
        throw_exception(new_illegal_argument(*msg));
    }
}

Force add2(Value l, Value r)
{
    check_ints(l, r);
    std::uint64_t ul{std::uint64_t(get_int64_value(l))}, ur{std::uint64_t(get_int64_value(r))}, ret{ul + ur};
    auto overflow = Int64((ul ^ ret) & (ur ^ ret)) < 0;
    if (overflow)
        throw_integer_overflow();
    return create_int64(Int64(ret));
}

Force sub(const Value *args, std::uint8_t n)
{
    if (n == 0 || n > 2)
        throw_arity_error(MINUS, n);
    Value l{n == 1 ? *ZERO : args[0]}, r{args[n - 1]};
    check_ints(l, r);
    std::uint64_t ul{std::uint64_t(get_int64_value(l))}, ur{std::uint64_t(get_int64_value(r))}, ret{ul - ur};
    auto overflow = Int64((ul ^ ret) & (~ur ^ ret)) < 0;
    if (overflow)
        throw_integer_overflow();
    return create_int64(ret);
}

Force mult2(Value l, Value r)
{
    check_ints(l, r);
    auto lv = get_int64_value(l), rv = get_int64_value(r);
    Int64 ret = Int64(std::uint64_t(lv) * std::uint64_t(rv));
    auto overflow =
        (lv == std::numeric_limits<Int64>::min() && rv < 0) ||
        (rv != 0 && ret / rv != lv);
    if (overflow)
        throw_integer_overflow();
    return create_int64(ret);
}

Force lt2(Value l, Value r)
{
    check_ints(l, r);
    return get_int64_value(l) < get_int64_value(r) ? TRUE : nil;
}

Force pr_str_exception(Value e)
{
    cleo::Root msg{call_multimethod1(*rt::get_message, e)};
    cleo::Root type{cleo::pr_str(cleo::get_value_type(e))};
    msg = print_str(*msg);

    return create_string(
        std::string(get_string_ptr(*type), get_string_len(*type)) + ": " +
        std::string(get_string_ptr(*msg), get_string_len(*msg)));
}

Force create_ns_macro()
{
    Root form{create_string("(macro* ns [ns] `(do (cleo.core/in-ns '~ns) (cleo.core/refer 'cleo.core)))")};
    form = read(*form);
    return eval(*form);
}

Force create_swap_fn()
{
    Root form{create_string("(fn* swap!"
                            " ([a f] (do (cleo.core/reset! a (f @a)) @a))"
                            " ([a f x] (do (cleo.core/reset! a (f @a x)) @a))"
                            " ([a f x y] (do (cleo.core/reset! a (f @a x y)) @a)))")};
    form = read(*form);
    return eval(*form);
}

Force keyword_get(Value k, Value coll)
{
    if (get_value_tag(coll) != tag::OBJECT)
        return nil;
    std::array<Value, 2> args{{coll, k}};
    return call_multimethod(*rt::obj_call, args.data(), args.size());
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

Force pr(const Value *args, std::uint8_t n)
{
    Root s;
    for (decltype(n) i = 0; i < n; ++i)
    {
        s = pr_str(args[i]);
        if (i > 0)
            std::cout << ' ';
        std::cout << std::string(get_string_ptr(*s), get_string_len(*s));
    }
    std::cout << std::flush;
    return force(nil);
}

Force macroexpand_noenv(Value val)
{
    return macroexpand(val, nil);
}

Force macroexpand1_noenv(Value val)
{
    return macroexpand1(val, nil);
}

Force get_seqable_first(Value val)
{
    Root s{call_multimethod1(*rt::seq, val)};
    return call_multimethod1(*rt::first, *s);
}

Force get_seqable_next(Value val)
{
    Root s{call_multimethod1(*rt::seq, val)};
    return call_multimethod1(*rt::next, *s);
}

Force pr_str_type(Value type)
{
    return pr_str(get_object_element(type, 0));
}

void define_type(Value type)
{
    define(get_type_name(type), type);
}

Force pr_str_var(Value var)
{
    check_type("var", var, *type::Var);
    return create_string("#'" + to_string(get_var_name(var)));
}

struct Initialize
{
    Initialize()
    {
        define_type(*type::Int64);
        define_type(*type::Float64);
        define_type(*type::String);
        define_type(*type::NativeFunction);
        define_type(*type::Keyword);
        define_type(*type::Symbol);
        define_type(*type::Var);
        define_type(*type::Seqable);
        define_type(*type::List);
        define_type(*type::SmallVector);
        define_type(*type::SmallVectorSeq);
        define_type(*type::SmallMap);
        define_type(*type::SmallSet);
        define_type(*type::SmallSetSeq);
        define_type(*type::Multimethod);
        define_type(*type::Seqable);
        define_type(*type::Sequence);
        define_type(*type::Callable);
        define_type(*type::Fn);
        define_type(*type::Macro);
        define_type(*type::Recur);
        define_type(*type::Atom);
        define_type(*type::Exception);
        define_type(*type::ReadError);
        define_type(*type::CallError);
        define_type(*type::SymbolNotFound);
        define_type(*type::IllegalArgument);
        define_type(*type::IllegalState);
        define_type(*type::UnexpectedEndOfInput);
        define_type(*type::FileNotFound);
        define_type(*type::ArithmeticException);

        define_multimethod(NEW, *first_arg, nil);

        Root f;

        define(CURRENT_NS, CLEO_CORE);
        f = create_native_function1<in_ns, &IN_NS>();
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

        derive(*type::List, *type::Seqable);
        f = create_native_function1<list_seq>();
        define_method(SEQ, *type::List, *f);
        f = create_native_function1<get_list_first>();
        define_method(FIRST, *type::List, *f);
        f = create_native_function1<get_list_next>();
        define_method(NEXT, *type::List, *f);

        derive(*type::SmallVector, *type::Seqable);
        f = create_native_function1<small_vector_seq>();
        define_method(SEQ, *type::SmallVector, *f);
        f = create_native_function1<get_small_vector_seq_first>();
        define_method(FIRST, *type::SmallVectorSeq, *f);
        f = create_native_function1<get_small_vector_seq_next>();
        define_method(NEXT, *type::SmallVectorSeq, *f);

        derive(*type::SmallSet, *type::Seqable);
        f = create_native_function1<small_set_seq>();
        define_method(SEQ, *type::SmallSet, *f);
        f = create_native_function1<get_small_set_seq_first>();
        define_method(FIRST, *type::SmallSetSeq, *f);
        f = create_native_function1<get_small_set_seq_next>();
        define_method(NEXT, *type::SmallSetSeq, *f);

        derive(*type::SmallVectorSeq, *type::Sequence);
        derive(*type::SmallSetSeq, *type::Sequence);
        derive(*type::Sequence, *type::Seqable);
        f = create_native_function1<identity>();
        define_method(SEQ, *type::Sequence, *f);

        f = create_native_function1<get_seqable_first>();
        define_method(FIRST, *type::Seqable, *f);
        f = create_native_function1<get_seqable_next>();
        define_method(NEXT, *type::Seqable, *f);

        define_multimethod(CONJ, *first_type, undefined);

        f = create_native_function2<small_vector_conj>();
        define_method(CONJ, *type::SmallVector, *f);

        f = create_native_function2<small_set_conj>();
        define_method(CONJ, *type::SmallSet, *f);

        f = create_native_function2<list_conj>();
        define_method(CONJ, *type::List, *f);

        define_multimethod(ASSOC, *first_type, undefined);

        f = create_native_function3<small_map_assoc>();
        define_method(ASSOC, *type::SmallMap, *f);

        define_multimethod(OBJ_CALL, *first_type, nil);

        derive(*type::SmallSet, *type::Callable);
        f = create_native_function2<small_set_get>();
        define_method(OBJ_CALL, *type::SmallSet, *f);

        derive(*type::SmallMap, *type::Callable);
        f = create_native_function2<small_map_get>();
        define_method(OBJ_CALL, *type::SmallMap, *f);

        derive(*type::Keyword, *type::Callable);
        f = create_native_function2<keyword_get, &KEYWORD>();
        define_method(OBJ_CALL, *type::Keyword, *f);

        derive(*type::SmallVector, *type::Callable);
        f = create_native_function2<small_vector_get>();
        define_method(OBJ_CALL, *type::SmallVector, *f);

        define_multimethod(OBJ_EQ, *equal_dispatch, nil);
        define_method(OBJ_EQ, nil, *ret_nil);

        auto define_seq_eq = [&](auto type1, auto type2)
        {
            std::array<Value, 2> two{{type1, type2}};
            Root v{create_small_vector(two.data(), two.size())};
            Root f{create_native_function2<are_seqables_equal>()};
            define_method(OBJ_EQ, *v, *f);
        };

        define_seq_eq(*type::SmallVector, *type::SmallVector);
        define_seq_eq(*type::SmallVector, *type::List);
        define_seq_eq(*type::SmallVector, *type::Sequence);
        define_seq_eq(*type::Sequence, *type::SmallVector);
        define_seq_eq(*type::Sequence, *type::List);
        define_seq_eq(*type::Sequence, *type::Sequence);
        define_seq_eq(*type::List, *type::SmallVector);
        define_seq_eq(*type::List, *type::List);
        define_seq_eq(*type::List, *type::Sequence);

        std::array<Value, 2> two_sets{{*type::SmallSet, *type::SmallSet}};
        Root v{create_small_vector(two_sets.data(), two_sets.size())};
        f = create_native_function2<are_small_sets_equal>();
        define_method(OBJ_EQ, *v, *f);

        std::array<Value, 2> two_maps{{*type::SmallMap, *type::SmallMap}};
        v = create_small_vector(two_maps.data(), two_maps.size());
        f = create_native_function2<are_small_maps_equal>();
        define_method(OBJ_EQ, *v, *f);

        define(PRINT_READABLY, TRUE);

        define_multimethod(PR_STR_OBJ, *first_type, nil);
        f = create_native_function1<pr_str_type>();
        define_method(PR_STR_OBJ, *type::MetaType, *f);

        f = create_native_function1<pr_str_small_vector>();
        define_method(PR_STR_OBJ, *type::SmallVector, *f);
        f = create_native_function1<pr_str_small_set>();
        define_method(PR_STR_OBJ, *type::SmallSet, *f);
        f = create_native_function1<pr_str_small_map>();
        define_method(PR_STR_OBJ, *type::SmallMap, *f);
        f = create_native_function1<pr_str_seqable>();
        define_method(PR_STR_OBJ, *type::Seqable, *f);
        f = create_native_function1<pr_str_object>();
        define_method(PR_STR_OBJ, nil, *f);

        f = create_native_function1<pr_str_var>();
        define_method(PR_STR_OBJ, *type::Var, *f);

        f = create_native_function2<add2, &PLUS>();
        define(PLUS, *f);
        f = create_native_function(sub);
        define(MINUS, *f);
        f = create_native_function2<mult2, &ASTERISK>();
        define(ASTERISK, *f);
        f = create_native_function2<lt2, &LT>();
        define(LT, *f);
        f = create_native_function2<are_equal, &EQ>();
        define(EQ, *f);

        f = create_native_function1<pr_str_exception>();
        define_method(PR_STR_OBJ, *type::Exception, *f);

        define_multimethod(GET_MESSAGE, *first_type, nil);

        derive(*type::ReadError, *type::Exception);
        f = create_native_new3<new_read_error, &NEW>();
        define_method(NEW, *type::ReadError, *f);
        f = create_native_function1<read_error_message>();
        define_method(GET_MESSAGE, *type::ReadError, *f);

        derive(*type::UnexpectedEndOfInput, *type::ReadError);
        f = create_native_new2<new_unexpected_end_of_input, &NEW>();
        define_method(NEW, *type::UnexpectedEndOfInput, *f);
        f = create_native_function1<unexpected_end_of_input_message>();
        define_method(GET_MESSAGE, *type::UnexpectedEndOfInput, *f);

        derive(*type::CallError, *type::Exception);
        f = create_native_new1<new_call_error, &NEW>();
        define_method(NEW, *type::CallError, *f);
        f = create_native_function1<call_error_message>();
        define_method(GET_MESSAGE, *type::CallError, *f);

        derive(*type::SymbolNotFound, *type::Exception);
        f = create_native_new1<new_symbol_not_found, &NEW>();
        define_method(NEW, *type::SymbolNotFound, *f);
        f = create_native_function1<symbol_not_found_message>();
        define_method(GET_MESSAGE, *type::SymbolNotFound, *f);

        derive(*type::IllegalArgument, *type::Exception);
        f = create_native_new1<new_illegal_argument, &NEW>();
        define_method(NEW, *type::IllegalArgument, *f);
        f = create_native_function1<illegal_argument_message>();
        define_method(GET_MESSAGE, *type::IllegalArgument, *f);

        derive(*type::IllegalState, *type::Exception);
        f = create_native_new1<new_illegal_state, &NEW>();
        define_method(NEW, *type::IllegalState, *f);
        f = create_native_function1<illegal_state_message>();
        define_method(GET_MESSAGE, *type::IllegalState, *f);

        derive(*type::FileNotFound, *type::Exception);
        f = create_native_new1<new_file_not_found, &NEW>();
        define_method(NEW, *type::FileNotFound, *f);
        f = create_native_function1<file_not_found_message>();
        define_method(GET_MESSAGE, *type::FileNotFound, *f);

        derive(*type::ArithmeticException, *type::Exception);
        f = create_native_new1<new_arithmetic_exception, &NEW>();
        define_method(NEW, *type::ArithmeticException, *f);
        f = create_native_function1<arithmetic_exceptio_message>();
        define_method(GET_MESSAGE, *type::ArithmeticException, *f);

        f = create_native_function1<macroexpand1_noenv, &MACROEXPAND1>();
        define(MACROEXPAND1, *f);

        f = create_native_function1<macroexpand_noenv, &MACROEXPAND>();
        define(MACROEXPAND, *f);

        f = create_native_function1<refer, &REFER>();
        define(REFER, *f);

        f = create_native_function1<read, &READ_STRING>();
        define(READ_STRING, *f);

        f = create_native_function1<load, &LOAD_STRING>();
        define(LOAD_STRING, *f);

        f = create_native_function1<require, &REQUIRE>();
        define(REQUIRE, *f);

        f = create_native_function1<create_atom, &ATOM>();
        define(ATOM, *f);

        define_multimethod(DEREF, *first_type, nil);
        f = create_native_function1<atom_deref>();
        define_method(DEREF, *type::Atom, *f);

        define_multimethod(RESET, *first_type, nil);
        f = create_native_function2<atom_reset>();
        define_method(RESET, *type::Atom, *f);

        f = create_swap_fn();
        define(SWAP, *f);

        f = create_native_function2<apply, &APPLY>();
        define(APPLY, *f);

        f = create_native_function(pr);
        define(create_symbol("cleo.core", "pr"), *f);

        f = create_native_function1<get_value_type, &TYPE>();
        define(TYPE, *f);
    }
} initialize;

}

}
