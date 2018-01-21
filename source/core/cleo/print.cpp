#include "print.hpp"
#include "global.hpp"
#include "var.hpp"
#include "multimethod.hpp"
#include "small_vector.hpp"
#include "small_set.hpp"
#include "small_map.hpp"
#include "error.hpp"
#include <sstream>

namespace cleo
{
namespace
{

Force pr_str_native_function(Value fn)
{
    std::ostringstream os;
    os << "#cleo.core/NativeFunction[0x" << std::hex << fn << "]";
    return create_string(os.str());
}

Force pr_str_symbol(Value sym)
{
    auto ns = get_keyword_namespace(sym);
    auto name = get_keyword_name(sym);
    std::string s;
    s.reserve(1 + (ns == nil ? 0 : get_string_len(ns)) + get_string_len(name));
    if (ns != nil)
    {
        s.append(get_string_ptr(ns), get_string_len(ns));
        s += '/';
    }
    s.append(get_string_ptr(name), get_string_len(name));
    return create_string(s);
}

Force pr_str_keyword(Value kw)
{
    auto ns = get_keyword_namespace(kw);
    auto name = get_keyword_name(kw);
    std::string s;
    s.reserve(1 + (ns == nil ? 0 : get_string_len(ns)) + get_string_len(name));
    s += ':';
    if (ns != nil)
    {
        s.append(get_string_ptr(ns), get_string_len(ns));
        s += '/';
    }
    s.append(get_string_ptr(name), get_string_len(name));
    return create_string(s);
}

Force pr_str_float(Value val)
{
    std::ostringstream os;
    os << get_float64_value(val);
    auto s = os.str();
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos)
        s += ".0";
    return create_string(s);
}

Force pr_str_string(Value val)
{
    std::string s;
    s.reserve(2 * get_string_len(val) + 2);
    s += '\"';
    auto p = get_string_ptr(val);
    auto e = p + get_string_len(val);
    for (; p != e; ++p)
        switch (*p)
        {
            case '\t': s += "\\t"; break;
            case '\r': s += "\\r"; break;
            case '\n': s += "\\n"; break;
            case '\\':
            case '\"':
            case '\'':
                s += '\\';
                s += *p;
                break;
            default:
                s += *p;
        }
    s += '\"';
    return create_string(s);
}

}

Force pr_str_object(Value val)
{
    if (get_value_tag(val) != tag::OBJECT)
    {
        Root msg{create_string("expected an object")};
        throw_exception(new_illegal_argument(*msg));
    }
    Root type{pr_str(get_object_type(val))};
    std::ostringstream os;
    os << '#' << std::string(get_string_ptr(*type), get_string_len(*type)) << "[0x" << std::hex << val << "]";
    return create_string(os.str());
}

Force pr_str_small_vector(Value v)
{
    std::string str;
    str += '[';
    auto size = get_small_vector_size(v);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (i > 0)
            str += ' ';
        Root s{pr_str(get_small_vector_elem(v, i))};
        str.append(get_string_ptr(*s), get_string_len(*s));
    }
    str += ']';
    return create_string(str);
}

Force pr_str_small_set(Value s)
{
    std::string str;
    str += "#{";
    auto size = get_small_set_size(s);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (i > 0)
            str += ' ';
        Root ss{pr_str(get_small_set_elem(s, i))};
        str.append(get_string_ptr(*ss), get_string_len(*ss));
    }
    str += '}';
    return create_string(str);
}

Force pr_str_small_map(Value m)
{
    std::string str;
    str += '{';
    auto size = get_small_map_size(m);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (i > 0)
            str += ", ";
        Root s{pr_str(get_small_map_key(m, i))};
        str.append(get_string_ptr(*s), get_string_len(*s));
        str += ' ';
        s = pr_str(get_small_map_val(m, i));
        str.append(get_string_ptr(*s), get_string_len(*s));
    }
    str += '}';
    return create_string(str);
}

Force pr_str_seqable(Value v)
{
    auto seq = lookup_var(SEQ);
    auto first = lookup_var(FIRST);
    auto next = lookup_var(NEXT);
    std::string str;
    str += '(';
    bool first_elem = true;
    for (Root s{call_multimethod1(seq, v)}; *s != nil; s = call_multimethod1(next, *s))
    {
        if (first_elem)
            first_elem = false;
        else
            str += ' ';

        Root f{call_multimethod1(first, *s)};
        Root ss{pr_str(*f)};
        str.append(get_string_ptr(*ss), get_string_len(*ss));
    }
    str += ')';
    return create_string(str);
}

Force print_str_small_vector(Value v)
{
    std::string str;
    str += '[';
    auto size = get_small_vector_size(v);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (i > 0)
            str += ' ';
        Root s{print_str(get_small_vector_elem(v, i))};
        str.append(get_string_ptr(*s), get_string_len(*s));
    }
    str += ']';
    return create_string(str);
}

Force print_str_small_set(Value s)
{
    std::string str;
    str += "#{";
    auto size = get_small_set_size(s);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (i > 0)
            str += ' ';
        Root ss{print_str(get_small_set_elem(s, i))};
        str.append(get_string_ptr(*ss), get_string_len(*ss));
    }
    str += '}';
    return create_string(str);
}

Force print_str_small_map(Value m)
{
    std::string str;
    str += '{';
    auto size = get_small_map_size(m);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (i > 0)
            str += ", ";
        Root s{print_str(get_small_map_key(m, i))};
        str.append(get_string_ptr(*s), get_string_len(*s));
        str += ' ';
        s = print_str(get_small_map_val(m, i));
        str.append(get_string_ptr(*s), get_string_len(*s));
    }
    str += '}';
    return create_string(str);
}

Force print_str_seqable(Value v)
{
    auto seq = lookup_var(SEQ);
    auto first = lookup_var(FIRST);
    auto next = lookup_var(NEXT);
    std::string str;
    str += '(';
    bool first_elem = true;
    for (Root s{call_multimethod1(seq, v)}; *s != nil; s = call_multimethod1(next, *s))
    {
        if (first_elem)
            first_elem = false;
        else
            str += ' ';

        Root f{call_multimethod1(first, *s)};
        Root ss{print_str(*f)};
        str.append(get_string_ptr(*ss), get_string_len(*ss));
    }
    str += ')';
    return create_string(str);
}

Force pr_str(Value val)
{
    switch (get_value_tag(val))
    {
        case tag::NIL: return create_string("nil");
        case tag::NATIVE_FUNCTION: return pr_str_native_function(val);
        case tag::SYMBOL: return pr_str_symbol(val);
        case tag::KEYWORD: return pr_str_keyword(val);
        case tag::INT64: return create_string(std::to_string(get_int64_value(val)));
        case tag::FLOAT64: return pr_str_float(val);
        case tag::STRING: return pr_str_string(val);
        default: // tag::OBJECT
            return call_multimethod(lookup_var(PR_STR_OBJ), &val, 1);
    }
}

Force print_str(Value val)
{
    switch (get_value_tag(val))
    {
        case tag::NIL: return create_string("nil");
        case tag::NATIVE_FUNCTION: return pr_str_native_function(val);
        case tag::SYMBOL: return pr_str_symbol(val);
        case tag::KEYWORD: return pr_str_keyword(val);
        case tag::INT64: return create_string(std::to_string(get_int64_value(val)));
        case tag::FLOAT64: return pr_str_float(val);
        case tag::STRING: return val;
        default: // tag::OBJECT
            return call_multimethod(lookup_var(PRINT_STR_OBJ), &val, 1);
    }
}

}
