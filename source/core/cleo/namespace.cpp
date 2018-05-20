#include "namespace.hpp"
#include "global.hpp"
#include "var.hpp"
#include "error.hpp"
#include "eval.hpp"
#include "util.hpp"
#include "multimethod.hpp"
#include "persistent_hash_map.hpp"
#include "print.hpp"
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
    auto current_ns_name = *rt::current_ns;
    Root current_ns{map_get(*namespaces, current_ns_name)};
    if (!*current_ns)
        current_ns = *EMPTY_MAP;
    current_ns = map_merge(*current_ns, ns_map(ns));
    namespaces = map_assoc(*namespaces, current_ns_name, *current_ns);
    return nil;
}

Value define(Value sym, Value val, Value meta)
{
    assert(get_value_tag(sym) == tag::SYMBOL);
    auto ns_name = namespace_symbol(sym);
    Root ns{persistent_hash_map_get(*namespaces, ns_name)};
    if (!*ns)
        ns = *EMPTY_MAP;
    auto var = define_var(sym, val, meta);
    auto var_name = name_symbol(sym);
    ns = persistent_hash_map_assoc(*ns, var_name, var);
    namespaces = persistent_hash_map_assoc(*namespaces, ns_name, *ns);
    return var;
}

Value resolve_var(Value ns, Value sym)
{
    if (auto var = maybe_resolve_var(ns, sym))
        return var;
    Root ss{pr_str(sym)};
    Root msg{create_string("unable to resolve symbol " + std::string(get_string_ptr(*ss), get_string_len(*ss)))};
    throw_exception(new_symbol_not_found(*msg));
}

Value resolve_var(Value sym)
{
    return resolve_var(*rt::current_ns, sym);
}

Value maybe_resolve_var(Value ns, Value sym)
{
    auto sym_ns = namespace_symbol(sym);
    ns = sym_ns ? sym_ns : name_symbol(ns);
    ns = map_get(*namespaces, ns);
    auto var_name = name_symbol(sym);
    auto var = ns ? map_get(ns, var_name) : nil;
    if (!var || (sym_ns && get_symbol_name(sym_ns) != get_symbol_namespace(get_var_name(var))))
        return nil;
    return var;
}

Value maybe_resolve_var(Value sym)
{
    return maybe_resolve_var(*rt::current_ns, sym);
}

Value lookup(Value ns, Value sym)
{
    return get_var_value(resolve_var(ns, sym));
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

Value ns_map(Value ns)
{
    check_type("ns", ns, *type::Symbol);
    auto found = map_get(*namespaces, ns);
    if (!found)
        throw_illegal_argument("Namespace not found: " + to_string(ns));
    return found;
}

}
