#pragma once
#include "value.hpp"
#include "hash.hpp"
#include "equality.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <array>
#include <vector>
#include <cassert>

namespace cleo
{

extern std::vector<void *> allocations;
extern std::vector<Value> extra_roots;
extern unsigned gc_frequency;
extern unsigned gc_counter;

class Root
{
public:
    Root() : index(extra_roots.size()) { extra_roots.push_back(nil); }
    Root(Force f) : Root() { *this = f.val; }
    Root(const Root& ) = delete;
    ~Root() { assert(extra_roots.size() == index + 1); extra_roots.pop_back(); }
    Root& operator=(const Root& ) = delete;
    Root& operator=(const Force& f) { return *this = f.val; }
    Root& operator=(Value val) { extra_roots[index] = val; return *this; }
    Value operator*() const { return extra_roots[index]; }
private:
    decltype(extra_roots)::size_type index;
};

class Roots
{
public:
    using size_type = decltype(extra_roots)::size_type;

    explicit Roots(size_type count) : index(extra_roots.size()), count(count) { extra_roots.resize(index + count, nil); }
    Roots(const Roots& ) = delete;
    ~Roots() { assert(extra_roots.size() == index + count); extra_roots.resize(index); }
    Roots& operator=(const Roots& ) = delete;
    Value operator[](size_type k) const { return extra_roots[index + k]; }
    void set(size_type k, Force f) { extra_roots[index + k] = f.val; }
private:
    size_type index, count;
};

extern std::unordered_map<std::string, std::unordered_map<std::string, Value>> symbols;
extern std::unordered_map<std::string, std::unordered_map<std::string, Value>> keywords;

extern std::unordered_map<Value, Value> vars;

struct Hierachy
{
    std::unordered_map<Value, std::unordered_set<Value, StdHash, StdEqualTo>, StdHash, StdEqualTo> ancestors;
};

struct Multimethod
{
    Value dispatchFn;
    Value defaultDispatchVal;
    std::unordered_map<Value, Value, StdHash, StdEqualTo> fns;
};

extern std::unordered_map<Value, Multimethod> multimethods;
extern Hierachy global_hierarchy;

extern Root current_exception;

extern const Value TRUE;
extern const Value SEQ;
extern const Value FIRST;
extern const Value NEXT;
extern const Value CONJ;
extern const Value ASSOC;
extern const Value OBJ_EQ;
extern const Value OBJ_CALL;
extern const Value PR_STR_OBJ;
extern const Value GET_MESSAGE;
extern const Value QUOTE;
extern const Value SYNTAX_QUOTE;
extern const Value UNQUOTE;
extern const Value UNQUOTE_SPLICING;
extern const Value FN;
extern const Value MACRO;
extern const Value DEF;
extern const Value LET;
extern const Value DO;
extern const Value IF;
extern const Value LOOP;
extern const Value RECUR;
extern const Value PLUS;
extern const Value MINUS;
extern const Value ASTERISK;
extern const Value LT;
extern const Value EQ;
extern const Value THROW;
extern const Value TRY;
extern const Value CATCH;
extern const Value FINALLY;
extern const Value VA;
extern const Value CURRENT_NS;
extern const Value ENV_NS;
extern const Value IN_NS;
extern const Value NS;
extern const Value LIB_PATH;
extern const Value ATOM;
extern const Value DEREF;
extern const Value RESET;
extern const Value SWAP;
extern const Value APPLY;
extern const Value FORM;
extern const Value ENV;

extern const std::unordered_set<Value> SPECIAL_SYMBOLS;

namespace type
{
extern const Value NativeFunction;
extern const Value Symbol;
extern const Value Keyword;
extern const Value List;
extern const Value SmallVector;
extern const Value SmallVectorSeq;
extern const Value SmallMap;
extern const Value SmallSet;
extern const Value SmallSetSeq;
extern const Value Multimethod;
extern const Value Seqable;
extern const Value Sequence;
extern const Value Callable;
extern const Value Fn;
extern const Value Macro;
extern const Value Recur;
extern const Value Atom;
extern const Value Exception;
extern const Value ReadError;
extern const Value CallError;
extern const Value SymbolNotFound;
extern const Value IllegalArgument;
extern const Value IllegalState;
extern const Value UnexpectedEndOfInput;
extern const Value FileNotFound;
}

extern const std::array<Value, 7> type_by_tag;

extern const Root EMPTY_LIST;
extern const Root EMPTY_VECTOR;
extern const Root EMPTY_SET;
extern const Root EMPTY_MAP;
extern const Root recur;

extern Root namespaces;
extern Root bindings;

extern Int64 next_id;

Int64 gen_id();

}
