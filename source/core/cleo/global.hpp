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

extern const Value TRUE;
extern const Value SEQ;
extern const Value FIRST;
extern const Value NEXT;
extern const Value OBJ_EQ;
extern const Value PR_STR_OBJ;
extern const Value QUOTE;
extern const Value FN;

namespace type
{
extern const Value NATIVE_FUNCTION;
extern const Value CONS;
extern const Value LIST;
extern const Value SMALL_VECTOR;
extern const Value SMALL_VECTOR_SEQ;
extern const Value SMALL_MAP;
extern const Value MULTIMETHOD;
extern const Value SEQUABLE;
extern const Value SEQUENCE;
extern const Value FN;
}

extern const std::array<Value, 7> type_by_tag;

}
