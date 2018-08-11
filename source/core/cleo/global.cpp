#include "global.hpp"
#include "array.hpp"
#include "multimethod.hpp"
#include "equality.hpp"
#include "list.hpp"
#include "cons.hpp"
#include "lazy_seq.hpp"
#include "print.hpp"
#include "var.hpp"
#include "error.hpp"
#include "eval.hpp"
#include "array_set.hpp"
#include "array_map.hpp"
#include "persistent_hash_map.hpp"
#include "namespace.hpp"
#include "reader.hpp"
#include "atom.hpp"
#include "util.hpp"
#include "clib.hpp"
#include "fn_call.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <limits>
#include <chrono>

namespace cleo
{

std::vector<Allocation> allocations;
std::vector<Value> extra_roots;
unsigned gc_frequency = 4096;
unsigned gc_counter = gc_frequency - 1;
std::unique_ptr<std::ostream> gc_log;

vm::Stack stack;

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
const Value COUNT = create_symbol("cleo.core", "count");
const Value GET = create_symbol("cleo.core", "get");
const Value CONTAINS = create_symbol("cleo.core", "contains?");
const Value CONJ = create_symbol("cleo.core", "conj*");
const Value ASSOC = create_symbol("cleo.core", "assoc*");
const Value DISSOC = create_symbol("cleo.core", "dissoc*");
const Value MERGE = create_symbol("cleo.core", "merge");
const Value ARRAY_MAP = create_symbol("cleo.core", "array-map");
const Value OBJ_EQ = create_symbol("cleo.core", "obj=");
const Value OBJ_CALL = create_symbol("cleo.core", "obj-call");
const Value PRINT_READABLY = create_symbol("cleo.core", "*print-readably*");
const Value PR_STR = create_symbol("cleo.core", "pr-str");
const Value PR_STR_OBJ = create_symbol("cleo.core", "pr-str-obj");
const Value STR = create_symbol("cleo.core", "str");
const Value GET_MESSAGE = create_symbol("cleo.core", "get-message");
const Value QUOTE = create_symbol("quote");
const Value UNQUOTE = create_symbol("cleo.core", "unquote");
const Value UNQUOTE_SPLICING = create_symbol("cleo.core", "unquote-splicing");
const Value FN = create_symbol("fn*");
const Value DEF = create_symbol("def");
const Value LET = create_symbol("let*");
const Value DO = create_symbol("do");
const Value IF = create_symbol("if");
const Value LOOP = create_symbol("loop*");
const Value RECUR = create_symbol("recur");
const Value APPLY_SPECIAL = create_symbol("apply*");
const Value PLUS = create_symbol("cleo.core", "+");
const Value MINUS = create_symbol("cleo.core", "-");
const Value ASTERISK = create_symbol("cleo.core", "*");
const Value IDENTICAL = create_symbol("cleo.core", "identical?");
const Value ISA = create_symbol("cleo.core", "isa?");
const Value SYMBOL_Q = create_symbol("cleo.core", "symbol?");
const Value KEYWORD_Q = create_symbol("cleo.core", "keyword?");
const Value VECTOR_Q = create_symbol("cleo.core", "vector?");
const Value LT = create_symbol("cleo.core", "<");
const Value EQ = create_symbol("cleo.core", "=");
const Value THROW = create_symbol("throw");
const Value TRY = create_symbol("try*");
const Value CATCH = create_symbol("catch*");
const Value FINALLY = create_symbol("finally*");
const Value VA = create_symbol("&");
const Value CURRENT_NS = create_symbol("cleo.core", "*ns*");
const Value IN_NS = create_symbol("cleo.core", "in-ns");
const Value NS = create_symbol("cleo.core", "ns");
const Value LIB_PATHS = create_symbol("cleo.core", "*lib-paths*");
const Value ATOM = create_symbol("cleo.core", "atom");
const Value DEREF = create_symbol("cleo.core", "deref");
const Value RESET = create_symbol("cleo.core", "reset!");
const Value SWAP = create_symbol("cleo.core", "swap!");
const Value APPLY = create_symbol("cleo.core", "apply*");
const Value FORM = create_symbol("&form");
const Value ENV = create_symbol("&env");
const Value CLEO_CORE = create_symbol("cleo.core");
const Value NEW = create_symbol("cleo.core", "new");
const Value HASH_OBJ = create_symbol("cleo.core", "hash-obj");
const Value IMPORT_C_FN = create_symbol("cleo.core", "import-c-fn");
const Value COMMAND_LINE_ARGS = create_symbol("cleo.core", "*command-line-args*");
const Value LIST = create_symbol("cleo.core", "list");
const Value CONS = create_symbol("cleo.core", "cons");
const Value LAZY_SEQ = create_symbol("cleo.core", "lazy-seq*");
const Value VECTOR = create_symbol("cleo.core", "vector");
const Value HASH_MAP = create_symbol("cleo.core", "hash-map");
const Value HASH_SET = create_symbol("cleo.core", "hash-set");
const Value CONCATI = create_symbol("cleo.core", "concati");
const Value MACRO_KEY = create_keyword("macro");

const Root ZERO{create_int64(0)};
const Root ONE{create_int64(1)};
const Root NEG_ONE{create_int64(-1)};
const Root TWO{create_int64(2)};
const Root THREE{create_int64(3)};
const Root SENTINEL{create_object0(nil)};

const std::unordered_set<Value, std::hash<Value>, StdIs> SPECIAL_SYMBOLS{
    QUOTE,
    FN,
    DEF,
    LET,
    DO,
    IF,
    LOOP,
    RECUR,
    APPLY_SPECIAL,
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

Force create_type(const std::string& ns, const std::string& name)
{
    Root obj{create_object1(*type::MetaType, create_symbol(ns, name))};
    return *obj;
}

namespace type
{
const Root Int64{create_type("cleo.core", "Int64")};
const Root Float64{create_type("cleo.core", "Float64")};
const Root String{create_type("cleo.core", "String")};
const Root NativeFunction{create_type("cleo.core", "NativeFunction")};
const Root CFunction{create_type("cleo.core", "CFunction")};
const Root Symbol{create_type("cleo.core", "Symbol")};
const Root Keyword{create_type("cleo.core", "Keyword")};
const Root Var{create_type("cleo.core", "Var")};
const Root List{create_type("cleo.core", "List")};
const Root Cons{create_type("cleo.core", "Cons")};
const Root LazySeq{create_type("cleo.core", "LazySeq")};
const Root Array{create_type("cleo.core", "Array")};
const Root TransientArray{create_type("cleo.core", "TransientArray")};
const Root ArraySeq{create_type("cleo.core", "ArraySeq")};
const Root ArrayMap{create_type("cleo.core", "ArrayMap")};
const Root ArrayMapSeq{create_type("cleo.core", "ArrayMapSeq")};
const Root ArraySet{create_type("cleo.core", "ArraySet")};
const Root ArraySetSeq{create_type("cleo.core", "ArraySetSeq")};
const Root Multimethod{create_type("cleo.core", "Multimethod")};
const Root Seqable{create_type("cleo.core", "Seqable")};
const Root Sequence{create_type("cleo.core", "Sequence")};
const Root Callable{create_type("cleo.core", "Callable")};
const Root Fn{create_type("cleo.core", "Fn")};
const Root BytecodeFn{create_type("cleo.core", "BytecodeFn")};
const Root BytecodeFnBody{create_type("cleo.core", "BytecodeFnBody")};
const Root BytecodeFnExceptionTable{create_type("cleo.core", "BytecodeFnExceptionTable")};
const Root Recur{create_type("cleo.core", "Recur")};
const Root Atom{create_type("cleo.core", "Atom")};
const Root PersistentMap{create_type("cleo.core", "PersistentMap")};
const Root PersistentHashMap{create_type("cleo.core", "PersistentHashMap")};
const Root PersistentHashMapSeq{create_type("cleo.core", "PersistentHashMapSeq")};
const Root PersistentHashMapSeqParent{create_type("cleo.core", "PersistentHashMapSeqParent")};
const Root PersistentHashMapCollisionNode(create_type("cleo.core", "PersistentHashMapCollisionNode"));
const Root PersistentHashMapArrayNode(create_type("cleo.core", "PersistentHashMapArrayNode"));
const Root Exception{create_type("cleo.core", "Exception")};
const Root ReadError{create_type("cleo.core", "ReadError")};
const Root CallError{create_type("cleo.core", "CallError")};
const Root SymbolNotFound{create_type("cleo.core", "SymbolNotFound")};
const Root IllegalArgument{create_type("cleo.core", "IllegalArgument")};
const Root IllegalState{create_type("cleo.core", "IllegalState")};
const Root UnexpectedEndOfInput{create_type("cleo.core", "UnexpectedEndOfInput")};
const Root FileNotFound{create_type("cleo.core", "FileNotFound")};
const Root ArithmeticException{create_type("cleo.core", "ArithmeticException")};
const Root IndexOutOfBounds{create_type("cleo.core", "IndexOutOfBounds")};
const Root CompilationError{create_type("cleo.core", "CompilationError")};
const Root Namespace{create_type("cleo.core", "Namespace")};

const Root VarValueRef{create_type("cleo.core.internal", "VarValueRef")};
const Root FnCall{create_type("cleo.core.internal", "FnCall")};
}

namespace clib
{
const Value int64 = create_keyword("int64");
const Value string = create_keyword("string");
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
const Root EMPTY_VECTOR{create_array(nullptr, 0)};
const Root EMPTY_SET{create_array_set()};
const Root EMPTY_MAP{create_persistent_hash_map()};

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

const Root transient_array{create_native_function1<cleo::transient_array>()};
const Root transient_array_conj{create_native_function2<cleo::transient_array_conj>()};
const Root transient_array_persistent{create_native_function1<cleo::transient_array_persistent>()};
const Root array_set_conj{create_native_function2<cleo::array_set_conj>()};
const Root persistent_hash_map_assoc{create_native_function3<cleo::persistent_hash_map_assoc>()};

const DynamicVar current_ns = define_var(CURRENT_NS, nil);
const DynamicVar lib_paths = define_var(LIB_PATHS, nil);
const StaticVar obj_eq = define_var(OBJ_EQ, nil);
const StaticVar obj_call = define_var(OBJ_CALL, nil);
const DynamicVar print_readably = define_var(PRINT_READABLY, nil);
const StaticVar pr_str_obj = define_var(PR_STR_OBJ, nil);
const StaticVar first = define_var(FIRST, nil);
const StaticVar next = define_var(NEXT, nil);
const StaticVar seq = define_var(SEQ, nil);
const StaticVar count = define_var(COUNT, nil);
const StaticVar get = define_var(GET, nil);
const StaticVar contains = define_var(CONTAINS, nil);
const StaticVar assoc = define_var(ASSOC, nil);
const StaticVar merge = define_var(MERGE, nil);
const StaticVar get_message = define_var(GET_MESSAGE, nil);
const StaticVar hash_obj = define_var(HASH_OBJ, nil);

}

namespace
{

const Value MACROEXPAND1 = create_symbol("cleo.core", "macroexpand-1");
const Value MACROEXPAND = create_symbol("cleo.core", "macroexpand");
const Value REFER = create_symbol("cleo.core", "refer");
const Value READ_STRING = create_symbol("cleo.core", "read-string");
const Value LOAD_STRING = create_symbol("cleo.core", "load-string");
const Value REQUIRE = create_symbol("cleo.core", "require");
const Value ALIAS = create_symbol("cleo.core", "alias");
const Value TYPE = create_symbol("cleo.core", "type");
const Value KEYWORD_TYPE = get_type_name(*type::Keyword);
const Value GENSYM = create_symbol("cleo.core", "gensym");
const Value MEMUSED = create_symbol("cleo.core", "mem-used");
const Value MEMALLOCS = create_symbol("cleo.core", "mem-allocs");
const Value BITNOT = create_symbol("cleo.core", "bit-not");
const Value BITAND = create_symbol("cleo.core", "bit-and*");
const Value BITOR = create_symbol("cleo.core", "bit-or*");
const Value BITXOR = create_symbol("cleo.core", "bit-xor*");
const Value BITANDNOT = create_symbol("cleo.core", "bit-and-not*");
const Value BITCLEAR = create_symbol("cleo.core", "bit-clear");
const Value BITSET = create_symbol("cleo.core", "bit-set");
const Value BITFLIP = create_symbol("cleo.core", "bit-flip");
const Value BITTEST = create_symbol("cleo.core", "bit-test");
const Value BITSHIFTLEFT = create_symbol("cleo.core", "bit-shift-left");
const Value BITSHIFTRIGHT = create_symbol("cleo.core", "bit-shift-right");
const Value UNSIGNEDBITSHIFTRIGHT = create_symbol("cleo.core", "unsigned-bit-shift-right");
const Value MAP_Q = create_symbol("cleo.core", "map?");
const Value KEYWORD = create_symbol("cleo.core", "keyword");
const Value NAME = create_symbol("cleo.core", "name");
const Value SYMBOL = create_symbol("cleo.core", "symbol");
const Value NS_MAP = create_symbol("cleo.core", "ns-map");
const Value NS_NAME = create_symbol("cleo.core", "ns-name");
const Value NS_ALIASES = create_symbol("cleo.core", "ns-aliases");
const Value SLASH = create_symbol("cleo.core", "/");
const Value GC_LOG = create_symbol("cleo.core", "gc-log");
const Value GET_TIME = create_symbol("cleo.core", "get-time");
const Value TRANSIENT = create_symbol("cleo.core", "transient");
const Value PERSISTENT = create_symbol("cleo.core", "persistent!");
const Value CONJ_E = create_symbol("cleo.core", "conj!");

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
    return create_array(types.data(), types.size());
})};

const Root ret_nil{create_native_function([](const Value *, std::uint8_t)
{
    return force(nil);
})};

const Root ret_zero{create_native_function([](const Value *, std::uint8_t)
{
    return force(*ZERO);
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

Force div2(Value l, Value r)
{
    check_ints(l, r);
    auto lv = get_int64_value(l), rv = get_int64_value(r);
    if (rv == 0)
    {
        Root s{create_string("Divide by zero")};
        throw_exception(new_arithmetic_exception(*s));
    }
    return create_int64(lv / rv);
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
    Root form{create_string("(fn* ns [&form &env ns] `(do (cleo.core/in-ns '~ns) (cleo.core/refer '~'cleo.core)))")};
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
    if (coll.is_nil() || get_value_tag(coll) != tag::OBJECT)
        return nil;
    std::array<Value, 2> args{{coll, k}};
    return call_multimethod(*rt::obj_call, args.data(), args.size());
}

Value array_get(Value v, Value index)
{
    if (get_value_tag(index) != tag::INT64)
        return nil;
    auto i = get_int64_value(index);
    if (i < 0 || i >= get_array_size(v))
        return nil;
    return get_array_elem(v, i);
}

Value transient_array_get(Value v, Value index)
{
    if (get_value_tag(index) != tag::INT64)
        return nil;
    auto i = get_int64_value(index);
    if (i < 0 || i >= get_int64_value(get_transient_array_size(v)))
        return nil;
    return get_transient_array_elem(v, i);
}

Value array_call(Value v, Value index)
{
    if (get_value_tag(index) != tag::INT64)
        throw_illegal_argument("Key must be integer");
    auto i = get_int64_value(index);
    if (i < 0 || i >= get_array_size(v))
        throw_index_out_of_bounds();
    return get_array_elem(v, i);
}

Value transient_array_call(Value v, Value index)
{
    if (get_value_tag(index) != tag::INT64)
        throw_illegal_argument("Key must be integer");
    auto i = get_int64_value(index);
    if (i < 0 || i >= get_int64_value(get_transient_array_size(v)))
        throw_index_out_of_bounds();
    return get_transient_array_elem(v, i);
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

Force prn(const Value *args, std::uint8_t n)
{
    pr(args, n);
    std::cout << std::endl;
    return nil;
}

Force print(const Value *args, std::uint8_t n)
{
    Root s;
    for (decltype(n) i = 0; i < n; ++i)
    {
        s = print_str(args[i]);
        if (i > 0)
            std::cout << ' ';
        std::cout << std::string(get_string_ptr(*s), get_string_len(*s));
    }
    std::cout << std::flush;
    return nil;
}

Force println(const Value *args, std::uint8_t n)
{
    print(args, n);
    std::cout << std::endl;
    return nil;
}

Force str(const Value *args, std::uint8_t n)
{
    Root s;
    std::string str;
    for (decltype(n) i = 0; i < n; ++i)
    {
        s = print_str(args[i]);
        str.append(get_string_ptr(*s), get_string_len(*s));
    }
    return create_string(str);
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

Force merge_maps(Value m1, Value m2)
{
    Root m{m1}, kv;
    for (Root seq{call_multimethod1(*rt::seq, m2)}; *seq; seq = call_multimethod1(*rt::next, *seq))
    {
        kv = call_multimethod1(*rt::first, *seq);
        m = map_assoc(*m, get_array_elem(*kv, 0), get_array_elem(*kv, 1));
    }
    return *m;
}

Value identical(Value x, Value y)
{
    return x.is(y) ? TRUE : nil;
}

Value symbol_q(Value x)
{
    return get_value_tag(x) == tag::SYMBOL ? TRUE : nil;
}

Value keyword_q(Value x)
{
    return get_value_tag(x) == tag::KEYWORD ? TRUE : nil;
}

Value vector_q(Value x)
{
    return get_value_type(x) == *type::Array ? TRUE : nil;
}

Value map_q(Value x)
{
    auto vt = get_value_type(x);
    return vt == *type::PersistentHashMap || vt == *type::ArrayMap ? TRUE : nil;
}

Force list(const Value *args, std::uint8_t n)
{
    return create_list(args, n);
}

Force vector(const Value *args, std::uint8_t n)
{
    return create_array(args, n);
}

Force hash_map(const Value *args, std::uint8_t n)
{
    Root m{*EMPTY_MAP};
    if (n % 2 == 1)
        throw_illegal_argument("No value supplied for key: " + to_string(args[n - 1]));
    for (std::uint8_t i = 0; i < n; i += 2)
        m = map_assoc(*m, args[i], args[i + 1]);
    return *m;
}

Force hash_set(const Value *args, std::uint8_t n)
{
    Root s{*EMPTY_SET};
    for (std::uint8_t i = 0; i < n; ++i)
        s = array_set_conj(*s, args[i]);
    return *s;
}

Force concati(const Value *args, std::uint8_t n)
{
    Root v{*EMPTY_VECTOR}, val;
    for (std::uint8_t i = 0; i < n; ++i)
        for (Root s{call_multimethod1(*rt::seq, args[i])}; *s; s = call_multimethod1(*rt::next, *s))
        {
            val = call_multimethod1(*rt::first, *s);
            v = array_conj(*v, *val);
        }
    return *v;
}

Force gensym(const Value *args, std::uint8_t n)
{
    if (n > 1)
        throw_arity_error(GENSYM, n);
    std::ostringstream os;
    Root prefix{n > 0 ? print_str(args[0]) : nil};
    if (*prefix)
        os.write(get_string_ptr(*prefix), get_string_len(*prefix));
    else
        os << "G__";
    os << gen_id();
    return create_symbol(os.str());
}

Force mem_used()
{
    return create_int64(get_mem_used());
}

Force mem_allocs()
{
    return create_int64(get_mem_allocations());
}

Force bit_not(Value x)
{
    check_type("x", x, *type::Int64);
    return create_int64(~get_int64_value(x));
}

Force bit_and(Value x, Value y)
{
    check_type("x", x, *type::Int64);
    check_type("y", y, *type::Int64);
    return create_int64(get_int64_value(x) & get_int64_value(y));
}

Force bit_or(Value x, Value y)
{
    check_type("x", x, *type::Int64);
    check_type("y", y, *type::Int64);
    return create_int64(get_int64_value(x) | get_int64_value(y));
}

Force bit_xor(Value x, Value y)
{
    check_type("x", x, *type::Int64);
    check_type("y", y, *type::Int64);
    return create_int64(get_int64_value(x) ^ get_int64_value(y));
}

Force bit_and_not(Value x, Value y)
{
    check_type("x", x, *type::Int64);
    check_type("y", y, *type::Int64);
    return create_int64(get_int64_value(x) & ~get_int64_value(y));
}

Force bit_clear(Value x, Value n)
{
    check_type("x", x, *type::Int64);
    check_type("n", n, *type::Int64);
    return create_int64(get_int64_value(x) & ~(Int64(1) << (get_int64_value(n) & 0x2f)));
}

Force bit_set(Value x, Value n)
{
    check_type("x", x, *type::Int64);
    check_type("n", n, *type::Int64);
    return create_int64(get_int64_value(x) | (Int64(1) << (get_int64_value(n) & 0x2f)));
}

Force bit_flip(Value x, Value n)
{
    check_type("x", x, *type::Int64);
    check_type("n", n, *type::Int64);
    return create_int64(get_int64_value(x) ^ (Int64(1) << (get_int64_value(n) & 0x2f)));
}

Value bit_test(Value x, Value n)
{
    check_type("x", x, *type::Int64);
    check_type("n", n, *type::Int64);
    return (get_int64_value(x) & (Int64(1) << (get_int64_value(n) & 0x2f))) ? TRUE : nil;
}

Force bit_shift_left(Value x, Value n)
{
    check_type("x", x, *type::Int64);
    check_type("n", n, *type::Int64);
    return create_int64(std::uint64_t(get_int64_value(x)) << (get_int64_value(n) & 0x2f));
}

Force bit_shift_right(Value x, Value n)
{
    check_type("x", x, *type::Int64);
    check_type("n", n, *type::Int64);
    static_assert(Int64(-2) >> 1 == Int64(-1), "arithmetic right shift needed");
    return create_int64(get_int64_value(x) >> (get_int64_value(n) & 0x2f));
}

Force unsigned_bit_shift_right(Value x, Value n)
{
    check_type("x", x, *type::Int64);
    check_type("n", n, *type::Int64);
    return create_int64(std::uint64_t(get_int64_value(x)) >> (get_int64_value(n) & 0x2f));
}

Force nil_conj(Value, Value x)
{
    return create_list(&x, 1);
}

Force nil_assoc(Value, Value k, Value v)
{
    return persistent_hash_map_assoc(*EMPTY_MAP, k, v);
}

Value nil_dissoc(Value, Value)
{
    return nil;
}

Value nil_get(Value, Value)
{
    return nil;
}

Value nil_get(Value, Value, Value default_val)
{
    return default_val;
}

Value nil_contains(Value, Value)
{
    return nil;
}

Value nil_count(Value)
{
    return *ZERO;
}

Force seq_count(Value s)
{
    Root sn{call_multimethod1(*rt::seq, s)};
    Int64 n = 0;
    for (; *sn; sn = call_multimethod1(*rt::next, *sn))
        ++n;
    return create_int64(n);
}

Force cons_size(Value c)
{
    Root nc{call_multimethod1(*rt::count, cons_next(c))};
    return create_int64(1 + get_int64_value(*nc));
}

Force cons(Value elem, Value next)
{
    Root s{call_multimethod1(*rt::seq, next)};
    return create_cons(elem, *s);
}

Force mk_keyword(Value val)
{
    auto t = get_value_tag(val);
    switch (t)
    {
        case tag::KEYWORD: return val;
        case tag::SYMBOL:
        {
            auto name = get_symbol_name(val);
            return create_keyword(std::string(get_string_ptr(name), get_string_len(name)));
        }
        case tag::STRING: return create_keyword(std::string(get_string_ptr(val), get_string_len(val)));
        default: return nil;
    }
}

Force mk_symbol(Value ns, Value name)
{
    check_type("ns", ns, *type::String);
    check_type("name", name, *type::String);
    return create_symbol(
        std::string(get_string_ptr(ns), get_string_len(ns)),
        std::string(get_string_ptr(name), get_string_len(name)));
}

Force get_name(Value val)
{
    auto t = get_value_tag(val);
    switch (t)
    {
        case tag::KEYWORD: return get_keyword_name(val);
        case tag::SYMBOL: return get_symbol_name(val);
        case tag::STRING: return val;
        default: return nil;
    }
}

Value set_gc_log(Value set)
{
    if (static_cast<bool>(set) == static_cast<bool>(gc_log))
        return nil;
    gc_log.reset(set ? new std::ofstream("cleo_gc.log", std::ios::app) : nullptr);
    return nil;
}

Force get_time()
{
    using namespace std::chrono;
    return create_int64(duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count());
}

template <std::uint32_t f(Value)>
struct WrapUInt32Fn
{
    static Force fn(Value arg0)
    {
        return create_int64(f(arg0));
    }
};

template <Int64 f(Value)>
struct WrapInt64Fn
{
    static Force fn(Value arg0)
    {
        return create_int64(f(arg0));
    }
};

void define_function(Value name, Force f)
{
    Root fr{f};
    define(name, *fr);
}

struct Initialize
{
    Initialize()
    {
        define_type(*type::Int64);
        define_type(*type::Float64);
        define_type(*type::String);
        define_type(*type::NativeFunction);
        define_type(*type::CFunction);
        define_type(*type::Keyword);
        define_type(*type::Symbol);
        define_type(*type::Var);
        define_type(*type::Seqable);
        define_type(*type::List);
        define_type(*type::Cons);
        define_type(*type::LazySeq);
        define_type(*type::Array);
        define_type(*type::ArraySeq);
        define_type(*type::ArrayMap);
        define_type(*type::ArrayMapSeq);
        define_type(*type::ArraySet);
        define_type(*type::ArraySetSeq);
        define_type(*type::Multimethod);
        define_type(*type::Seqable);
        define_type(*type::Sequence);
        define_type(*type::Callable);
        define_type(*type::Fn);
        define_type(*type::BytecodeFn);
        define_type(*type::BytecodeFnBody);
        define_type(*type::BytecodeFnExceptionTable);
        define_type(*type::Recur);
        define_type(*type::Atom);
        define_type(*type::PersistentMap);
        define_type(*type::PersistentHashMap);
        define_type(*type::PersistentHashMapCollisionNode);
        define_type(*type::PersistentHashMapArrayNode);
        define_type(*type::PersistentHashMapSeqParent);
        define_type(*type::Exception);
        define_type(*type::ReadError);
        define_type(*type::CallError);
        define_type(*type::SymbolNotFound);
        define_type(*type::IllegalArgument);
        define_type(*type::IllegalState);
        define_type(*type::UnexpectedEndOfInput);
        define_type(*type::FileNotFound);
        define_type(*type::ArithmeticException);
        define_type(*type::IndexOutOfBounds);
        define_type(*type::Namespace);
        define_type(*type::TransientArray);

        define_multimethod(HASH_OBJ, *first_type, nil);
        define_method(HASH_OBJ, nil, *ret_zero);

        define_multimethod(NEW, *first_arg, nil);

        Root f;

        define(CURRENT_NS, get_ns(CLEO_CORE));
        f = create_native_function1<in_ns, &IN_NS>();
        define(IN_NS, *f);

        define(LIB_PATHS, nil);

        define(COMMAND_LINE_ARGS, nil);

        f = create_native_function2<identical, &IDENTICAL>();
        define(IDENTICAL, *f);

        f = create_native_function2<isa, &ISA>();
        define(ISA, *f);

        f = create_native_function1<symbol_q, &SYMBOL_Q>();
        define(SYMBOL_Q, *f);

        f = create_native_function1<keyword_q, &KEYWORD_Q>();
        define(KEYWORD_Q, *f);

        f = create_native_function1<vector_q, &VECTOR_Q>();
        define(VECTOR_Q, *f);

        f = create_native_function1<map_q, &MAP_Q>();
        define(MAP_Q, *f);

        f = create_native_function(list);
        define(LIST, *f);

        f = create_native_function(vector);
        define(VECTOR, *f);

        f = create_native_function(hash_map);
        define(HASH_MAP, *f);

        f = create_native_function(hash_set);
        define(HASH_SET, *f);

        f = create_native_function(concati);
        define(CONCATI, *f);

        f = create_native_function1<mk_keyword, &KEYWORD>();
        define(KEYWORD, *f);

        f = create_native_function1<get_name, &NAME>();
        define(NAME, *f);

        f = create_native_function2<mk_symbol, &SYMBOL>();
        define(SYMBOL, *f);

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

        derive(*type::List, *type::Sequence);
        f = create_native_function1<list_seq>();
        define_method(SEQ, *type::List, *f);
        f = create_native_function1<get_list_first>();
        define_method(FIRST, *type::List, *f);
        f = create_native_function1<get_list_next>();
        define_method(NEXT, *type::List, *f);

        derive(*type::Cons, *type::Sequence);
        f = create_native_function1<identity>();
        define_method(SEQ, *type::Cons, *f);
        f = create_native_function1<cons_first>();
        define_method(FIRST, *type::Cons, *f);
        f = create_native_function1<cons_next>();
        define_method(NEXT, *type::Cons, *f);

        derive(*type::LazySeq, *type::Sequence);
        f = create_native_function1<lazy_seq_seq>();
        define_method(SEQ, *type::LazySeq, *f);
        f = create_native_function1<lazy_seq_first>();
        define_method(FIRST, *type::LazySeq, *f);
        f = create_native_function1<lazy_seq_next>();
        define_method(NEXT, *type::LazySeq, *f);

        derive(*type::Array, *type::Seqable);
        f = create_native_function1<array_seq>();
        define_method(SEQ, *type::Array, *f);
        f = create_native_function1<get_array_seq_first>();
        define_method(FIRST, *type::ArraySeq, *f);
        f = create_native_function1<get_array_seq_next>();
        define_method(NEXT, *type::ArraySeq, *f);

        derive(*type::ArraySet, *type::Seqable);
        f = create_native_function1<array_set_seq>();
        define_method(SEQ, *type::ArraySet, *f);
        f = create_native_function1<get_array_set_seq_first>();
        define_method(FIRST, *type::ArraySetSeq, *f);
        f = create_native_function1<get_array_set_seq_next>();
        define_method(NEXT, *type::ArraySetSeq, *f);

        derive(*type::ArrayMap, *type::Seqable);
        f = create_native_function1<array_map_seq>();
        define_method(SEQ, *type::ArrayMap, *f);
        f = create_native_function1<get_array_map_seq_first>();
        define_method(FIRST, *type::ArrayMapSeq, *f);
        f = create_native_function1<get_array_map_seq_next>();
        define_method(NEXT, *type::ArrayMapSeq, *f);

        derive(*type::PersistentHashMap, *type::Seqable);
        f = create_native_function1<persistent_hash_map_seq>();
        define_method(SEQ, *type::PersistentHashMap, *f);
        f = create_native_function1<get_persistent_hash_map_seq_first>();
        define_method(FIRST, *type::PersistentHashMapSeq, *f);
        f = create_native_function1<get_persistent_hash_map_seq_next>();
        define_method(NEXT, *type::PersistentHashMapSeq, *f);

        derive(*type::ArraySeq, *type::Sequence);
        derive(*type::ArraySetSeq, *type::Sequence);
        derive(*type::ArrayMapSeq, *type::Sequence);
        derive(*type::PersistentHashMapSeq, *type::Sequence);
        derive(*type::Sequence, *type::Seqable);
        f = create_native_function1<identity>();
        define_method(SEQ, *type::Sequence, *f);

        f = create_native_function1<get_seqable_first>();
        define_method(FIRST, *type::Seqable, *f);
        f = create_native_function1<get_seqable_next>();
        define_method(NEXT, *type::Seqable, *f);

        define_multimethod(COUNT, *first_type, undefined);
        f = create_native_function1<WrapUInt32Fn<get_array_map_size>::fn>();
        define_method(COUNT, *type::ArrayMap, *f);
        f = create_native_function1<WrapInt64Fn<get_persistent_hash_map_size>::fn>();
        define_method(COUNT, *type::PersistentHashMap, *f);
        f = create_native_function1<WrapUInt32Fn<get_array_set_size>::fn>();
        define_method(COUNT, *type::ArraySet, *f);
        f = create_native_function1<WrapInt64Fn<get_list_size>::fn>();
        define_method(COUNT, *type::List, *f);
        f = create_native_function1<cons_size>();
        define_method(COUNT, *type::Cons, *f);
        f = create_native_function1<seq_count>();
        define_method(COUNT, *type::Sequence, *f);
        f = create_native_function1<WrapUInt32Fn<get_array_size>::fn>();
        define_method(COUNT, *type::Array, *f);
        f = create_native_function1<get_transient_array_size>();
        define_method(COUNT, *type::TransientArray, *f);
        f = create_native_function1<WrapUInt32Fn<get_string_len>::fn>();
        define_method(COUNT, *type::String, *f);
        f = create_native_function1<nil_count>();
        define_method(COUNT, nil, *f);

        define_multimethod(GET, *first_type, undefined);

        f = create_native_function2<array_map_get, &GET>();
        define_method(GET, *type::ArrayMap, *f);

        f = create_native_function2or3<persistent_hash_map_get, persistent_hash_map_get, &GET>();
        define_method(GET, *type::PersistentHashMap, *f);

        f = create_native_function2<array_get>();
        define_method(GET, *type::Array, *f);

        f = create_native_function2<transient_array_get>();
        define_method(GET, *type::TransientArray, *f);

        f = create_native_function2or3<nil_get, nil_get, &GET>();
        define_method(GET, nil, *f);

        define_multimethod(CONTAINS, *first_type, undefined);

        f = create_native_function2<array_map_contains, &CONTAINS>();
        define_method(CONTAINS, *type::ArrayMap, *f);

        f = create_native_function2<persistent_hash_map_contains, &CONTAINS>();
        define_method(CONTAINS, *type::PersistentHashMap, *f);

        f = create_native_function2<nil_contains, &CONTAINS>();
        define_method(CONTAINS, nil, *f);

        define_multimethod(CONJ, *first_type, undefined);

        f = create_native_function2<array_conj>();
        define_method(CONJ, *type::Array, *f);

        define_method(CONJ, *type::ArraySet, *rt::array_set_conj);

        f = create_native_function2<list_conj>();
        define_method(CONJ, *type::List, *f);

        f = create_native_function2<cons_conj>();
        define_method(CONJ, *type::Cons, *f);

        f = create_native_function2<lazy_seq_conj>();
        define_method(CONJ, *type::LazySeq, *f);

        f = create_native_function2<nil_conj>();
        define_method(CONJ, nil, *f);

        f = create_native_function2<cons, &CONS>();
        define(CONS, *f);

        f = create_native_function1<create_lazy_seq, &LAZY_SEQ>();
        define(LAZY_SEQ, *f);

        define_multimethod(ASSOC, *first_type, undefined);

        derive(*type::ArrayMap, *type::PersistentMap);
        f = create_native_function3<array_map_assoc>();
        define_method(ASSOC, *type::ArrayMap, *f);

        derive(*type::PersistentHashMap, *type::PersistentMap);
        define_method(ASSOC, *type::PersistentHashMap, *rt::persistent_hash_map_assoc);

        f = create_native_function3<nil_assoc>();
        define_method(ASSOC, nil, *f);

        define_multimethod(DISSOC, *first_type, undefined);

        f = create_native_function2<persistent_hash_map_dissoc>();
        define_method(DISSOC, *type::PersistentHashMap, *f);

        f = create_native_function2<nil_dissoc>();
        define_method(DISSOC, nil, *f);

        f = create_native_function0<create_array_map, &ARRAY_MAP>();
        define(ARRAY_MAP, *f);

        define_multimethod(MERGE, *equal_dispatch, nil);
        f = create_native_function2<merge_maps, &MERGE>();

        std::array<Value, 2> two_maps{{*type::PersistentMap, *type::PersistentMap}};
        Root v{create_array(two_maps.data(), two_maps.size())};
        define_method(MERGE, *v, *f);

        define_multimethod(OBJ_CALL, *first_type, nil);

        derive(*type::ArraySet, *type::Callable);
        f = create_native_function2<array_set_get>();
        define_method(OBJ_CALL, *type::ArraySet, *f);

        derive(*type::ArrayMap, *type::Callable);
        f = create_native_function2<array_map_get>();
        define_method(OBJ_CALL, *type::ArrayMap, *f);

        derive(*type::PersistentHashMap, *type::Callable);
        f = create_native_function2or3<persistent_hash_map_get, persistent_hash_map_get>();
        define_method(OBJ_CALL, *type::PersistentHashMap, *f);

        derive(*type::Keyword, *type::Callable);
        f = create_native_function2<keyword_get, &KEYWORD_TYPE>();
        define_method(OBJ_CALL, *type::Keyword, *f);

        derive(*type::Array, *type::Callable);
        f = create_native_function2<array_call>();
        define_method(OBJ_CALL, *type::Array, *f);

        derive(*type::TransientArray, *type::Callable);
        f = create_native_function2<transient_array_call>();
        define_method(OBJ_CALL, *type::TransientArray, *f);

        derive(*type::CFunction, *type::Callable);
        f = create_native_function(call_c_function);
        define_method(OBJ_CALL, *type::CFunction, *f);

        define_multimethod(OBJ_EQ, *equal_dispatch, nil);
        define_method(OBJ_EQ, nil, *ret_nil);

        auto define_seq_eq = [&](auto type1, auto type2)
        {
            std::array<Value, 2> two{{type1, type2}};
            Root v{create_array(two.data(), two.size())};
            Root f{create_native_function2<are_seqables_equal>()};
            define_method(OBJ_EQ, *v, *f);
        };

        define_seq_eq(*type::Array, *type::Array);
        define_seq_eq(*type::Array, *type::List);
        define_seq_eq(*type::Array, *type::Sequence);
        define_seq_eq(*type::Sequence, *type::Array);
        define_seq_eq(*type::Sequence, *type::List);
        define_seq_eq(*type::Sequence, *type::Sequence);
        define_seq_eq(*type::List, *type::Array);
        define_seq_eq(*type::List, *type::List);
        define_seq_eq(*type::List, *type::Sequence);

        std::array<Value, 2> two_sets{{*type::ArraySet, *type::ArraySet}};
        v = create_array(two_sets.data(), two_sets.size());
        f = create_native_function2<are_array_sets_equal>();
        define_method(OBJ_EQ, *v, *f);

        std::array<Value, 2> two_array_maps{{*type::ArrayMap, *type::ArrayMap}};
        v = create_array(two_array_maps.data(), two_array_maps.size());
        f = create_native_function2<are_array_maps_equal>();
        define_method(OBJ_EQ, *v, *f);

        std::array<Value, 2> two_hash_maps{{*type::PersistentHashMap, *type::PersistentHashMap}};
        v = create_array(two_hash_maps.data(), two_hash_maps.size());
        f = create_native_function2<are_persistent_hash_maps_equal>();
        define_method(OBJ_EQ, *v, *f);

        v = create_array(two_maps.data(), two_maps.size());
        f = create_native_function2<are_maps_equal>();
        define_method(OBJ_EQ, *v, *f);

        std::array<Value, 2> two_var_refs{{*type::VarValueRef, *type::VarValueRef}};
        v = create_array(two_var_refs.data(), two_var_refs.size());
        f = create_native_function2<var_value_ref_equals>();
        define_method(OBJ_EQ, *v, *f);

        std::array<Value, 2> two_fn_calls{{*type::FnCall, *type::FnCall}};
        v = create_array(two_fn_calls.data(), two_fn_calls.size());
        f = create_native_function2<fn_call_equals>();
        define_method(OBJ_EQ, *v, *f);

        define(PRINT_READABLY, TRUE);

        define_multimethod(PR_STR_OBJ, *first_type, nil);
        f = create_native_function1<pr_str_type>();
        define_method(PR_STR_OBJ, *type::MetaType, *f);

        f = create_native_function1<var_value_ref_pr_str>();
        define_method(PR_STR_OBJ, *type::VarValueRef, *f);
        f = create_native_function1<fn_call_pr_str>();
        define_method(PR_STR_OBJ, *type::FnCall, *f);
        f = create_native_function1<pr_str_array>();
        define_method(PR_STR_OBJ, *type::Array, *f);
        f = create_native_function1<pr_str_array_set>();
        define_method(PR_STR_OBJ, *type::ArraySet, *f);
        f = create_native_function1<pr_str_array_map>();
        define_method(PR_STR_OBJ, *type::ArrayMap, *f);
        f = create_native_function1<pr_str_persistent_hash_map>();
        define_method(PR_STR_OBJ, *type::PersistentHashMap, *f);
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
        f = create_native_function2<div2, &SLASH>();
        define(SLASH, *f);
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
        f = create_native_function1<arithmetic_exception_message>();
        define_method(GET_MESSAGE, *type::ArithmeticException, *f);

        derive(*type::IndexOutOfBounds, *type::Exception);
        f = create_native_new0<new_index_out_of_bounds, &NEW>();
        define_method(NEW, *type::IndexOutOfBounds, *f);
        f = create_native_function1<index_out_of_bounds_message>();
        define_method(GET_MESSAGE, *type::IndexOutOfBounds, *f);

        derive(*type::CompilationError, *type::Exception);
        f = create_native_function1<compilation_error_message>();
        define_method(GET_MESSAGE, *type::CompilationError, *f);

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

        define_function(ALIAS, create_native_function2<alias, &ALIAS>());

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

        f = create_native_function1<pr_str, &PR_STR>();
        define(PR_STR, *f);

        f = create_native_function(pr);
        define(create_symbol("cleo.core", "pr"), *f);

        f = create_native_function(prn);
        define(create_symbol("cleo.core", "prn"), *f);

        f = create_native_function(print);
        define(create_symbol("cleo.core", "print"), *f);

        f = create_native_function(println);
        define(create_symbol("cleo.core", "println"), *f);

        define_function(STR, create_native_function(str));

        f = create_native_function1<get_value_type, &TYPE>();
        define(TYPE, *f);

        Root macro_meta{create_persistent_hash_map()};
        macro_meta = persistent_hash_map_assoc(*macro_meta, MACRO_KEY, TRUE);
        f = create_ns_macro();
        define(NS, *f, *macro_meta);

        f = create_native_function4<import_c_fn, &IMPORT_C_FN>();
        define(IMPORT_C_FN, *f);

        f = create_native_function(gensym);
        define(GENSYM, *f);

        f = create_native_function0<mem_used, &MEMUSED>();
        define(MEMUSED, *f);

        f = create_native_function0<mem_allocs, &MEMALLOCS>();
        define(MEMALLOCS, *f);

        define_function(BITNOT, create_native_function1<bit_not, &BITNOT>());
        define_function(BITAND, create_native_function2<bit_and, &BITAND>());
        define_function(BITOR, create_native_function2<bit_or, &BITOR>());
        define_function(BITXOR, create_native_function2<bit_xor, &BITXOR>());
        define_function(BITANDNOT, create_native_function2<bit_and_not, &BITANDNOT>());
        define_function(BITCLEAR, create_native_function2<bit_clear, &BITCLEAR>());
        define_function(BITSET, create_native_function2<bit_set, &BITSET>());
        define_function(BITFLIP, create_native_function2<bit_flip, &BITFLIP>());
        define_function(BITTEST, create_native_function2<bit_test, &BITTEST>());
        define_function(BITSHIFTLEFT, create_native_function2<bit_shift_left, &BITSHIFTLEFT>());
        define_function(BITSHIFTRIGHT, create_native_function2<bit_shift_right, &BITSHIFTRIGHT>());
        define_function(UNSIGNEDBITSHIFTRIGHT, create_native_function2<unsigned_bit_shift_right, &UNSIGNEDBITSHIFTRIGHT>());

        define_function(NS_MAP, create_native_function1<ns_map, &NS_MAP>());
        define_function(NS_NAME, create_native_function1<ns_name, &NS_NAME>());
        define_function(NS_ALIASES, create_native_function1<ns_aliases, &NS_ALIASES>());

        define_function(GC_LOG, create_native_function1<set_gc_log, &GC_LOG>());

        define_function(GET_TIME, create_native_function0<get_time, &GET_TIME>());

        define_multimethod(CONJ_E, *first_type, undefined);

        define_method(CONJ_E, *type::TransientArray, *rt::transient_array_conj);

        define_multimethod(TRANSIENT, *first_type, undefined);

        define_method(TRANSIENT, *type::Array, *rt::transient_array);

        define_multimethod(PERSISTENT, *first_type, undefined);

        define_method(PERSISTENT, *type::TransientArray, *rt::transient_array_persistent);

    }
} initialize;

}

}
