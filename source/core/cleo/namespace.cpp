#include "namespace.hpp"
#include "global.hpp"
#include "var.hpp"
#include "error.hpp"
#include "small_map.hpp"
#include "eval.hpp"
#include <fstream>

namespace cleo
{

Value in_ns(Value ns)
{
    if (ns != nil && get_value_tag(ns) != tag::SYMBOL)
    {
        Root msg{create_string("ns must be a symbol")};
        throw_exception(new_illegal_argument(*msg));
    }
    set_var(CURRENT_NS, ns);
    return nil;
}

Value refer(Value ns)
{
    auto current_ns_name = get_symbol_name(lookup_var(CURRENT_NS));
    Root current_ns{small_map_get(*namespaces, current_ns_name)};
    if (*current_ns == nil)
        current_ns = *EMPTY_MAP;
    ns = small_map_get(*namespaces, get_symbol_name(ns));
    if (ns == nil)
        return nil;
    current_ns = small_map_merge(*current_ns, ns);
    namespaces = small_map_assoc(*namespaces, current_ns_name, *current_ns);
    return nil;
}

Value define(Value sym, Value val)
{
    assert(get_value_tag(sym) == tag::SYMBOL);
    Root ns{small_map_get(*namespaces, get_symbol_namespace(sym))};
    if (*ns == nil)
        ns = *EMPTY_MAP;
    ns = small_map_assoc(*ns, get_symbol_name(sym), sym);
    namespaces = small_map_assoc(*namespaces, get_symbol_namespace(sym), *ns);
    define_var(sym, val);
    return nil;
}

Value resolve(Value ns, Value sym)
{
    auto sym_ns = get_symbol_namespace(sym);
    if (sym_ns == nil)
    {
        ns = small_map_get(*namespaces, get_symbol_name(ns));
        if (ns != nil)
        {
            auto found = small_map_get(ns, get_symbol_name(sym));
            if (found != nil)
                sym = found;
        }
    }
    return sym;
}

Value resolve(Value sym)
{
    return resolve(lookup_var(CURRENT_NS), sym);
}

Value lookup(Value ns, Value sym)
{
    sym = resolve(ns, sym);
    return lookup_var(sym);
}

Value lookup(Value sym)
{
    return lookup(lookup_var(CURRENT_NS), sym);
}

Value require(Value ns)
{
    if (get_value_tag(ns) != tag::SYMBOL)
    {
        Root msg{create_string("ns must be a symbol")};
        throw_exception(new_illegal_argument(*msg));
    }
    auto ns_name = get_symbol_name(ns);
    auto lib_path = lookup(LIB_PATH);
    std::string path =
        std::string(get_string_ptr(lib_path), get_string_len(lib_path)) + "/" +
        std::string(get_string_ptr(ns_name), get_string_len(ns_name)) + ".cleo";
    std::ifstream f(path);
    if (!f)
    {
        Root msg{create_string("Could not locate " + path)};
        throw_exception(new_file_not_found(*msg));
    }
    std::string source{std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
    Root ssource{create_string(source)};
    load(*ssource);
    return nil;
}

}
