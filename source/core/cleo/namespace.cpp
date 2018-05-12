#include "namespace.hpp"
#include "global.hpp"
#include "var.hpp"
#include "error.hpp"
#include "eval.hpp"
#include "util.hpp"
#include "multimethod.hpp"
#include "persistent_hash_map.hpp"
#include <fstream>

namespace cleo
{

namespace
{

std::string locate_source(const std::string& ns_name)
{
    for (Root s{call_multimethod1(*rt::seq, *rt::lib_paths)}; *s; s = call_multimethod1(*rt::next, *s))
    {
        Root dir{call_multimethod1(*rt::first, *s)};
        check_type("library path", *dir, *type::String);
        auto path = std::string(get_string_ptr(*dir), get_string_len(*dir)) + "/" + ns_name + ".cleo";
        if (std::ifstream(path))
            return path;
    }

    return ns_name + ".cleo";
}

}

Value in_ns(Value ns)
{
    if (get_value_tag(ns) != tag::SYMBOL)
    {
        Root msg{create_string("ns must be a symbol")};
        throw_exception(new_illegal_argument(*msg));
    }
    rt::current_ns = ns;
    return nil;
}

Value refer(Value ns)
{
    if (get_value_tag(ns) != tag::SYMBOL)
    {
        Root msg{create_string("ns must be a symbol")};
        throw_exception(new_illegal_argument(*msg));
    }
    auto current_ns_name = get_symbol_name(*rt::current_ns);
    Root current_ns{map_get(*namespaces, current_ns_name)};
    if (!*current_ns)
        current_ns = *EMPTY_MAP;
    ns =map_get(*namespaces, get_symbol_name(ns));
    if (!ns)
        return nil;
    current_ns = map_merge(*current_ns, ns);
    namespaces = map_assoc(*namespaces, current_ns_name, *current_ns);
    return nil;
}

Value define(Value sym, Value val, Value meta)
{
    assert(get_value_tag(sym) == tag::SYMBOL);
    Root ns{persistent_hash_map_get(*namespaces, get_symbol_namespace(sym))};
    if (!*ns)
        ns = *EMPTY_MAP;
    ns = persistent_hash_map_assoc(*ns, get_symbol_name(sym), sym);
    namespaces = persistent_hash_map_assoc(*namespaces, get_symbol_namespace(sym), *ns);
    return define_var(sym, val, meta);
}

Value resolve(Value ns, Value sym)
{
    auto sym_ns = get_symbol_namespace(sym);
    if (!sym_ns)
    {
        ns = map_get(*namespaces, get_symbol_name(ns));
        if (ns)
        {
            if (auto found = map_get(ns, get_symbol_name(sym)))
                sym = found;
        }
    }
    return sym;
}

Value resolve(Value sym)
{
    return resolve(*rt::current_ns, sym);
}

Value lookup(Value ns, Value sym)
{
    sym = resolve(ns, sym);
    return get_var_value(lookup_var(sym));
}

Value lookup(Value sym)
{
    return lookup(*rt::current_ns, sym);
}

Value require(Value ns)
{
    if (get_value_tag(ns) != tag::SYMBOL)
    {
        Root msg{create_string("ns must be a symbol")};
        throw_exception(new_illegal_argument(*msg));
    }
    auto ns_name = get_symbol_name(ns);
    std::string path = locate_source({get_string_ptr(ns_name), get_string_len(ns_name)});
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
