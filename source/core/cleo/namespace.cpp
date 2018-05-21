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

Force create_namespace(Value name, Value mapping)
{
    return create_object2(*type::Namespace, name, mapping);
}

Value get_ns_mapping(Value ns)
{
    return get_object_element(ns, 1);
}

void set_ns_mapping(Value ns, Value mapping)
{
    set_object_element(ns, 1, mapping);
}

Value get_ns(Value name)
{
    check_type("ns", name, *type::Symbol);
    auto found = persistent_hash_map_get(*namespaces, name);
    if (!found)
        throw_illegal_argument("Namespace not found: " + to_string(name));
    return found;
}

Value get_or_create_ns(Value name)
{
    auto ns = persistent_hash_map_get(*namespaces, name);
    if (ns)
        return ns;
    Root new_ns{create_namespace(name, nil)};
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

Value in_ns(Value ns)
{
    if (get_value_tag(ns) != tag::SYMBOL)
    {
        Root msg{create_string("ns must be a symbol")};
        throw_exception(new_illegal_argument(*msg));
    }
    get_or_create_ns(ns);
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
    auto current_ns{get_ns(current_ns_name)};
    Root mapping{get_ns_mapping(current_ns)};
    auto other_mapping = ns_map(ns);
    mapping = map_merge(*mapping ? *mapping : *EMPTY_MAP, other_mapping ? other_mapping : *EMPTY_MAP);
    set_ns_mapping(current_ns, *mapping);
    return nil;
}

Value define(Value sym, Value val, Value meta)
{
    assert(get_value_tag(sym) == tag::SYMBOL);
    auto ns_name = namespace_symbol(sym);
    auto ns = get_or_create_ns(ns_name);
    auto var = define_var(sym, val, meta);
    auto var_name = name_symbol(sym);
    Root mapping{get_ns_mapping(ns)};
    mapping = persistent_hash_map_assoc(*mapping ? *mapping : *EMPTY_MAP, var_name, var);
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
    return resolve_var(*rt::current_ns, sym);
}

Value maybe_resolve_var(Value ns, Value sym)
{
    auto sym_ns = namespace_symbol(sym);
    ns = sym_ns ? sym_ns : name_symbol(ns);
    ns = persistent_hash_map_get(*namespaces, ns);
    if (!ns)
        return nil;
    auto mapping = get_ns_mapping(ns);
    auto var_name = name_symbol(sym);
    auto var = mapping ? persistent_hash_map_get(mapping, var_name) : nil;
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
    return get_ns_mapping(get_ns(ns));
}

}
