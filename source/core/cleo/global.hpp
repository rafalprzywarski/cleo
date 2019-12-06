#pragma once
#include "value.hpp"
#include "hash.hpp"
#include "equality.hpp"
#include "var.hpp"
#include "memory.hpp"
#include "vm.hpp"
#include "error.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <array>
#include <vector>
#include <cassert>
#include <ostream>
#include <memory>
#include <thread>
#include <atomic>

namespace cleo
{

extern std::vector<Allocation> allocations;
extern std::vector<Value> extra_roots;
extern unsigned gc_frequency;
extern unsigned gc_counter;
extern std::unique_ptr<std::ostream> gc_log;

extern vm::Stack stack;
extern vm::IntStack int_stack;

inline void stack_push(Force val)
{
    if (stack.size() == stack.capacity())
        throw_exception(new_stack_overflow());
    stack.push_back(val.value());
}

template <typename FwdIt>
inline void stack_push(FwdIt first, FwdIt last)
{
    if (stack.size() + std::distance(first, last) > stack.capacity())
        throw_exception(new_stack_overflow());
    stack.insert(stack.end(), first, last);
}

inline void stack_push(Force val, std::size_t n)
{
    if (stack.size() + n > stack.capacity())
        throw_exception(new_stack_overflow());
    stack.resize(stack.size() + n, val.value());
}

inline void stack_pop()
{
    if (!stack.empty())
        stack.pop_back();
}

inline void stack_pop(std::size_t n)
{
    if (stack.size() <= n)
        stack.clear();
    else
        stack.resize(stack.size() - n);
}

inline auto stack_reserve(std::size_t n)
{
    stack_push(nil, n);
}

inline void int_stack_push(Int64 val)
{
    if (int_stack.size() == int_stack.capacity())
        throw_exception(new_stack_overflow());
    int_stack.push_back(val);
}

inline void int_stack_pop()
{
    if (!int_stack.empty())
        int_stack.pop_back();
}

class StackGuard
{
public:
    StackGuard(std::size_t n = 0) : stack_size(stack.size() - n), int_stack_size(int_stack.size()) {}
    StackGuard(const StackGuard& ) = delete;
    ~StackGuard()
    {
        assert(stack.size() >= stack_size); stack.resize(stack_size);
        assert(int_stack.size() >= int_stack_size); int_stack.resize(int_stack_size);
    }
private:
    const std::size_t stack_size;
    const std::size_t int_stack_size;
};

class Root
{
public:
    Root() : index(extra_roots.size()) { extra_roots.push_back(nil); }
    Root(Force f) : Root() { *this = f.value(); }
    Root(const Root& ) = delete;
    ~Root() { assert(extra_roots.size() == index + 1); extra_roots.pop_back(); }
    Root& operator=(const Root& ) = delete;
    Root& operator=(const Force& f) { return *this = f.value(); }
    Root& operator=(Value val) { extra_roots[index] = val; return *this; }
    Value operator*() const { return extra_roots[index]; }
    Value *operator->() const { return &extra_roots[index]; }
private:
    decltype(extra_roots)::size_type index;
};

class ConstRoot
{
public:
    ConstRoot(Force f) : val(f.value()) { extra_roots.push_back(val); }
    ConstRoot(const ConstRoot& ) = delete;
    ConstRoot& operator=(const ConstRoot& ) = delete;
    ~ConstRoot() { extra_roots.pop_back(); }
    Value operator*() const { return val; }
    const Value *operator->() const { return &val; }
private:
    const Value val;
};

class Roots
{
public:
    using size_type = decltype(extra_roots)::size_type;

    explicit Roots(size_type count)
        : index(extra_roots.size())
#ifndef NDEBUG
        , count(count)
#endif
    { extra_roots.resize(index + count, nil); }
    Roots(const Roots& ) = delete;
    ~Roots() { assert(extra_roots.size() == index + count); extra_roots.resize(index); }
    Roots& operator=(const Roots& ) = delete;
    Value operator[](size_type k) const { return extra_roots[index + k]; }
    void set(size_type k, Force f) { extra_roots[index + k] = f.value(); }
private:
    size_type index;
#ifndef NDEBUG
    size_type count;
#endif
};

class StaticVar
{
public:
    StaticVar(Value var) : var(var) { }
    Value operator*() const { return get_var_root_value(var); }
private:
    Value var;
};

class DynamicVar
{
public:
    DynamicVar(Value var) : var(var) { }
    Value operator*() const { return get_var_value(var); }
    const DynamicVar& operator=(Value val) const { set_var(get_var_name(var), val); return *this; }
private:
    Value var;
};

extern std::unordered_map<std::string, std::unordered_map<std::string, Value>> symbols;
extern std::unordered_map<std::string, std::unordered_map<std::string, Value>> keywords;

extern std::unordered_map<Value, Value, std::hash<Value>, StdIs> vars;

struct Hierachy
{
    std::unordered_map<Value, std::unordered_set<Value, StdHash>, StdHash> ancestors;
};

struct Multimethod
{
    Value dispatchFn;
    Value defaultDispatchVal;
    std::unordered_map<Value, Value, StdHash> fns;
    mutable std::unordered_map<Value, Value, StdHash> memoized_fns;
};

extern std::unordered_map<Value, Multimethod, std::hash<Value>, StdIs> multimethods;
extern Hierachy global_hierarchy;

extern Root current_exception;

namespace prof
{
extern bool enabled;
constexpr std::size_t MAX_CALLSTACK_SIZE = 2048;
extern Value callstack[MAX_CALLSTACK_SIZE];
extern volatile std::size_t callstack_size;
extern Value callstack_copy[MAX_CALLSTACK_SIZE];
extern std::size_t callstack_size_copy;
extern std::atomic_bool callstack_copy_ready;
extern std::atomic_bool callstack_copy_needed;
extern std::atomic_bool finished;
extern std::thread::id main_thread_id;
extern std::vector<std::vector<Value>> callstacks;
extern std::thread collector;
}

extern const Value TRUE;
extern const Value SEQ;
extern const Value FIRST;
extern const Value NEXT;
extern const Value COUNT;
extern const Value GET;
extern const Value CONTAINS;
extern const Value CONJ;
extern const Value ASSOC;
extern const Value DISSOC;
extern const Value MERGE;
extern const Value ARRAY_MAP;
extern const Value OBJ_EQ;
extern const Value OBJ_CALL;
extern const Value PRINT_READABLY;
extern const Value PR_STR;
extern const Value PR_STR_OBJ;
extern const Value STR;
extern const Value GET_MESSAGE;
extern const Value QUOTE;
extern const Value UNQUOTE;
extern const Value UNQUOTE_SPLICING;
extern const Value FN;
extern const Value DEF;
extern const Value LET;
extern const Value DO;
extern const Value IF;
extern const Value LOOP;
extern const Value RECUR;
extern const Value INTERNAL_ADD_2;
extern const Value MINUS;
extern const Value ASTERISK;
extern const Value IDENTICAL;
extern const Value ISA;
extern const Value SYMBOL_Q;
extern const Value VECTOR_Q;
extern const Value LT;
extern const Value EQ;
extern const Value THROW;
extern const Value TRY;
extern const Value CATCH;
extern const Value FINALLY;
extern const Value VA;
extern const Value CURRENT_NS;
extern const Value IN_NS;
extern const Value NS;
extern const Value LIB_PATHS;
extern const Value ATOM;
extern const Value DEREF;
extern const Value RESET;
extern const Value SWAP;
extern const Value APPLY;
extern const Value FORM;
extern const Value ENV;
extern const Value CLEO_CORE;
extern const Value NEW;
extern const Value HASH_OBJ;
extern const Value IMPORT_C_FN;
extern const Value COMMAND_LINE_ARGS;
extern const Value LIST;
extern const Value CONS;
extern const Value LAZY_SEQ;
extern const Value VECTOR;
extern const Value HASH_MAP;
extern const Value HASH_SET;
extern const Value CONCATI;
extern const Value MACRO_KEY;
extern const Value DYNAMIC_KEY;
extern const Value CONST_KEY;
extern const Value PRIVATE_KEY;
extern const Value NAME_KEY;
extern const Value NS_KEY;
extern const Value DOT;
extern const Value COMPILE;
extern const Value SHOULD_RECOMPILE;
extern const Root ZERO;
extern const Root ONE;
extern const Root NEG_ONE;
extern const Root TWO;
extern const Root THREE;
extern const Root SENTINEL;
extern const Root RELOAD;

extern const std::unordered_set<Value, std::hash<Value>, StdIs> SPECIAL_SYMBOLS;

namespace type
{
extern const ConstRoot Type;
extern const Value Int64;
extern const ConstRoot UChar;
extern const ConstRoot Float64;
extern const ConstRoot UTF8String;
extern const ConstRoot NativeFunction;
extern const ConstRoot CFunction;
extern const ConstRoot Symbol;
extern const ConstRoot Keyword;
extern const ConstRoot Var;
extern const ConstRoot List;
extern const ConstRoot Cons;
extern const ConstRoot LazySeq;
extern const ConstRoot Array;
extern const ConstRoot TransientArray;
extern const ConstRoot ArraySeq;
extern const ConstRoot ArrayMap;
extern const ConstRoot ArrayMapSeq;
extern const ConstRoot ArraySet;
extern const ConstRoot ArraySetSeq;
extern const ConstRoot Multimethod;
extern const ConstRoot Seqable;
extern const ConstRoot Sequence;
extern const ConstRoot Callable;
extern const ConstRoot BytecodeFn;
extern const ConstRoot BytecodeFnBody;
extern const ConstRoot BytecodeFnExceptionTable;
extern const ConstRoot Atom;
extern const ConstRoot PersistentMap;
extern const ConstRoot PersistentHashMap;
extern const ConstRoot PersistentHashMapSeq;
extern const ConstRoot PersistentHashMapSeqParent;
extern const ConstRoot PersistentHashMapCollisionNode;
extern const ConstRoot PersistentHashMapArrayNode;
extern const ConstRoot Exception;
extern const ConstRoot CastError;
extern const ConstRoot ReadError;
extern const ConstRoot CallError;
extern const ConstRoot SymbolNotFound;
extern const ConstRoot IllegalArgument;
extern const ConstRoot IllegalState;
extern const ConstRoot UnexpectedEndOfInput;
extern const ConstRoot FileNotFound;
extern const ConstRoot ArithmeticException;
extern const ConstRoot IndexOutOfBounds;
extern const ConstRoot CompilationError;
extern const ConstRoot StackOverflow;
extern const ConstRoot Namespace;
extern const ConstRoot UTF8StringSeq;
}

namespace clib
{
extern const Value int64;
extern const Value string;
}

extern const ConstRoot EMPTY_LIST;
extern const ConstRoot EMPTY_VECTOR;
extern const ConstRoot EMPTY_SET;
extern const ConstRoot EMPTY_MAP;
extern const ConstRoot EMPTY_HASH_MAP;

extern Root namespaces;
extern Root bindings;

extern Int64 next_id;

Int64 gen_id();

namespace rt
{

extern const Root transient_array;
extern const Root transient_array_conj;
extern const Root transient_array_persistent;
extern const Root array_set_conj;
extern const Root map_assoc;

extern const DynamicVar current_ns;
extern const DynamicVar lib_paths;
extern const StaticVar obj_eq;
extern const StaticVar obj_call;
extern const DynamicVar print_readably;
extern const StaticVar pr_str_obj;
extern const StaticVar first;
extern const StaticVar next;
extern const StaticVar seq;
extern const StaticVar count;
extern const StaticVar get;
extern const StaticVar contains;
extern const StaticVar assoc;
extern const StaticVar merge;
extern const StaticVar get_message;
extern const StaticVar hash_obj;
extern const StaticVar compile;

}

inline Value get_value_type(Value val)
{
    switch (get_value_tag(val))
    {
    case tag::OBJECT: return get_object_type(val);
    case tag::NATIVE_FUNCTION: return *type::NativeFunction;
    case tag::SYMBOL: return *type::Symbol;
    case tag::KEYWORD: return *type::Keyword;
    case tag::INT64: return type::Int64;
    case tag::UCHAR: return *type::UChar;
    case tag::FLOAT64: return *type::Float64;
    case tag::UTF8STRING: return *type::UTF8String;
    case tag::OBJECT_TYPE: return *type::Type;
    default: return nil;
    }
}

}
