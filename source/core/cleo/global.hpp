#pragma once
#include "value.hpp"
#include "hash.hpp"
#include "equality.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <array>

namespace cleo
{

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

extern const std::array<Value, 7> type_by_tag;

extern const Value TRUE;
extern const Value SEQ;
extern const Value FIRST;
extern const Value NEXT;
extern const Value OBJ_EQ;
extern const Value PR_STR_OBJ;

namespace type
{
extern const Value CONS;
extern const Value LIST;
extern const Value SMALL_VECTOR;
extern const Value SMALL_VECTOR_SEQ;
extern const Value MULTIMETHOD;
extern const Value SEQUABLE;
extern const Value SEQUENCE;
}

}
