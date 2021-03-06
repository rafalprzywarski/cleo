#include "global.hpp"
#include "array.hpp"
#include "byte_array.hpp"
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
#include "persistent_hash_set.hpp"
#include "namespace.hpp"
#include "reader.hpp"
#include "atom.hpp"
#include "util.hpp"
#include "clib.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <limits>
#include <chrono>
#include <cstring>
#include <algorithm>
#include "bytecode_fn.hpp"
#include "compile.hpp"
#include "profiler.hpp"
#include "string_seq.hpp"

namespace cleo
{

std::vector<Allocation> allocations;
std::vector<Value> extra_roots;
unsigned gc_frequency = 4096;
unsigned gc_counter = gc_frequency - 1;
std::unique_ptr<std::ostream> gc_log;

vm::Stack stack;
vm::IntStack int_stack;

std::unordered_map<std::string, std::unordered_map<std::string, Value>> symbols;
std::unordered_map<std::string, std::unordered_map<std::string, Value>> keywords;

std::unordered_map<Value, Value, std::hash<Value>, StdIs> vars;

Root current_exception;

namespace prof
{
bool enabled = false;
Value callstack[MAX_CALLSTACK_SIZE];
volatile std::size_t callstack_size = 0;
Value callstack_copy[MAX_CALLSTACK_SIZE];
std::size_t callstack_size_copy;
std::atomic_bool callstack_copy_ready;
std::atomic_bool callstack_copy_needed;
std::atomic_bool finished;
std::thread::id main_thread_id{};
std::vector<std::vector<Value>> callstacks;
std::thread collector{};
}

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
const Value INTERNAL_ADD_2 = create_symbol("cleo.core", "internal-add-2");
const Value MINUS = create_symbol("cleo.core", "-");
const Value ASTERISK = create_symbol("cleo.core", "*");
const Value IDENTICAL = create_symbol("cleo.core", "identical?");
const Value ISA = create_symbol("cleo.core", "isa?");
const Value SYMBOL_Q = create_symbol("cleo.core", "symbol?");
const Value KEYWORD_Q = create_symbol("cleo.core", "keyword?");
const Value STRING_Q = create_symbol("cleo.core", "string?");
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
const Value APPLY = create_symbol("cleo.core", "apply");
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
const Value DYNAMIC_KEY = create_keyword("dynamic");
const Value CONST_KEY = create_keyword("const");
const Value PRIVATE_KEY = create_keyword("private");
const Value NAME_KEY = create_keyword("name");
const Value NS_KEY = create_keyword("ns");
const Value DOT = create_symbol(".");
const Value EVAL = create_symbol("cleo.core", "eval");
const Value COMPILE_FN_AST = create_symbol("cleo.compiler", "compile-fn-ast");
const Value ADD_VAR_FN_DEP = create_symbol("cleo.core", "add-var-fn-dep");
const Value SHOULD_RECOMPILE = create_symbol("cleo.core", "should-recompile");
const Value GLOBAL_HIERARCHY = create_symbol("cleo.core", "global-hierarchy");

const Root ZERO{create_int64(0)};
const Root ONE{create_int64(1)};
const Root NEG_ONE{create_int64(-1)};
const Root TWO{create_int64(2)};
const Root THREE{create_int64(3)};
namespace
{
Force create_basic_type(const std::string& ns, const std::string& name)
{
    return create_object_type(ns, name, nullptr, nullptr, 0, false, false);
}

const ConstRoot SENTINEL_TYPE{create_basic_type("cleo.core", "Sentinel")};
}
const Root SENTINEL{create_object0(*SENTINEL_TYPE)};
const Root RELOAD{create_keyword("reload")};

const std::unordered_set<Value, std::hash<Value>, StdIs> SPECIAL_SYMBOLS{
    QUOTE,
    FN,
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
    VA,
    DOT
};

namespace type
{
const ConstRoot Type{create_basic_type("cleo.core", "Type")};
const ConstRoot True{create_basic_type("cleo.core", "True")};
const ConstRoot Protocol{create_basic_type("cleo.core", "Protocol")};
}

namespace
{
const ConstRoot TRUE_root{create_object(*type::True, nullptr, 0)};
}

const Value TRUE = *TRUE_root;

namespace
{

Force create_dynamic_type(const std::string& ns, const std::string& name)
{
    return create_dynamic_object_type(ns, name);
}

struct FieldDesc
{
    std::string name;
    Value type;

    FieldDesc(const char *name) : name(name) { }
    FieldDesc(const char *name, Value type) : name(name), type(type) { }
};

Force create_static_type(const std::string& ns, const std::string& name, const std::initializer_list<FieldDesc>& fields)
{
    std::vector<Value> field_names, field_types;
    field_names.reserve(fields.size());
    for (auto& f : fields)
    {
        field_names.push_back(create_symbol(f.name));
        field_types.push_back(f.type);
    }
    return create_object_type(ns, name, field_names.data(), field_types.data(), field_names.size(), true, false);
}
}

namespace type
{
const ConstRoot Int64_root{create_basic_type("cleo.core", "Int64")};
const Value Int64{*Int64_root};
const ConstRoot UChar{create_basic_type("cleo.core", "UChar")};
const ConstRoot Float64{create_basic_type("cleo.core", "Float64")};
const ConstRoot UTF8String{create_basic_type("cleo.core", "UTF8String")};
const ConstRoot NativeFunction{create_basic_type("cleo.core", "NativeFunction")};
const ConstRoot CFunction{create_static_type("cleo.core", "CFunction", {{"addr", Int64}, "name", "param-types"})};
const ConstRoot Symbol{create_basic_type("cleo.core", "Symbol")};
const ConstRoot Keyword{create_basic_type("cleo.core", "Keyword")};
const ConstRoot Var{create_static_type("cleo.core", "Var", {"name", "value", "meta", "dep-fns"})};
const ConstRoot List{create_static_type("cleo.core", "List", {{"size", Int64}, "first", "next"})};
const ConstRoot Cons{create_static_type("cleo.core", "Cons", {"first", "next"})};
const ConstRoot LazySeq{create_static_type("cleo.core", "LazySeq", {"fn", "seq"})};
const ConstRoot PersistentVector{create_protocol("cleo.core", "PersistentVector")};
const ConstRoot Array{create_dynamic_type("cleo.core", "Array")};
const ConstRoot TransientArray{create_dynamic_type("cleo.core", "TransientArray")};
const ConstRoot ArraySeq{create_static_type("cleo.core", "ArraySeq", {"array", {"index", Int64}})};
const ConstRoot ByteArray{create_dynamic_type("cleo.core", "ByteArray")};
const ConstRoot TransientByteArray{create_dynamic_type("cleo.core", "TransientByteArray")};
const ConstRoot ByteArraySeq{create_static_type("cleo.core", "ByteArraySeq", {"array", {"index", Int64}})};
const ConstRoot ArrayMap{create_dynamic_type("cleo.core", "ArrayMap")};
const ConstRoot ArrayMapSeq{create_static_type("cleo.core", "ArrayMapSeq", {"first", "map", {"index", Int64}})};
const ConstRoot PersistentSet{create_protocol("cleo.core", "PersistentSet")};
const ConstRoot ArraySet{create_dynamic_type("cleo.core", "ArraySet")};
const ConstRoot ArraySetSeq{create_static_type("cleo.core", "ArraySetSeq", {"set", {"index", Int64}})};
const ConstRoot Hierarchy{create_static_type("cleo.core", "Hierarchy", {"ancestors"})};
const ConstRoot Multimethod{create_static_type("cleo.core", "Multimethod", {"dispatch_fn", "hierarchy", "memoized_fns", "fns", "default_dispatch_val", "name"})};
const ConstRoot Seqable{create_protocol("cleo.core", "Seqable")};
const ConstRoot Sequence{create_protocol("cleo.core", "Sequence")};
const ConstRoot Callable{create_protocol("cleo.core", "Callable")};
const ConstRoot BytecodeFn{create_dynamic_type("cleo.core", "BytecodeFn")};
const ConstRoot OpenBytecodeFn{create_dynamic_type("cleo.core", "OpenBytecodeFn")};
const ConstRoot BytecodeFnBody{create_dynamic_type("cleo.core", "BytecodeFnBody")};
const ConstRoot BytecodeFnExceptionTable{create_dynamic_type("cleo.core", "BytecodeFnExceptionTable")};
const ConstRoot Atom{create_static_type("cleo.core", "Atom", {"value"})};
const ConstRoot PersistentMap{create_protocol("cleo.core", "PersistentMap")};
const ConstRoot PersistentHashMap{create_dynamic_type("cleo.core", "PersistentHashMap")};
const ConstRoot PersistentHashMapSeq{create_static_type("cleo.core", "PersistentHashMapSeq", {"first", "node", {"index", Int64}, "parent"})};
const ConstRoot PersistentHashMapSeqParent{create_static_type("cleo.core", "PersistentHashMapSeqParent", {{"index", Int64}, "node", "parent"})};
const ConstRoot PersistentHashMapCollisionNode(create_dynamic_type("cleo.core", "PersistentHashMapCollisionNode"));
const ConstRoot PersistentHashMapArrayNode(create_dynamic_type("cleo.core", "PersistentHashMapArrayNode"));
const ConstRoot PersistentHashSet{create_static_type("cleo.core", "PersistentHashSet", {{"size", Int64}, "root"})};
const ConstRoot PersistentHashSetSeq{create_static_type("cleo.core", "PersistentHashSetSeq", {"first", "node", {"index", Int64}, "parent"})};
const ConstRoot PersistentHashSetSeqParent{create_static_type("cleo.core", "PersistentHashSetSeqParent", {{"index", Int64}, "node", "parent"})};
const ConstRoot PersistentHashSetCollisionNode(create_dynamic_type("cleo.core", "PersistentHashSetCollisionNode"));
const ConstRoot PersistentHashSetArrayNode(create_dynamic_type("cleo.core", "PersistentHashSetArrayNode"));
const ConstRoot Exception{create_static_type("cleo.core", "Exception", {"msg"})};
const ConstRoot LogicException{create_static_type("cleo.core", "LogicException", {"msg", "callstack"})};
const ConstRoot CastError{create_static_type("cleo.core", "CastError", {"msg", "callstack"})};
const ConstRoot ReadError{create_static_type("cleo.core", "ReadError", {"msg", {"line", Int64}, {"column", Int64}})};
const ConstRoot CallError{create_static_type("cleo.core", "CallError", {"msg", "callstack"})};
const ConstRoot SymbolNotFound{create_static_type("cleo.core", "SymbolNotFound", {"msg"})};
const ConstRoot IllegalArgument{create_static_type("cleo.core", "IllegalArgument", {"msg", "callstack"})};
const ConstRoot IllegalState{create_static_type("cleo.core", "IllegalState", {"msg", "callstack"})};
const ConstRoot UnexpectedEndOfInput{create_static_type("cleo.core", "UnexpectedEndOfInput", {"msg", {"line", Int64}, {"column", Int64}})};
const ConstRoot FileNotFound{create_static_type("cleo.core", "FileNotFound", {"msg"})};
const ConstRoot ArithmeticException{create_static_type("cleo.core", "ArithmeticException", {"msg", "callstack"})};
const ConstRoot IndexOutOfBounds{create_static_type("cleo.core", "IndexOutOfBounds", {"msg", "callstack"})};
const ConstRoot CompilationError{create_static_type("cleo.core", "CompilationError", {"msg"})};
const ConstRoot StackOverflow{create_static_type("cleo.core", "StackOverflow", {"msg", "callstack"})};
const ConstRoot Namespace{create_static_type("cleo.core", "Namespace", {"name", "meta", "mapping", "aliases"})};
const ConstRoot UTF8StringSeq{create_static_type("cleo.core", "UTF8StringSeq", {"str", {"offset", Int64}})};
}

namespace clib
{
const Value int64 = create_keyword("int64");
const Value string = create_keyword("string");
}

const ConstRoot EMPTY_LIST{create_list(nullptr, 0)};
const ConstRoot EMPTY_VECTOR{create_array(nullptr, 0)};
const ConstRoot EMPTY_BYTE_ARRAY{create_byte_array(nullptr, 0)};
const ConstRoot EMPTY_SET{create_array_set()};
const ConstRoot EMPTY_MAP{create_array_map()};
const ConstRoot EMPTY_HASH_MAP{create_persistent_hash_map()};
const ConstRoot EMPTY_HASH_SET{create_persistent_hash_set()};

Root namespaces{*EMPTY_MAP};
Root bindings;

Int64 next_id = 0;

Int64 gen_id()
{
    return next_id++;
}

namespace
{

const ConstRoot DYNAMIC_META{map_assoc(*EMPTY_MAP, DYNAMIC_KEY, TRUE)};
const ConstRoot CONST_META{map_assoc(*EMPTY_MAP, CONST_KEY, TRUE)};
const ConstRoot MACRO_META{map_assoc(*EMPTY_MAP, MACRO_KEY, TRUE)};

}

namespace
{
const Value TRANSIENT = create_symbol("cleo.core", "transient");
const Value PERSISTENT = create_symbol("cleo.core", "persistent!");
const Value CONJ_E = create_symbol("cleo.core", "conj!");
const Value POP_E = create_symbol("cleo.core", "pop!");
}

namespace rt
{

const Root transient_array{create_native_function1<cleo::transient_array, &TRANSIENT>()};
const Root transient_array_conj{create_native_function2<cleo::transient_array_conj, &CONJ_E>()};
const Root transient_array_pop{create_native_function1<cleo::transient_array_pop, &POP_E>()};
const Root transient_array_persistent{create_native_function1<cleo::transient_array_persistent, &PERSISTENT>()};
const Root set_conj{create_native_function2<cleo::set_conj, &CONJ>()};
const Root map_assoc{create_native_function3<cleo::map_assoc, &ASSOC>()};

const DynamicVar current_ns = define_var(CURRENT_NS, nil, *DYNAMIC_META);
const DynamicVar lib_paths = define_var(LIB_PATHS, nil, *DYNAMIC_META);
const StaticVar obj_eq = define_var(OBJ_EQ, nil);
const StaticVar obj_call = define_var(OBJ_CALL, nil);
const DynamicVar print_readably = define_var(PRINT_READABLY, nil, *DYNAMIC_META);
const StaticVar pr_str_obj = define_var(PR_STR_OBJ, nil);
const StaticVar first = define_var(FIRST, nil);
const StaticVar next = define_var(NEXT, nil);
const StaticVar seq = define_var(SEQ, nil);
const StaticVar count = define_var(COUNT, nil);
const StaticVar get = define_var(GET, nil);
const StaticVar contains = define_var(CONTAINS, nil);
const StaticVar assoc = define_var(ASSOC, nil);
const StaticVar merge = define_var(MERGE, nil);
const StaticVar hash_obj = define_var(HASH_OBJ, nil);
const StaticVar eval = define_var(EVAL, nil);
const StaticVar compile_fn_ast = define_var(COMPILE_FN_AST, nil);
const StaticVar global_hierarchy = define_var(GLOBAL_HIERARCHY, nil);

}

Force current_callstack()
{
    return create_array(prof::callstack, prof::callstack_size);
}

namespace
{

const Value MACROEXPAND1 = create_symbol("cleo.core", "macroexpand-1");
const Value MACROEXPAND = create_symbol("cleo.core", "macroexpand");
const Value REFER = create_symbol("cleo.core", "refer");
const Value READ_STRING = create_symbol("cleo.core", "read-string");
const Value LOAD_STRING = create_symbol("cleo.core", "load-string");
const Value REQUIRE = create_symbol("cleo.core", "require*");
const Value ALIAS = create_symbol("cleo.core", "alias");
const Value TYPE = create_symbol("cleo.core", "type");
const Value KEYWORD_TYPE_NAME = get_object_type_name(*type::Keyword);
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
const Value KEYWORD = create_symbol("cleo.core", "keyword");
const Value NAME = create_symbol("cleo.core", "name");
const Value NAMESPACE = create_symbol("cleo.core", "namespace");
const Value SYMBOL = create_symbol("cleo.core", "symbol");
const Value NS_MAP = create_symbol("cleo.core", "ns-map");
const Value NS_NAME = create_symbol("cleo.core", "ns-name");
const Value NS_ALIASES = create_symbol("cleo.core", "ns-aliases");
const Value NAMESPACES = create_symbol("cleo.core", "namespaces*");
const Value QUOT = create_symbol("cleo.core", "quot");
const Value REM = create_symbol("cleo.core", "rem");
const Value GC_LOG = create_symbol("cleo.core", "gc-log");
const Value GET_TIME = create_symbol("cleo.core", "get-time");
const Value PROTOCOL = create_symbol("cleo.core", "protocol*");
const Value CREATE_TYPE = create_symbol("cleo.core", "type*");
const Value DEFMULTI = create_symbol("cleo.core", "defmulti*");
const Value DEFMETHOD = create_symbol("cleo.core", "defmethod*");
const Value DISASM = create_symbol("cleo.core", "disasm*");
const Value GET_BYTECODE_FN_BODY = create_symbol("cleo.core", "get-bytecode-fn-body");
const Value GET_BYTECODE_FN_CONSTS = create_symbol("cleo.core", "get-bytecode-fn-consts");
const Value GET_BYTECODE_FN_VARS = create_symbol("cleo.core", "get-bytecode-fn-vars");
const Value GET_BYTECODE_FN_LOCALS_SIZE = create_symbol("cleo.core", "get-bytecode-fn-locals-size");
const Value META = create_symbol("cleo.core", "meta");
const Value THE_NS = create_symbol("cleo.core", "the-ns");
const Value FIND_NS = create_symbol("cleo.core", "find-ns");
const Value RESOLVE = create_symbol("cleo.core", "resolve");
const Value SUBS = create_symbol("cleo.core", "subs");
const Value ASSOC_E = create_symbol("cleo.core", "assoc!");
const Value DEFINE_VAR = create_symbol("cleo.core", "define-var");
const Value SERIALIZE_FN = create_symbol("cleo.core", "serialize-fn");
const Value DESERIALIZE_FN = create_symbol("cleo.core", "deserialize-fn");
const Value CHAR = create_symbol("cleo.core", "char");
const Value START_PROFILING = create_symbol("cleo.core", "start-profiling");
const Value FINISH_PROFILING = create_symbol("cleo.core", "finish-profiling");
const Value STR_STARTS_WITH = create_symbol("cleo.core", "str-starts-with?");
const Value SORT_E = create_symbol("cleo.core", "sort!");
const Value DERIVE = create_symbol("cleo.core", "derive");
const Value GET_TYPE_FIELD_INDEX = create_symbol("cleo.core", "get-type-field-index");
const Value PEEK = create_symbol("cleo.core", "peek");
const Value POP = create_symbol("cleo.core", "pop");
const Value VAR_NAME = create_symbol("cleo.core", "var-name");
const Value CURRENT_CALLSTACK = create_symbol("cleo.core", "current-callstack");

const Value FIRST_ARG_TYPE = create_symbol("first-arg-type");
const Value FIRST_ARG = create_symbol("first-arg");
const Value EQUAL_DISPATCH = create_symbol("equal-dispatch");

const Value BYTE_ARRAY = create_symbol("cleo.core", "byte-array");

const Value TRANSIENT_VECTOR = create_symbol("cleo.core", "transient-vector");
const Value PERSISTENT_VECTOR = create_symbol("cleo.core", "persistent-vector!");
const Value TRANSIENT_VECTOR_ASSOC = create_symbol("cleo.core", "transient-vector-assoc!");

const Root first_type{create_native_function([](const Value *args, std::uint8_t num_args) -> Force
{
    return (num_args < 1) ? nil : get_value_type(args[0]);
}, FIRST_ARG_TYPE)};

const Root first_arg{create_native_function([](const Value *args, std::uint8_t num_args) -> Force
{
    return (num_args < 1) ? nil : args[0];
}, FIRST_ARG)};

const Root equal_dispatch{create_native_function([](const Value *args, std::uint8_t num_args)
{
    check_arity(OBJ_EQ, 2, num_args);
    std::array<Value, 2> types{{get_value_type(args[0]), get_value_type(args[1])}};
    return create_array(types.data(), types.size());
}, EQUAL_DISPATCH)};

const Root ret_eq_nil{create_native_function([](const Value *, std::uint8_t)
{
    return force(nil);
}, OBJ_EQ)};

const Root ret_hash_zero{create_native_function([](const Value *, std::uint8_t)
{
    return force(*ZERO);
}, HASH_OBJ)};

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
        Root msg{create_string("expected Int64, got: " + to_string(l) + " " + to_string(r))};
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

Force quot(Value l, Value r)
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

Force rem(Value l, Value r)
{
    check_ints(l, r);
    auto lv = get_int64_value(l), rv = get_int64_value(r);
    if (rv == 0)
    {
        Root s{create_string("Divide by zero")};
        throw_exception(new_arithmetic_exception(*s));
    }
    return create_int64(lv % rv);
}

Force lt2(Value l, Value r)
{
    check_ints(l, r);
    return get_int64_value(l) < get_int64_value(r) ? TRUE : nil;
}

Force pr_str_exception(Value e)
{
    cleo::Root msg{exception_message(e)};
    cleo::Root type{cleo::pr_str(cleo::get_value_type(e))};
    msg = print_str(*msg);

    return create_string(
        std::string(get_string_ptr(*type), get_string_size(*type)) + ": " +
        std::string(get_string_ptr(*msg), get_string_size(*msg)));
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
    if (i < 0 || i >= get_transient_array_size(v))
        return nil;
    return get_transient_array_elem(v, i);
}

Value byte_array_get(Value v, Value index)
{
    if (get_value_tag(index) != tag::INT64)
        return nil;
    auto i = get_int64_value(index);
    if (i < 0 || i >= get_byte_array_size(v))
        return nil;
    return get_byte_array_elem(v, i);
}

Value transient_byte_array_get(Value v, Value index)
{
    if (get_value_tag(index) != tag::INT64)
        return nil;
    auto i = get_int64_value(index);
    if (i < 0 || i >= get_transient_byte_array_size(v))
        return nil;
    return get_transient_byte_array_elem(v, i);
}

Force transient_array_assoc(Value v, Value index, Value e)
{
    if (get_value_tag(index) != tag::INT64)
        return nil;
    auto i = get_int64_value(index);
    if (i < 0 || i > get_transient_array_size(v))
        throw_index_out_of_bounds();
    return transient_array_assoc_elem(v, i, e);
}

Force transient_byte_array_assoc(Value v, Value index, Value e)
{
    if (get_value_tag(index) != tag::INT64)
        return nil;
    auto i = get_int64_value(index);
    if (i < 0 || i > get_transient_byte_array_size(v))
        throw_index_out_of_bounds();
    return transient_byte_array_assoc_elem(v, i, e);
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

Value byte_array_call(Value v, Value index)
{
    if (get_value_tag(index) != tag::INT64)
        throw_illegal_argument("Key must be integer");
    auto i = get_int64_value(index);
    if (i < 0 || i >= get_byte_array_size(v))
        throw_index_out_of_bounds();
    return get_byte_array_elem(v, i);
}

Value transient_array_call(Value v, Value index)
{
    if (get_value_tag(index) != tag::INT64)
        throw_illegal_argument("Key must be integer");
    auto i = get_int64_value(index);
    if (i < 0 || i >= get_transient_array_size(v))
        throw_index_out_of_bounds();
    return get_transient_array_elem(v, i);
}

Value transient_byte_array_call(Value v, Value index)
{
    if (get_value_tag(index) != tag::INT64)
        throw_illegal_argument("Key must be integer");
    auto i = get_int64_value(index);
    if (i < 0 || i >= get_transient_byte_array_size(v))
        throw_index_out_of_bounds();
    return get_transient_byte_array_elem(v, i);
}

Force var_call(const Value *args, std::uint8_t n)
{
    std::vector<Value> vargs{args, args + n};
    vargs[0] = get_var_value(args[0]);
    return call(vargs.data(), vargs.size());
}

Force pr(const Value *args, std::uint8_t n)
{
    Root s;
    for (decltype(n) i = 0; i < n; ++i)
    {
        s = pr_str(args[i]);
        if (i > 0)
            std::cout << ' ';
        std::cout << std::string(get_string_ptr(*s), get_string_size(*s));
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
        std::cout << std::string(get_string_ptr(*s), get_string_size(*s));
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
        if (auto arg = args[i])
        {
            s = print_str(arg);
            str.append(get_string_ptr(*s), get_string_size(*s));
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

void define_type(Value type)
{
    define(get_object_type_name(type), type, *CONST_META);
}

void define_protocol(Value p)
{
    define(get_protocol_name(p), p, *CONST_META);
}

Force pr_str_var(Value var)
{
    check_type("var", var, *type::Var);
    return create_string("#'" + to_string(get_var_name(var)));
}

Force pr_str_open_bytecode_fn(Value fn)
{
    check_kind("fn", fn, *type::OpenBytecodeFn);
    std::ostringstream os;
    os << "#" << to_string(get_object_type_name(get_value_type(fn))) << "[" << to_string(get_bytecode_fn_name(fn)) << " 0x" << std::hex << fn.bits() << "]";
    return create_string(os.str());
}

Force pr_str_multimethod(Value m)
{
    check_type("m", m, *type::Multimethod);
    std::ostringstream os;
    os << "#" << to_string(get_object_type_name(*type::Multimethod)) << "[" << to_string(get_multimethod_name(m)) << " 0x" << std::hex << m.bits() << "]";
    return create_string(os.str());
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

Value string_q(Value x)
{
    return get_value_tag(x) == tag::UTF8STRING ? TRUE : nil;
}

Force list(const Value *args, std::uint8_t n)
{
    return create_list(args, n);
}

Force vector(const Value *args, std::uint8_t n)
{
    return create_array(args, n);
}

Force byte_array(const Value *args, std::uint8_t n)
{
    return create_byte_array(args, n);
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
        s = set_conj(*s, args[i]);
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

Force apply_wrapped(const Value *args, std::uint8_t n)
{
    if (n < 2)
        throw_arity_error(APPLY, n);
    return apply(args, n);
}

Force gensym(const Value *args, std::uint8_t n)
{
    if (n > 1)
        throw_arity_error(GENSYM, n);
    std::ostringstream os;
    Root prefix{n > 0 ? print_str(args[0]) : nil};
    if (*prefix)
        os.write(get_string_ptr(*prefix), get_string_size(*prefix));
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
    check_type("x", x, type::Int64);
    return create_int64(~get_int64_value(x));
}

Force bit_and(Value x, Value y)
{
    check_type("x", x, type::Int64);
    check_type("y", y, type::Int64);
    return create_int64(get_int64_value(x) & get_int64_value(y));
}

Force bit_or(Value x, Value y)
{
    check_type("x", x, type::Int64);
    check_type("y", y, type::Int64);
    return create_int64(get_int64_value(x) | get_int64_value(y));
}

Force bit_xor(Value x, Value y)
{
    check_type("x", x, type::Int64);
    check_type("y", y, type::Int64);
    return create_int64(get_int64_value(x) ^ get_int64_value(y));
}

Force bit_and_not(Value x, Value y)
{
    check_type("x", x, type::Int64);
    check_type("y", y, type::Int64);
    return create_int64(get_int64_value(x) & ~get_int64_value(y));
}

Force bit_clear(Value x, Value n)
{
    check_type("x", x, type::Int64);
    check_type("n", n, type::Int64);
    return create_int64(get_int64_value(x) & ~(Int64(1) << (get_int64_value(n) & 0x3f)));
}

Force bit_set(Value x, Value n)
{
    check_type("x", x, type::Int64);
    check_type("n", n, type::Int64);
    return create_int64(get_int64_value(x) | (Int64(1) << (get_int64_value(n) & 0x3f)));
}

Force bit_flip(Value x, Value n)
{
    check_type("x", x, type::Int64);
    check_type("n", n, type::Int64);
    return create_int64(get_int64_value(x) ^ (Int64(1) << (get_int64_value(n) & 0x3f)));
}

Value bit_test(Value x, Value n)
{
    check_type("x", x, type::Int64);
    check_type("n", n, type::Int64);
    return (get_int64_value(x) & (Int64(1) << (get_int64_value(n) & 0x3f))) ? TRUE : nil;
}

Force bit_shift_left(Value x, Value n)
{
    check_type("x", x, type::Int64);
    check_type("n", n, type::Int64);
    return create_int64(std::uint64_t(get_int64_value(x)) << (get_int64_value(n) & 0x3f));
}

Force bit_shift_right(Value x, Value n)
{
    check_type("x", x, type::Int64);
    check_type("n", n, type::Int64);
    static_assert(Int64(-2) >> 1 == Int64(-1), "arithmetic right shift needed");
    return create_int64(get_int64_value(x) >> (get_int64_value(n) & 0x3f));
}

Force unsigned_bit_shift_right(Value x, Value n)
{
    check_type("x", x, type::Int64);
    check_type("n", n, type::Int64);
    return create_int64(std::uint64_t(get_int64_value(x)) >> (get_int64_value(n) & 0x3f));
}

Force nil_conj(Value, Value x)
{
    return create_list(&x, 1);
}

Force nil_assoc(Value, Value k, Value v)
{
    return map_assoc(*EMPTY_MAP, k, v);
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
    Root next{cons_next(c)};
    Root nc{call_multimethod1(*rt::count, *next)};
    return create_int64(1 + get_int64_value(*nc));
}

Force cons(Value elem, Value next)
{
    Root s{is_seq(next) ? next : call_multimethod1(*rt::seq, next)};
    return create_cons(elem, *s);
}

Value array_peek(Value v)
{
    auto size = get_array_size(v);
    return size > 0 ? get_array_elem_unchecked(v, size - 1) : nil;
}

Value transient_array_peek(Value v)
{
    auto size = get_transient_array_size(v);
    return size > 0 ? get_transient_array_elem(v, size - 1) : nil;
}

Force mk_keyword(Value val)
{
    auto t = get_value_tag(val);
    switch (t)
    {
        case tag::KEYWORD: return val;
        case tag::SYMBOL:
        {
            auto ns = get_symbol_namespace(val);
            auto name = get_symbol_name(val);
            return create_keyword(ns ? std::string(get_string_ptr(ns), get_string_size(ns)) : std::string(),
                                  std::string(get_string_ptr(name), get_string_size(name)));
        }
        case tag::UTF8STRING: return create_keyword(std::string(get_string_ptr(val), get_string_size(val)));
        default: return nil;
    }
}

Force mk_symbol(Value ns, Value name)
{
    if (ns)
        check_type("ns", ns, *type::UTF8String);
    check_type("name", name, *type::UTF8String);
    if (ns)
        return create_symbol(
            std::string(get_string_ptr(ns), get_string_size(ns)),
            std::string(get_string_ptr(name), get_string_size(name)));
    else
        return create_symbol(std::string(get_string_ptr(name), get_string_size(name)));
}

Force get_name(Value val)
{
    auto t = get_value_tag(val);
    switch (t)
    {
        case tag::KEYWORD: return get_keyword_name(val);
        case tag::SYMBOL: return get_symbol_name(val);
        case tag::UTF8STRING: return val;
        default: return nil;
    }
}

Force get_namespace(Value val)
{
    auto t = get_value_tag(val);
    switch (t)
    {
        case tag::KEYWORD: return get_keyword_namespace(val);
        case tag::SYMBOL: return get_symbol_namespace(val);
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

Force protocol(Value name)
{
    check_type("name", name, *type::Symbol);
    return create_protocol(name);
}

Force create_type(Value name, Value field_names, Value field_types)
{
    check_type("name", name, *type::Symbol);
    check_type("field-names", field_names, *type::Array);
    check_type("field-types", field_types, *type::Array);
    auto size = get_array_size(field_names);
    if (get_array_size(field_types) != size)
        throw_illegal_argument("mismatched field names/types");
    std::vector<Value> names(size);
    std::vector<Value> types(size);
    for (Int64 i = 0; i < size; ++i)
    {
        auto name = get_array_elem(field_names, i);
        auto type = get_array_elem(field_types, i);
        check_type("field-name", name, *type::Symbol);
        if (type && !get_value_type(type).is(*type::Protocol))
            check_type("field-type", type, *type::Type);
        names[i] = name;
        types[i] = type;
    }
    return create_static_object_type(name, names.data(), types.data(), names.size());
}

Force new_instance(const Value *args, std::uint8_t n)
{
    if (n < 1)
        throw_arity_error(NEW, n);
    auto type = args[0];
    check_type("type", type, *type::Type);
    if (!is_object_type_constructible(type) || get_object_type_field_count(type) != (n - 1))
        throw_illegal_argument("No matching constructor found for type " + to_string(type));
    for (Int64 i = 1; i < n; ++i)
        if (auto ftype = get_object_type_field_type(type, i - 1))
            if (!isa(get_value_type(args[i]), ftype))
                throw_illegal_argument("Mismatched " + to_string(type) + " ctor argument " + std::to_string(i - 1) + " type, expected: " + to_string(ftype) + ", got: " + to_string(args[i]));
    return create_object(type, args + 1, n - 1);
}

Value defmulti(Value name, Value dispatchFn, Value defaultDispatchVal)
{
    check_type("name", name, *type::Symbol);
    return define_multimethod(name, dispatchFn, defaultDispatchVal);
}

Value defmethod(Value mm, Value dispatchValue, Value fn)
{
    check_type("multifn", mm, *type::Multimethod);
    define_method(get_multimethod_name(mm), dispatchValue, fn);
    return nil;
}

Value get_namespaces()
{
    return *namespaces;
}

Force disasm_bytes(const vm::Byte *bytes, Int64 size, Value consts, Value vars)
{
    Root dbs{transient_array(*EMPTY_VECTOR)};
    auto p = bytes;
    auto endp = p + size;
    auto read_u16 = [](const vm::Byte *p) { return std::uint8_t(p[0]) | std::uint16_t(std::uint8_t(p[1])) << 8; };
    auto read_i16 = [=](const vm::Byte *p) { return std::int16_t(read_u16(p)); };
    auto hex_bytes = [](const vm::Byte *p, unsigned size)
    {
        std::ostringstream os;
        for (unsigned i = 0; i < size; ++i)
            os << std::hex << std::setfill('0') << std::setw(2) << unsigned(std::uint8_t(p[i]));
        return os.str();
    };
    auto mk = [&p, bytes, hex_bytes](const std::string& oc, unsigned size = 1, Value arg0 = *SENTINEL)
    {
        Root offset{create_int64(p - bytes)};
        Root bytes{create_string(hex_bytes(p, size))};
        std::array<Value, 4> s{{*offset, *bytes, create_symbol(oc), arg0}};
        std::uint32_t n = 4;
        if (arg0 == *SENTINEL)
            n = 3;
        return create_array(s.data(), n);
    };
    Root oc, x;
    while (p != endp)
    {
        switch (*p)
        {
        case vm::LDC:
            oc = mk("LDC", 3, get_array_elem(consts, read_u16(p + 1)));
            dbs = transient_array_conj(*dbs, *oc);
            p += 3;
            break;
        case vm::LDL:
            x = create_int64(read_i16(p + 1));
            oc = mk("LDL", 3, *x);
            dbs = transient_array_conj(*dbs, *oc);
            p += 3;
            break;
        case vm::LDDV:
            oc = mk("LDDV", 3, get_array_elem(vars, read_u16(p + 1)));
            dbs = transient_array_conj(*dbs, *oc);
            p += 3;
            break;
        case vm::LDV:
            oc = mk("LDV", 3, get_array_elem(vars, read_u16(p + 1)));
            dbs = transient_array_conj(*dbs, *oc);
            p += 3;
            break;
        case vm::LDDF:
            oc = mk("LDDF");
            dbs = transient_array_conj(*dbs, *oc);
            ++p;
            break;
        case vm::LDSF:
            x = create_int64(read_u16(p + 1));
            oc = mk("LDSF", 3, *x);
            dbs = transient_array_conj(*dbs, *oc);
            p += 3;
            break;
        case vm::LDCV:
            x = create_int64(read_u16(p + 1));
            oc = mk("LDCV", 3, *x);
            dbs = transient_array_conj(*dbs, *oc);
            p += 3;
            break;
        case vm::STL:
            x = create_int64(read_i16(p + 1));
            oc = mk("STL", 3, *x);
            dbs = transient_array_conj(*dbs, *oc);
            p += 3;
            break;
        case vm::STVV:
            oc = mk("STVV");
            dbs = transient_array_conj(*dbs, *oc);
            ++p;
            break;
        case vm::STVM:
            oc = mk("STVM");
            dbs = transient_array_conj(*dbs, *oc);
            ++p;
            break;
        case vm::STVB:
            oc = mk("STVB");
            dbs = transient_array_conj(*dbs, *oc);
            ++p;
            break;
        case vm::POP:
            oc = mk("POP");
            dbs = transient_array_conj(*dbs, *oc);
            ++p;
            break;
        case vm::BNIL:
            x = create_int64((p - bytes) + 3 + read_i16(p + 1));
            oc = mk("BNIL", 3, *x);
            dbs = transient_array_conj(*dbs, *oc);
            p += 3;
            break;
        case vm::BNNIL:
            x = create_int64((p - bytes) + 3 + read_i16(p + 1));
            oc = mk("BNNIL", 3, *x);
            dbs = transient_array_conj(*dbs, *oc);
            p += 3;
            break;
        case vm::BR:
            x = create_int64((p - bytes) + 3 + read_i16(p + 1));
            oc = mk("BR", 3, *x);
            dbs = transient_array_conj(*dbs, *oc);
            p += 3;
            break;
        case vm::CALL:
            x = create_int64(std::uint8_t(p[1]));
            oc = mk("CALL", 2, *x);
            dbs = transient_array_conj(*dbs, *oc);
            p += 2;
            break;
        case vm::APPLY:
            x = create_int64(std::uint8_t(p[1]));
            oc = mk("APPLY", 2, *x);
            dbs = transient_array_conj(*dbs, *oc);
            p += 2;
            break;
        case vm::CNIL:
            oc = mk("CNIL");
            dbs = transient_array_conj(*dbs, *oc);
            ++p;
            break;
        case vm::IFN:
            x = create_int64(std::uint8_t(p[1]));
            oc = mk("IFN", 2, *x);
            dbs = transient_array_conj(*dbs, *oc);
            p += 2;
            break;
        case vm::THROW:
            oc = mk("THROW");
            dbs = transient_array_conj(*dbs, *oc);
            ++p;
            break;
        case vm::UBXI64:
            oc = mk("UBXI64");
            dbs = transient_array_conj(*dbs, *oc);
            ++p;
            break;
        case vm::BXI64:
            oc = mk("BXI64");
            dbs = transient_array_conj(*dbs, *oc);
            ++p;
            break;
        case vm::ADDI64:
            oc = mk("ADDI64");
            dbs = transient_array_conj(*dbs, *oc);
            ++p;
            break;
        case vm::NOT:
            oc = mk("NOT");
            dbs = transient_array_conj(*dbs, *oc);
            ++p;
            break;
        default:
            throw_illegal_argument("Invalid opcode: " + std::to_string(*p));
        }
    }
    return transient_array_persistent(*dbs);
}

Force disasm_exception_table(Value et)
{
    if (!et)
        return nil;
    Root det{transient_array(*EMPTY_VECTOR)};
    for (Int64 i = 0; i < get_bytecode_fn_exception_table_size(et); ++i)
    {
        Root soff{create_int64(get_bytecode_fn_exception_table_start_offset(et, i))};
        Root eoff{create_int64(get_bytecode_fn_exception_table_end_offset(et, i))};
        Root hoff{create_int64(get_bytecode_fn_exception_table_handler_offset(et, i))};
        Root ssize{create_int64(get_bytecode_fn_exception_table_stack_size(et, i))};
        Root entry{*EMPTY_MAP};
        entry = map_assoc(*entry, create_keyword("start-offset"), *soff);
        entry = map_assoc(*entry, create_keyword("end-offset"), *eoff);
        entry = map_assoc(*entry, create_keyword("handler-offset"), *hoff);
        entry = map_assoc(*entry, create_keyword("stack-size"), *ssize);
        entry = map_assoc(*entry, create_keyword("type"), get_bytecode_fn_exception_table_type(et, i));
        det = transient_array_conj(*det, *entry);
    }
    return transient_array_persistent(*det);
}

Force disasm(Value fn)
{
    if (get_value_type(fn).is(*type::Var))
        fn = get_var_value(fn);
    check_kind("fn", fn, *type::OpenBytecodeFn);
    Root dfn{*EMPTY_MAP};
    dfn = map_assoc(*dfn, create_keyword("name"), get_bytecode_fn_name(fn));
    Root bodies{*EMPTY_VECTOR};
    Root fns{*EMPTY_VECTOR};
    for (Int64 i = 0; i < get_bytecode_fn_size(fn); ++i)
    {
        Root arity{create_int64(get_bytecode_fn_arity(fn, i))};
        Root dbody{*EMPTY_MAP};
        auto body = get_bytecode_fn_body(fn, i);
        Root locals_size{create_int64(get_bytecode_fn_body_locals_size(body))};
        dbody = map_assoc(*dbody, create_keyword("arity"), *arity);
        dbody = map_assoc(*dbody, create_keyword("locals-size"), *locals_size);
        auto consts = get_bytecode_fn_body_consts(body);
        Root dbs{disasm_bytes(get_bytecode_fn_body_bytes(body), get_bytecode_fn_body_bytes_size(body), consts, get_bytecode_fn_body_vars(body))};
        dbody = map_assoc(*dbody, create_keyword("bytecode"), *dbs);
        Root det{disasm_exception_table(get_bytecode_fn_body_exception_table(body))};
        if (*det)
            dbody = map_assoc(*dbody, create_keyword("exception-table"), *det);
        bodies = array_conj(*bodies, *dbody);
        for (Int64 j = 0; j < get_array_size(consts); ++j)
            if (isa(get_value_type(get_array_elem(consts, j)), *type::OpenBytecodeFn))
                fns = array_conj(*fns, get_array_elem(consts, j));
    }
    dfn = map_assoc(*dfn, create_keyword("bodies"), *bodies);
    dfn = map_assoc(*dfn, create_keyword("fns"), *fns);
    return *dfn;
}

Force get_bytecode_fn_body(Value fn, Value arity)
{
    check_kind("fn", fn, *type::OpenBytecodeFn);
    check_type("arity", arity, type::Int64);
    auto body_arity = bytecode_fn_find_body(fn, get_int64_value(arity));
    if (!body_arity.first || body_arity.second != get_int64_value(arity))
        return nil;
    Root bytes{transient_array(*EMPTY_VECTOR)}, b;
    auto p = get_bytecode_fn_body_bytes(body_arity.first);
    auto endp = p + get_bytecode_fn_body_bytes_size(body_arity.first);
    for (; p < endp; ++p)
    {
        b = create_int64(std::uint8_t(*p));
        bytes = transient_array_conj(*bytes, *b);
    }
    bytes = transient_array_persistent(*bytes);
    return *bytes;
}

Force get_bytecode_fn_consts(Value fn, Value arity)
{
    check_kind("fn", fn, *type::OpenBytecodeFn);
    check_type("arity", arity, type::Int64);
    auto body_arity = bytecode_fn_find_body(fn, get_int64_value(arity));
    if (!body_arity.first || body_arity.second != get_int64_value(arity))
        return nil;
    return get_bytecode_fn_body_consts(body_arity.first);
}

Force get_bytecode_fn_vars(Value fn, Value arity)
{
    check_kind("fn", fn, *type::OpenBytecodeFn);
    check_type("arity", arity, type::Int64);
    auto body_arity = bytecode_fn_find_body(fn, get_int64_value(arity));
    if (!body_arity.first || body_arity.second != get_int64_value(arity))
        return nil;
    return get_bytecode_fn_body_vars(body_arity.first);
}

Force get_bytecode_fn_locals_size(Value fn, Value arity)
{
    check_kind("fn", fn, *type::OpenBytecodeFn);
    check_type("arity", arity, type::Int64);
    auto body_arity = bytecode_fn_find_body(fn, get_int64_value(arity));
    if (!body_arity.first || body_arity.second != get_int64_value(arity))
        return nil;
    return create_int64(get_bytecode_fn_body_locals_size(body_arity.first));
}

Value meta(Value x)
{
    auto type = get_value_type(x);
    if (type.is(*type::Var))
        return get_var_meta(x);
    if (type.is(*type::Namespace))
        return get_ns_meta(x);
    return nil;
}

Value the_ns(Value ns)
{
    if (get_value_type(ns).is(*type::Namespace))
        return ns;
    return get_ns(ns);
}

Force subs(Value s, Value start)
{
    check_type("s", s, *type::UTF8String);
    check_type("start", start, type::Int64);
    auto start_ = get_int64_value(start);
    if (start_ < 0 ||
        start_ > get_string_len(s))
        throw_illegal_argument("Invalid substring start: " + std::to_string(start_));
    auto start_offset = get_string_char_offset(s, start_);
    return create_string(get_string_ptr(s) + start_offset, get_string_size(s) - start_offset);
}

Force subs(Value s, Value start, Value end)
{
    check_type("s", s, *type::UTF8String);
    check_type("start", start, type::Int64);
    auto start_ = get_int64_value(start);
    auto end_ = get_int64_value(end);
    if (start_ < 0 ||
        end_ < 0 ||
        start_ > get_string_len(s) ||
        end_ > get_string_len(s) ||
        start_ > end_)
        throw_illegal_argument("Invalid substring bounds: " + std::to_string(start_) + " " + std::to_string(end_));
    auto start_offset = get_string_char_offset(s, start_);
    auto end_offset = get_string_char_offset(s, end_);
    return create_string(get_string_ptr(s) + start_offset, end_offset - start_offset);
}

Value string_get(Value s, Value idx, Value def)
{
    check_type("s", s, *type::UTF8String);
    if (get_value_tag(idx) != tag::INT64)
        return def;
    Int64 i = get_int64_value(idx);
    if (i < 0 || i >= get_string_len(s))
        return def;
    return create_uchar(get_string_char(s, i));
}

Value string_get(Value s, Value idx)
{
    return string_get(s, idx, nil);
}

Value define_var_(Value name, Value meta)
{
    return define(name, nil, meta);
}

Value create_char(Value val)
{
    check_type("char", val, type::Int64);
    Int64 c = get_int64_value(val);
    if (c < 0 || c >= 0x110000)
        throw_illegal_argument("Character value out of range for UTF-8: " + std::to_string(c));
    return create_uchar(c);
}

Value str_starts_with(Value s, Value ss)
{
    check_type("s", s, *type::UTF8String);
    check_type("ss", ss, *type::UTF8String);
    return std::strncmp(get_string_ptr(s), get_string_ptr(ss), get_string_size(ss)) == 0 ? TRUE : nil;
}

Force sort_transient(Value pred, Value array)
{
    check_type("array", array, *type::TransientArray);
    auto size = get_transient_array_size(array);
    std::vector<Value> vals;
    vals.reserve(size);
    for (decltype(size) i = 0; i != size; ++i)
        vals.push_back(get_transient_array_elem(array, i));
    std::sort(begin(vals), end(vals),
              [pred](Value l, Value r)
                  {
                      Value predlr[] = {pred, l, r};
                      return !call(predlr, 3).value().is_nil();
                  });
    Root ret{array};
    for (decltype(size) i = 0; i != size; ++i)
        ret = transient_array_assoc_elem(*ret, i, vals[i]);
    return *ret;
}

Value derive_type(Value x, Value parent)
{
    derive(x, parent);
    return nil;
}

Force get_type_field_index(Value type, Value field)
{
    check_type("type", type, *type::Type);
    check_type("field", field, *type::Symbol);
    auto index = get_object_field_index(type, field);
    return index < 0 ? nil : create_int64(index);
}

Force current_callstack_fn()
{
    return create_array(prof::callstack, prof::callstack_size - 1);
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

void define_function(Value name, Force f, Value meta = nil)
{
    Root fr{f};
    define(name, *fr, meta);
}

struct Initialize
{
    Initialize()
    {
        stack.reserve(1048576);
        int_stack.reserve(1048576);
        Root core_meta{*EMPTY_MAP};
        Root core_doc{create_string("The core Cleo library")};
        core_meta = map_assoc(*core_meta, create_keyword("doc"), *core_doc);
        define_ns(CLEO_CORE, *core_meta);

        create_global_hierarchy();

        define_type(*type::Type);
        define_type(*type::True);
        define_type(*type::Protocol);
        define_type(type::Int64);
        define_type(*type::Float64);
        define_type(*type::UTF8String);
        define_type(*type::NativeFunction);
        define_type(*type::CFunction);
        define_type(*type::Keyword);
        define_type(*type::Symbol);
        define_type(*type::Var);
        define_type(*type::List);
        define_type(*type::Cons);
        define_type(*type::LazySeq);
        define_protocol(*type::PersistentVector);
        define_type(*type::Array);
        derive(*type::Array, *type::PersistentVector);
        define_type(*type::ArraySeq);
        define_type(*type::ByteArray);
        derive(*type::ByteArray, *type::PersistentVector);
        define_type(*type::ByteArraySeq);
        define_type(*type::TransientByteArray);
        define_type(*type::ArrayMap);
        define_type(*type::ArrayMapSeq);
        define_protocol(*type::PersistentSet);
        define_type(*type::ArraySet);
        derive(*type::ArraySet, *type::PersistentSet);
        define_type(*type::ArraySetSeq);
        define_type(*type::Hierarchy);
        define_type(*type::Multimethod);
        define_protocol(*type::Seqable);
        derive(*type::PersistentVector, *type::Seqable);
        define_protocol(*type::Sequence);
        define_protocol(*type::Callable);
        define_type(*type::OpenBytecodeFn);
        define_type(*type::BytecodeFn);
        derive(*type::BytecodeFn, *type::OpenBytecodeFn);
        define_type(*type::BytecodeFnBody);
        define_type(*type::BytecodeFnExceptionTable);
        define_type(*type::Atom);
        define_protocol(*type::PersistentMap);
        define_type(*type::PersistentHashMap);
        define_type(*type::PersistentHashMapCollisionNode);
        define_type(*type::PersistentHashMapArrayNode);
        define_type(*type::PersistentHashMapSeqParent);
        define_type(*type::PersistentHashSet);
        define_type(*type::PersistentHashSetCollisionNode);
        define_type(*type::PersistentHashSetArrayNode);
        define_type(*type::PersistentHashSetSeqParent);
        derive(*type::PersistentHashSet, *type::PersistentSet);
        define_type(*type::Exception);
        define_type(*type::LogicException);
        derive(*type::LogicException, *type::Exception);
        define_type(*type::CastError);
        derive(*type::CastError, *type::LogicException);
        define_type(*type::ReadError);
        derive(*type::ReadError, *type::Exception);
        define_type(*type::UnexpectedEndOfInput);
        derive(*type::UnexpectedEndOfInput, *type::ReadError);
        define_type(*type::CallError);
        derive(*type::CallError, *type::LogicException);
        define_type(*type::SymbolNotFound);
        derive(*type::SymbolNotFound, *type::Exception);
        define_type(*type::IllegalArgument);
        derive(*type::IllegalArgument, *type::LogicException);
        define_type(*type::IllegalState);
        derive(*type::IllegalState, *type::LogicException);
        define_type(*type::FileNotFound);
        derive(*type::FileNotFound, *type::Exception);
        define_type(*type::ArithmeticException);
        derive(*type::ArithmeticException, *type::LogicException);
        define_type(*type::IndexOutOfBounds);
        derive(*type::IndexOutOfBounds, *type::LogicException);
        define_type(*type::CompilationError);
        derive(*type::CompilationError, *type::Exception);
        define_type(*type::StackOverflow);
        derive(*type::StackOverflow, *type::LogicException);

        define_type(*type::Namespace);
        define_type(*type::TransientArray);

        define(SHOULD_RECOMPILE, TRUE);

        define_function(PROTOCOL, create_native_function1<protocol, &PROTOCOL>());
        define_function(CREATE_TYPE, create_native_function3<create_type, &CREATE_TYPE>());

        define_multimethod(HASH_OBJ, *first_type, nil);
        define_method(HASH_OBJ, nil, *ret_hash_zero);

        define_function(NEW, create_native_function(new_instance, NEW), *CONST_META);

        define_function(META, create_native_function1<meta, &META>());

        define_function(THE_NS, create_native_function1<the_ns, &THE_NS>());
        define_function(FIND_NS, create_native_function1<find_ns, &FIND_NS>());
        define_function(RESOLVE, create_native_function1<maybe_resolve_var, &RESOLVE>());

        define_function(SUBS, create_native_function2or3<subs, subs, &SUBS>());

        define_function(DEFINE_VAR, create_native_function2<define_var_, &DEFINE_VAR>());

        Root f;

        f = create_native_function1<array_hash, &HASH_OBJ>();
        define_method(HASH_OBJ, *type::Array, *f);

        define(CURRENT_NS, get_ns(CLEO_CORE), *DYNAMIC_META);
        f = create_native_function1or2<in_ns, in_ns, &IN_NS>();
        define(IN_NS, *f);

        define(LIB_PATHS, nil, *DYNAMIC_META);

        define(COMMAND_LINE_ARGS, nil, *DYNAMIC_META);

        f = create_native_function2<identical, &IDENTICAL>();
        define(IDENTICAL, *f);

        f = create_native_function2<isa, &ISA>();
        define(ISA, *f);

        f = create_native_function1<symbol_q, &SYMBOL_Q>();
        define(SYMBOL_Q, *f);

        f = create_native_function1<keyword_q, &KEYWORD_Q>();
        define(KEYWORD_Q, *f);

        f = create_native_function1<string_q, &STRING_Q>();
        define(STRING_Q, *f);

        f = create_native_function(list, LIST);
        define(LIST, *f);

        f = create_native_function(vector, VECTOR);
        define(VECTOR, *f);

        f = create_native_function(hash_map, HASH_MAP);
        define(HASH_MAP, *f);

        f = create_native_function(hash_set, HASH_SET);
        define(HASH_SET, *f);

        f = create_native_function(concati, CONCATI);
        define(CONCATI, *f);

        f = create_native_function1<mk_keyword, &KEYWORD>();
        define(KEYWORD, *f);

        f = create_native_function1<get_name, &NAME>();
        define(NAME, *f);

        f = create_native_function1<get_namespace, &NAMESPACE>();
        define(NAMESPACE, *f);

        f = create_native_function2<mk_symbol, &SYMBOL>();
        define(SYMBOL, *f);

        auto undefined = create_symbol("cleo.core/-UNDEFINED-");
        define_multimethod(SEQ, *first_type, undefined);
        define_multimethod(FIRST, *first_type, undefined);
        define_multimethod(NEXT, *first_type, undefined);
        define_multimethod(PEEK, *first_type, undefined);
        define_multimethod(POP, *first_type, undefined);

        f = create_native_function1<nil_seq, &SEQ>();
        define_method(SEQ, nil, *f);
        f = create_native_function1<nil_seq, &FIRST>();
        define_method(FIRST, nil, *f);
        f = create_native_function1<nil_seq, &NEXT>();
        define_method(NEXT, nil, *f);

        derive(*type::List, *type::Sequence);
        f = create_native_function1<list_seq, &SEQ>();
        define_method(SEQ, *type::List, *f);
        f = create_native_function1<get_list_first, &FIRST>();
        define_method(FIRST, *type::List, *f);
        f = create_native_function1<get_list_next, &NEXT>();
        define_method(NEXT, *type::List, *f);
        f = create_native_function1<get_list_first, &PEEK>();
        define_method(PEEK, *type::List, *f);
        f = create_native_function1<get_list_next, &POP>();
        define_method(POP, *type::List, *f);

        derive(*type::Cons, *type::Sequence);
        f = create_native_function1<identity, &SEQ>();
        define_method(SEQ, *type::Cons, *f);
        f = create_native_function1<cons_first, &FIRST>();
        define_method(FIRST, *type::Cons, *f);
        f = create_native_function1<cons_next, &NEXT>();
        define_method(NEXT, *type::Cons, *f);

        derive(*type::LazySeq, *type::Sequence);
        f = create_native_function1<lazy_seq_seq, &SEQ>();
        define_method(SEQ, *type::LazySeq, *f);
        f = create_native_function1<lazy_seq_first, &FIRST>();
        define_method(FIRST, *type::LazySeq, *f);
        f = create_native_function1<lazy_seq_next, &NEXT>();
        define_method(NEXT, *type::LazySeq, *f);

        f = create_native_function1<array_seq, &SEQ>();
        define_method(SEQ, *type::Array, *f);
        f = create_native_function1<get_array_seq_first, &FIRST>();
        define_method(FIRST, *type::ArraySeq, *f);
        f = create_native_function1<get_array_seq_next, &NEXT>();
        define_method(NEXT, *type::ArraySeq, *f);
        f = create_native_function1<array_peek, &PEEK>();
        define_method(PEEK, *type::Array, *f);
        f = create_native_function1<array_pop, &POP>();
        define_method(POP, *type::Array, *f);

        f = create_native_function(byte_array, BYTE_ARRAY);
        define(BYTE_ARRAY, *f);
        f = create_native_function1<byte_array_seq, &SEQ>();
        define_method(SEQ, *type::ByteArray, *f);
        f = create_native_function1<get_byte_array_seq_first, &FIRST>();
        define_method(FIRST, *type::ByteArraySeq, *f);
        f = create_native_function1<get_byte_array_seq_next, &NEXT>();
        define_method(NEXT, *type::ByteArraySeq, *f);
        f = create_native_function1<byte_array_pop, &POP>();
        define_method(POP, *type::ByteArray, *f);

        f = create_native_function1<transient_array_peek, &PEEK>();
        define_method(PEEK, *type::TransientArray, *f);

        derive(*type::ArraySet, *type::Seqable);
        f = create_native_function1<array_set_seq, &SEQ>();
        define_method(SEQ, *type::ArraySet, *f);
        f = create_native_function1<get_array_set_seq_first, &FIRST>();
        define_method(FIRST, *type::ArraySetSeq, *f);
        f = create_native_function1<get_array_set_seq_next, &NEXT>();
        define_method(NEXT, *type::ArraySetSeq, *f);

        derive(*type::ArrayMap, *type::Seqable);
        f = create_native_function1<array_map_seq, &SEQ>();
        define_method(SEQ, *type::ArrayMap, *f);
        f = create_native_function1<get_array_map_seq_first, &FIRST>();
        define_method(FIRST, *type::ArrayMapSeq, *f);
        f = create_native_function1<get_array_map_seq_next, &NEXT>();
        define_method(NEXT, *type::ArrayMapSeq, *f);

        derive(*type::PersistentHashMap, *type::Seqable);
        f = create_native_function1<persistent_hash_map_seq, &SEQ>();
        define_method(SEQ, *type::PersistentHashMap, *f);
        f = create_native_function1<get_persistent_hash_map_seq_first, &FIRST>();
        define_method(FIRST, *type::PersistentHashMapSeq, *f);
        f = create_native_function1<get_persistent_hash_map_seq_next, &NEXT>();
        define_method(NEXT, *type::PersistentHashMapSeq, *f);

        derive(*type::PersistentHashSet, *type::Seqable);
        f = create_native_function1<persistent_hash_set_seq, &SEQ>();
        define_method(SEQ, *type::PersistentHashSet, *f);
        f = create_native_function1<get_persistent_hash_set_seq_first, &FIRST>();
        define_method(FIRST, *type::PersistentHashSetSeq, *f);
        f = create_native_function1<get_persistent_hash_set_seq_next, &NEXT>();
        define_method(NEXT, *type::PersistentHashSetSeq, *f);

        derive(*type::UTF8String, *type::Seqable);
        f = create_native_function1<string_seq, &SEQ>();
        define_method(SEQ, *type::UTF8String, *f);
        f = create_native_function1<string_seq_first, &FIRST>();
        define_method(FIRST, *type::UTF8StringSeq, *f);
        f = create_native_function1<string_seq_next, &NEXT>();
        define_method(NEXT, *type::UTF8StringSeq, *f);

        derive(*type::ArraySeq, *type::Sequence);
        derive(*type::ByteArraySeq, *type::Sequence);
        derive(*type::ArraySetSeq, *type::Sequence);
        derive(*type::ArrayMapSeq, *type::Sequence);
        derive(*type::PersistentHashMapSeq, *type::Sequence);
        derive(*type::PersistentHashSetSeq, *type::Sequence);
        derive(*type::UTF8StringSeq, *type::Sequence);
        derive(*type::Sequence, *type::Seqable);
        f = create_native_function1<identity, &SEQ>();
        define_method(SEQ, *type::Sequence, *f);

        f = create_native_function1<get_seqable_first, &FIRST>();
        define_method(FIRST, *type::Seqable, *f);
        f = create_native_function1<get_seqable_next, &NEXT>();
        define_method(NEXT, *type::Seqable, *f);

        define_multimethod(COUNT, *first_type, undefined);
        f = create_native_function1<WrapUInt32Fn<get_array_map_size>::fn, &COUNT>();
        define_method(COUNT, *type::ArrayMap, *f);
        f = create_native_function1<WrapInt64Fn<get_persistent_hash_map_size>::fn, &COUNT>();
        define_method(COUNT, *type::PersistentHashMap, *f);
        f = create_native_function1<WrapUInt32Fn<get_array_set_size>::fn, &COUNT>();
        define_method(COUNT, *type::ArraySet, *f);
        f = create_native_function1<WrapInt64Fn<get_persistent_hash_set_size>::fn, &COUNT>();
        define_method(COUNT, *type::PersistentHashSet, *f);
        f = create_native_function1<WrapInt64Fn<get_list_size>::fn, &COUNT>();
        define_method(COUNT, *type::List, *f);
        f = create_native_function1<cons_size, &COUNT>();
        define_method(COUNT, *type::Cons, *f);
        f = create_native_function1<seq_count, &COUNT>();
        define_method(COUNT, *type::Sequence, *f);
        f = create_native_function1<WrapUInt32Fn<get_array_size>::fn, &COUNT>();
        define_method(COUNT, *type::Array, *f);
        f = create_native_function1<WrapInt64Fn<get_byte_array_size>::fn, &COUNT>();
        define_method(COUNT, *type::ByteArray, *f);
        f = create_native_function1<WrapInt64Fn<get_transient_array_size>::fn, &COUNT>();
        define_method(COUNT, *type::TransientArray, *f);
        f = create_native_function1<WrapInt64Fn<get_transient_byte_array_size>::fn, &COUNT>();
        define_method(COUNT, *type::TransientByteArray, *f);
        f = create_native_function1<WrapUInt32Fn<get_string_len>::fn, &COUNT>();
        define_method(COUNT, *type::UTF8String, *f);
        f = create_native_function1<nil_count, &COUNT>();
        define_method(COUNT, nil, *f);

        define_multimethod(GET, *first_type, undefined);

        f = create_native_function2or3<array_map_get, array_map_get, &GET>();
        define_method(GET, *type::ArrayMap, *f);

        f = create_native_function2or3<persistent_hash_map_get, persistent_hash_map_get, &GET>();
        define_method(GET, *type::PersistentHashMap, *f);

        f = create_native_function2or3<array_set_get, array_set_get, &GET>();
        define_method(GET, *type::ArraySet, *f);

        f = create_native_function2or3<persistent_hash_set_get, persistent_hash_set_get, &GET>();
        define_method(GET, *type::PersistentHashSet, *f);

        f = create_native_function2<array_get, &GET>();
        define_method(GET, *type::Array, *f);

        f = create_native_function2<transient_array_get, &GET>();
        define_method(GET, *type::TransientArray, *f);

        f = create_native_function2<byte_array_get, &GET>();
        define_method(GET, *type::ByteArray, *f);

        f = create_native_function2<transient_byte_array_get, &GET>();
        define_method(GET, *type::TransientByteArray, *f);

        f = create_native_function2or3<nil_get, nil_get, &GET>();
        define_method(GET, nil, *f);

        f = create_native_function2or3<string_get, string_get, &GET>();
        define_method(GET, *type::UTF8String, *f);

        define_multimethod(CONTAINS, *first_type, undefined);

        f = create_native_function2<array_map_contains, &CONTAINS>();
        define_method(CONTAINS, *type::ArrayMap, *f);

        f = create_native_function2<persistent_hash_map_contains, &CONTAINS>();
        define_method(CONTAINS, *type::PersistentHashMap, *f);

        f = create_native_function2<array_set_contains, &CONTAINS>();
        define_method(CONTAINS, *type::ArraySet, *f);

        f = create_native_function2<persistent_hash_set_contains, &CONTAINS>();
        define_method(CONTAINS, *type::PersistentHashSet, *f);

        f = create_native_function2<nil_contains, &CONTAINS>();
        define_method(CONTAINS, nil, *f);

        define_multimethod(CONJ, *first_type, undefined);

        f = create_native_function2<array_conj, &CONJ>();
        define_method(CONJ, *type::Array, *f);

        f = create_native_function2<byte_array_conj, &CONJ>();
        define_method(CONJ, *type::ByteArray, *f);

        f = create_native_function2<cons_conj, &CONJ>();
        define_method(CONJ, *type::ArraySeq, *f);

        define_method(CONJ, *type::ArraySet, *rt::set_conj);

        f = create_native_function2<persistent_hash_set_conj, &CONJ>();
        define_method(CONJ, *type::PersistentHashSet, *f);

        f = create_native_function2<list_conj, &CONJ>();
        define_method(CONJ, *type::List, *f);

        f = create_native_function2<cons_conj, &CONJ>();
        define_method(CONJ, *type::Cons, *f);

        f = create_native_function2<lazy_seq_conj, &CONJ>();
        define_method(CONJ, *type::LazySeq, *f);

        f = create_native_function2<nil_conj, &CONJ>();
        define_method(CONJ, nil, *f);

        f = create_native_function2<cons, &CONS>();
        define(CONS, *f);

        f = create_native_function1<create_lazy_seq, &LAZY_SEQ>();
        define(LAZY_SEQ, *f);

        define_multimethod(ASSOC, *first_type, undefined);

        derive(*type::ArrayMap, *type::PersistentMap);
        f = create_native_function3<map_assoc, &ASSOC>();
        define_method(ASSOC, *type::ArrayMap, *f);

        derive(*type::PersistentHashMap, *type::PersistentMap);
        f = create_native_function3<persistent_hash_map_assoc, &ASSOC>();
        define_method(ASSOC, *type::PersistentHashMap, *f);

        define_multimethod(ASSOC_E, *first_type, undefined);

        f = create_native_function3<transient_array_assoc, &ASSOC_E>();
        define_method(ASSOC_E, *type::TransientArray, *f);
        f = create_native_function3<transient_byte_array_assoc, &ASSOC_E>();
        define_method(ASSOC_E, *type::TransientByteArray, *f);

        f = create_native_function3<nil_assoc, &ASSOC>();
        define_method(ASSOC, nil, *f);

        define_multimethod(DISSOC, *first_type, undefined);

        f = create_native_function2<array_map_dissoc, &DISSOC>();
        define_method(DISSOC, *type::ArrayMap, *f);

        f = create_native_function2<persistent_hash_map_dissoc, &DISSOC>();
        define_method(DISSOC, *type::PersistentHashMap, *f);

        f = create_native_function2<nil_dissoc, &DISSOC>();
        define_method(DISSOC, nil, *f);

        f = create_native_function0<create_array_map, &ARRAY_MAP>();
        define(ARRAY_MAP, *f);

        define_multimethod(MERGE, *equal_dispatch, nil);
        f = create_native_function2<merge_maps, &MERGE>();

        std::array<Value, 2> two_maps{{*type::PersistentMap, *type::PersistentMap}};
        Root v{create_array(two_maps.data(), two_maps.size())};
        define_method(MERGE, *v, *f);

        std::array<Value, 2> two_array_maps{{*type::ArrayMap, *type::ArrayMap}};
        v = create_array(two_array_maps.data(), two_array_maps.size());
        f = create_native_function2<array_map_merge, &MERGE>();
        define_method(MERGE, *v, *f);

        define_multimethod(OBJ_CALL, *first_type, nil);

        derive(*type::ArraySet, *type::Callable);
        f = create_native_function2or3<array_set_get, array_set_get, &OBJ_CALL>();
        define_method(OBJ_CALL, *type::ArraySet, *f);

        derive(*type::ArrayMap, *type::Callable);
        f = create_native_function2or3<array_map_get, array_map_get, &OBJ_CALL>();
        define_method(OBJ_CALL, *type::ArrayMap, *f);

        derive(*type::PersistentHashMap, *type::Callable);
        f = create_native_function2or3<persistent_hash_map_get, persistent_hash_map_get, &OBJ_CALL>();
        define_method(OBJ_CALL, *type::PersistentHashMap, *f);

        derive(*type::Keyword, *type::Callable);
        f = create_native_function2<keyword_get, &KEYWORD_TYPE_NAME>();
        define_method(OBJ_CALL, *type::Keyword, *f);

        derive(*type::Array, *type::Callable);
        f = create_native_function2<array_call, &OBJ_CALL>();
        define_method(OBJ_CALL, *type::Array, *f);

        derive(*type::ByteArray, *type::Callable);
        f = create_native_function2<byte_array_call, &OBJ_CALL>();
        define_method(OBJ_CALL, *type::ByteArray, *f);

        derive(*type::TransientArray, *type::Callable);
        f = create_native_function2<transient_array_call, &OBJ_CALL>();
        define_method(OBJ_CALL, *type::TransientArray, *f);

        derive(*type::TransientByteArray, *type::Callable);
        f = create_native_function2<transient_byte_array_call, &OBJ_CALL>();
        define_method(OBJ_CALL, *type::TransientByteArray, *f);

        derive(*type::CFunction, *type::Callable);
        f = create_native_function(call_c_function, OBJ_CALL);
        define_method(OBJ_CALL, *type::CFunction, *f);

        derive(*type::Var, *type::Callable);
        f = create_native_function(var_call, OBJ_CALL);
        define_method(OBJ_CALL, *type::Var, *f);

        define_multimethod(OBJ_EQ, *equal_dispatch, nil);
        define_method(OBJ_EQ, nil, *ret_eq_nil);

        auto define_seq_eq = [&](auto type1, auto type2)
        {
            std::array<Value, 2> two{{type1, type2}};
            Root rtwo{create_array(two.data(), two.size())};
            Root f{create_native_function2<are_seqables_equal, &OBJ_EQ>()};
            define_method(OBJ_EQ, *rtwo, *f);
            if (type1.is(type2))
                return;
            std::array<Value, 2> two_rev{{type2, type1}};
            Root rtwo_rev{create_array(two_rev.data(), two_rev.size())};
            define_method(OBJ_EQ, *rtwo_rev, *f);
        };

        define_seq_eq(*type::Array, *type::Array);
        define_seq_eq(*type::Array, *type::ByteArray);
        define_seq_eq(*type::Array, *type::List);
        define_seq_eq(*type::Array, *type::Sequence);
        define_seq_eq(*type::ByteArray, *type::ByteArray);
        define_seq_eq(*type::ByteArray, *type::List);
        define_seq_eq(*type::ByteArray, *type::Sequence);
        define_seq_eq(*type::Sequence, *type::List);
        define_seq_eq(*type::Sequence, *type::Sequence);
        define_seq_eq(*type::List, *type::List);

        std::array<Value, 2> two_array_sets{{*type::ArraySet, *type::ArraySet}};
        v = create_array(two_array_sets.data(), two_array_sets.size());
        f = create_native_function2<are_array_sets_equal, &OBJ_EQ>();
        define_method(OBJ_EQ, *v, *f);

        std::array<Value, 2> two_hash_sets{{*type::PersistentHashSet, *type::PersistentHashSet}};
        v = create_array(two_hash_sets.data(), two_hash_sets.size());
        f = create_native_function2<are_persistent_hash_sets_equal, &OBJ_EQ>();
        define_method(OBJ_EQ, *v, *f);

        v = create_array(two_array_maps.data(), two_array_maps.size());
        f = create_native_function2<are_array_maps_equal, &OBJ_EQ>();
        define_method(OBJ_EQ, *v, *f);

        std::array<Value, 2> two_hash_maps{{*type::PersistentHashMap, *type::PersistentHashMap}};
        v = create_array(two_hash_maps.data(), two_hash_maps.size());
        f = create_native_function2<are_persistent_hash_maps_equal, &OBJ_EQ>();
        define_method(OBJ_EQ, *v, *f);

        v = create_array(two_maps.data(), two_maps.size());
        f = create_native_function2<are_maps_equal, &OBJ_EQ>();
        define_method(OBJ_EQ, *v, *f);

        define(PRINT_READABLY, TRUE, *DYNAMIC_META);

        define_multimethod(PR_STR_OBJ, *first_type, nil);

        f = create_native_function1<pr_str_array, &PR_STR_OBJ>();
        define_method(PR_STR_OBJ, *type::Array, *f);
        f = create_native_function1<pr_str_array_set, &PR_STR_OBJ>();
        define_method(PR_STR_OBJ, *type::ArraySet, *f);
        f = create_native_function1<pr_str_persistent_hash_set, &PR_STR_OBJ>();
        define_method(PR_STR_OBJ, *type::PersistentHashSet, *f);
        f = create_native_function1<pr_str_array_map, &PR_STR_OBJ>();
        define_method(PR_STR_OBJ, *type::ArrayMap, *f);
        f = create_native_function1<pr_str_persistent_hash_map, &PR_STR_OBJ>();
        define_method(PR_STR_OBJ, *type::PersistentHashMap, *f);
        f = create_native_function1<pr_str_seqable, &PR_STR_OBJ>();
        define_method(PR_STR_OBJ, *type::Seqable, *f);
        f = create_native_function1<pr_str_vector, &PR_STR_OBJ>();
        define_method(PR_STR_OBJ, *type::PersistentVector, *f);
        f = create_native_function1<pr_str_object, &PR_STR_OBJ>();
        define_method(PR_STR_OBJ, nil, *f);

        f = create_native_function1<pr_str_var, &PR_STR_OBJ>();
        define_method(PR_STR_OBJ, *type::Var, *f);

        f = create_native_function1<pr_str_open_bytecode_fn, &PR_STR_OBJ>();
        define_method(PR_STR_OBJ, *type::OpenBytecodeFn, *f);

        f = create_native_function1<pr_str_multimethod, &PR_STR_OBJ>();
        define_method(PR_STR_OBJ, *type::Multimethod, *f);

        f = create_native_function2<add2, &INTERNAL_ADD_2>();
        define(INTERNAL_ADD_2, *f, *CONST_META);
        f = create_native_function(sub, MINUS);
        define(MINUS, *f);
        f = create_native_function2<mult2, &ASTERISK>();
        define(ASTERISK, *f);
        f = create_native_function2<quot, &QUOT>();
        define(QUOT, *f);
        f = create_native_function2<rem, &REM>();
        define(REM, *f);
        f = create_native_function2<lt2, &LT>();
        define(LT, *f);
        f = create_native_function2<are_equal, &EQ>();
        define(EQ, *f);

        f = create_native_function1<pr_str_exception, &PR_STR_OBJ>();
        define_method(PR_STR_OBJ, *type::Exception, *f);

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

        f = create_native_function2<require, &REQUIRE>();
        define(REQUIRE, *f);

        define_function(ALIAS, create_native_function2<alias, &ALIAS>());

        f = create_native_function1<create_atom, &ATOM>();
        define(ATOM, *f);

        define_multimethod(DEREF, *first_type, nil);
        f = create_native_function1<atom_deref, &DEREF>();
        define_method(DEREF, *type::Atom, *f);

        f = create_native_function1<get_var_value, &DEREF>();
        define_method(DEREF, *type::Var, *f);

        define_multimethod(RESET, *first_type, nil);
        f = create_native_function2<atom_reset, &RESET>();
        define_method(RESET, *type::Atom, *f);

        f = create_swap_fn();
        define(SWAP, *f);

        f = create_native_function(apply_wrapped, APPLY);
        define(APPLY, *f);

        f = create_native_function1<pr_str, &PR_STR>();
        define(PR_STR, *f);

        f = create_native_function(pr, create_symbol("cleo.core", "pr"));
        define(create_symbol("cleo.core", "pr"), *f);

        f = create_native_function(prn, create_symbol("cleo.core", "prn"));
        define(create_symbol("cleo.core", "prn"), *f);

        f = create_native_function(print, create_symbol("cleo.core", "print"));
        define(create_symbol("cleo.core", "print"), *f);

        f = create_native_function(println, create_symbol("cleo.core", "println"));
        define(create_symbol("cleo.core", "println"), *f);

        define_function(STR, create_native_function(str, STR));

        f = create_native_function1<get_value_type, &TYPE>();
        define(TYPE, *f);

        f = create_ns_macro();
        define(NS, *f, *MACRO_META);

        f = create_native_function4<import_c_fn, &IMPORT_C_FN>();
        define(IMPORT_C_FN, *f);

        f = create_native_function(gensym, GENSYM);
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
        define_function(NAMESPACES, create_native_function0<get_namespaces, &NAMESPACES>());

        define_function(VAR_NAME, create_native_function1<get_var_name, &VAR_NAME>());

        define_function(CURRENT_CALLSTACK, create_native_function0<current_callstack_fn, &CURRENT_CALLSTACK>());

        define_function(GC_LOG, create_native_function1<set_gc_log, &GC_LOG>());

        define_function(GET_TIME, create_native_function0<get_time, &GET_TIME>());

        define_multimethod(CONJ_E, *first_type, undefined);

        define_function(TRANSIENT_VECTOR, create_native_function1<transient_array, &TRANSIENT_VECTOR>(), *CONST_META);
        define_function(PERSISTENT_VECTOR, create_native_function1<transient_array_persistent, &PERSISTENT_VECTOR>(), *CONST_META);
        define_function(TRANSIENT_VECTOR_ASSOC, create_native_function3<transient_array_assoc, &TRANSIENT_VECTOR_ASSOC>(), *CONST_META);

        define_method(CONJ_E, *type::TransientArray, *rt::transient_array_conj);
        f = create_native_function2<transient_byte_array_conj, &CONJ_E>();
        define_method(CONJ_E, *type::TransientByteArray, *f);

        define_multimethod(POP_E, *first_type, undefined);

        define_method(POP_E, *type::TransientArray, *rt::transient_array_pop);
        f = create_native_function1<transient_byte_array_pop, &POP_E>();
        define_method(POP_E, *type::TransientByteArray, *f);

        define_multimethod(TRANSIENT, *first_type, undefined);

        define_method(TRANSIENT, *type::Array, *rt::transient_array);
        f = create_native_function1<transient_byte_array, &TRANSIENT>();
        define_method(TRANSIENT, *type::ByteArray, *f);

        define_multimethod(PERSISTENT, *first_type, undefined);

        define_method(PERSISTENT, *type::TransientArray, *rt::transient_array_persistent);
        f = create_native_function1<transient_byte_array_persistent, &PERSISTENT>();
        define_method(PERSISTENT, *type::TransientByteArray, *f);

        define_function(DEFMULTI, create_native_function3<defmulti, &DEFMULTI>());
        define_function(DEFMETHOD, create_native_function3<defmethod, &DEFMETHOD>());

        define_function(DISASM, create_native_function1<disasm, &DISASM>());
        define_function(GET_BYTECODE_FN_BODY, create_native_function2<get_bytecode_fn_body, &GET_BYTECODE_FN_BODY>());
        define_function(GET_BYTECODE_FN_CONSTS, create_native_function2<get_bytecode_fn_consts, &GET_BYTECODE_FN_CONSTS>());
        define_function(GET_BYTECODE_FN_VARS, create_native_function2<get_bytecode_fn_vars, &GET_BYTECODE_FN_VARS>());
        define_function(GET_BYTECODE_FN_LOCALS_SIZE, create_native_function2<get_bytecode_fn_locals_size, &GET_BYTECODE_FN_LOCALS_SIZE>());

        define_function(SERIALIZE_FN, create_native_function1<serialize_fn, &SERIALIZE_FN>());
        define_function(DESERIALIZE_FN, create_native_function1<deserialize_fn, &DESERIALIZE_FN>());

        define_function(CHAR, create_native_function1<create_char, &CHAR>());

        define_function(START_PROFILING, create_native_function0<prof::start, &START_PROFILING>());
        define_function(FINISH_PROFILING, create_native_function0<prof::finish, &FINISH_PROFILING>());

        define_function(STR_STARTS_WITH, create_native_function2<str_starts_with, &STR_STARTS_WITH>());

        define_function(SORT_E, create_native_function2<sort_transient, &SORT_E>());

        define_function(DERIVE, create_native_function2<derive_type, &DERIVE>());

        define_function(GET_TYPE_FIELD_INDEX, create_native_function2<get_type_field_index, &GET_TYPE_FIELD_INDEX>());

        define_function(ADD_VAR_FN_DEP, create_native_function2<add_var_fn_dep, &ADD_VAR_FN_DEP>());
    }
} initialize;

}

}
