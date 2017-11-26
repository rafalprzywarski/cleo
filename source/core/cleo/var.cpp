#include "var.hpp"
#include "singleton.hpp"
#include <unordered_map>

namespace cleo
{

class Vars {};
singleton<std::unordered_map<Value, Value>, Vars> vars;


void define(Value sym, Value val)
{
    (*vars)[sym] = val;
}

Value lookup(Value sym)
{
    auto it = vars->find(sym);
    return it == vars->end() ? nil : it->second;
}

}
