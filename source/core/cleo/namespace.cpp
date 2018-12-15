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

Force create_namespace(Value name, Value meta)
{
    return create_object4(*type::Namespace, name, meta, *EMPTY_MAP, *EMPTY_MAP);
}

Value get_ns_mapping(Value ns)
{
    return get_object_element(ns, 2);
}

void set_ns_mapping(Value ns, Value mapping)
{
    set_object_element(ns, 2, mapping);
}

Value get_ns_aliases(Value ns)
{
    return get_object_element(ns, 3);
}

void set_ns_aliseses(Value ns, Value aliases)
{
    set_object_element(ns, 3, aliases);
}

Value get_or_create_ns(Value name, Value meta)
{
    auto ns = persistent_hash_map_get(*namespaces, name);
    if (ns)
        return ns;
    Root new_ns{create_namespace(name, meta)};
    namespaces = persistent_hash_map_assoc(*namespaces, name, *new_ns);
    return *new_ns;
}

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

Value ns_name(Value ns)
{
    check_type("ns", ns, *type::Namespace);
    return get_object_element(ns, 0);
}

Value get_ns(Value name)
{
    check_type("ns", name, *type::Symbol);
    auto found = persistent_hash_map_get(*namespaces, name);
    if (!found)
        throw_illegal_argument("Namespace not found: " + to_string(name));
    return found;
}

Value get_ns_meta(Value ns)
{
    return get_object_element(ns, 1);
}

Value in_ns(Value ns, Value meta)
{
    if (get_value_tag(ns) != tag::SYMBOL)
    {
        Root msg{create_string("ns must be a symbol")};
        throw_exception(new_illegal_argument(*msg));
    }
    rt::current_ns = get_or_create_ns(ns, meta);
    return nil;
}

Value refer(Value ns)
{
    if (get_value_tag(ns) != tag::SYMBOL)
    {
        Root msg{create_string("ns must be a symbol")};
        throw_exception(new_illegal_argument(*msg));
    }
    Root mapping{get_ns_mapping(*rt::current_ns)};
    auto other_mapping = ns_map(ns);
    mapping = map_merge(*mapping, other_mapping);
    set_ns_mapping(*rt::current_ns, *mapping);
    return nil;
}

Value define(Value sym, Value val, Value meta)
{
    assert(get_value_tag(sym) == tag::SYMBOL);
    auto ns_name = namespace_symbol(sym);
    auto ns = get_or_create_ns(ns_name, nil);
    auto var = define_var(sym, val, meta);
    auto var_name = name_symbol(sym);
    Root mapping{get_ns_mapping(ns)};
    mapping = persistent_hash_map_assoc(*mapping, var_name, var);
    set_ns_mapping(ns, *mapping);
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
    return resolve_var(ns_name(*rt::current_ns), sym);
}

Value maybe_resolve_var(Value ns, Value sym)
{
    auto sym_ns = namespace_symbol(sym);
    auto sym_name = name_symbol(sym);
    ns = persistent_hash_map_get(*namespaces, ns);
    if (sym_ns)
    {
        if (auto alias = persistent_hash_map_get(get_ns_aliases(ns), sym_ns))
            sym_ns = ns_name(alias);
        ns = persistent_hash_map_get(*namespaces, sym_ns);
    }
    if (!ns)
        return nil;
    auto var = persistent_hash_map_get(get_ns_mapping(ns), sym_name);
    if (!var || (sym_ns && sym_ns != namespace_symbol(get_var_name(var))))
        return nil;
    return var;
}

Value maybe_resolve_var(Value sym)
{
    return maybe_resolve_var(ns_name(*rt::current_ns), sym);
}

Value lookup(Value ns, Value sym)
{
    return get_var_value(resolve_var(ns, sym));
}

Value lookup(Value sym)
{
    return lookup(ns_name(*rt::current_ns), sym);
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

Value alias(Value as, Value ns)
{
    Root aliases{get_ns_aliases(*rt::current_ns)};
    aliases = persistent_hash_map_assoc(*aliases, as, get_ns(ns));
    set_ns_aliseses(*rt::current_ns, *aliases);
    return nil;
}

Value ns_map(Value ns)
{
    return get_ns_mapping(get_ns(ns));
}

Value ns_aliases(Value ns)
{
    return get_ns_aliases(get_ns(ns));
}

}
