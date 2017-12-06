#include "global.hpp"

namespace cleo
{

std::unordered_map<std::string, std::unordered_map<std::string, Value>> symbols;
std::unordered_map<std::string, std::unordered_map<std::string, Value>> keywords;

std::unordered_map<Value, Value> vars;

std::unordered_map<Value, Multimethod> multimethods;
Hierachy global_hierarchy;

const std::array<Value, 7> type_by_tag{{
    nil,
    create_symbol("cleo.core", "NativeFunction"),
    create_symbol("cleo.core", "Symbol"),
    create_symbol("cleo.core", "Keyword"),
    create_symbol("cleo.core", "Int64"),
    create_symbol("cleo.core", "Float64"),
    create_symbol("cleo.core", "String")
}};

const Value TRUE = create_keyword("true");

namespace type
{
const Value CONS = create_symbol("cleo.core", "Cons");
const Value LIST = create_symbol("cleo.core", "List");
const Value SMALL_VECTOR = create_symbol("cleo.core", "SmallVector");
const Value SMALL_VECTOR_SEQ = create_symbol("cleo.core", "SmallVectorSeq");
const Value MULTIMETHOD = create_symbol("cleo.core", "Multimethod");
}

}
