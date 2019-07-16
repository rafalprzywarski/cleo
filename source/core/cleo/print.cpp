#include "print.hpp"
#include "global.hpp"
#include "var.hpp"
#include "multimethod.hpp"
#include "array.hpp"
#include "array_set.hpp"
#include "array_map.hpp"
#include "persistent_hash_map.hpp"
#include "error.hpp"
#include "util.hpp"
#include <sstream>

namespace cleo
{
namespace
{

Force pr_str_native_function(Value fn)
{
    std::ostringstream os;
    os << "#cleo.core/NativeFunction[" << to_string(get_native_function_name(fn)) << " 0x" << std::hex << fn.bits() << "]";
    return create_string(os.str());
}

Force pr_str_symbol(Value sym)
{
    auto ns = get_keyword_namespace(sym);
    auto name = get_keyword_name(sym);
    std::string s;
    s.reserve(1 + (!ns ? 0 : get_string_len(ns)) + get_string_len(name));
    if (ns)
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
    s.reserve(1 + (!ns ? 0 : get_string_len(ns)) + get_string_len(name));
    s += ':';
    if (ns)
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

char hex_digit(int x)
{
    return "0123456789abcdef"[x & 0xf];
}

Force pr_str_string(Value val)
{
    if (!*rt::print_readably)
        return val;
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
            case '\0': s += "\\0"; break;
            default:
                if (*p < 0x20 || std::uint8_t(*p) >= 0x80)
                {
                    s += "\\x";
                    s += hex_digit(*p >> 4);
                    s += hex_digit(*p);
                }
                else
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
    os << '#' << std::string(get_string_ptr(*type), get_string_len(*type)) << "[0x" << std::hex << val.bits() << "]";
    return create_string(os.str());
}

Force pr_str_array(Value v)
{
    std::string str;
    str += '[';
    auto size = get_array_size(v);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (i > 0)
            str += ' ';
        Root s{pr_str(get_array_elem(v, i))};
        str.append(get_string_ptr(*s), get_string_len(*s));
    }
    str += ']';
    return create_string(str);
}

Force pr_str_array_set(Value s)
{
    std::string str;
    str += "#{";
    auto size = get_array_set_size(s);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (i > 0)
            str += ' ';
        Root ss{pr_str(get_array_set_elem(s, i))};
        str.append(get_string_ptr(*ss), get_string_len(*ss));
    }
    str += '}';
    return create_string(str);
}

Force pr_str_array_map(Value m)
{
    std::string str;
    str += '{';
    auto size = get_array_map_size(m);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (i > 0)
            str += ", ";
        Root s{pr_str(get_array_map_key(m, i))};
        str.append(get_string_ptr(*s), get_string_len(*s));
        str += ' ';
        s = pr_str(get_array_map_val(m, i));
        str.append(get_string_ptr(*s), get_string_len(*s));
    }
    str += '}';
    return create_string(str);
}

Force pr_str_persistent_hash_map(Value val)
{
    std::string str;
    str += '{';
    for (Root seq{persistent_hash_map_seq(val)}; *seq; seq = get_persistent_hash_map_seq_next(*seq))
    {
        auto kv = get_persistent_hash_map_seq_first(*seq);
        if (str.back() != '{')
            str += ", ";
        Root s{pr_str(get_array_elem(kv, 0))};
        str.append(get_string_ptr(*s), get_string_len(*s));
        str += ' ';
        s = pr_str(get_array_elem(kv, 1));
        str.append(get_string_ptr(*s), get_string_len(*s));
    }
    str += '}';
    return create_string(str);
}

Force pr_str_seqable(Value v)
{
    std::string str;
    str += '(';
    bool first_elem = true;
    for (Root s{call_multimethod1(*rt::seq, v)}; *s; s = call_multimethod1(*rt::next, *s))
    {
        if (first_elem)
            first_elem = false;
        else
            str += ' ';

        Root f{call_multimethod1(*rt::first, *s)};
        Root ss{pr_str(*f)};
        str.append(get_string_ptr(*ss), get_string_len(*ss));
    }
    str += ')';
    return create_string(str);
}

Force pr_str_object_type(Value v)
{
    return pr_str(get_object_type_name(v));
}

Force pr_str(Value val)
{
    switch (get_value_tag(val))
    {
        case tag::NATIVE_FUNCTION: return pr_str_native_function(val);
        case tag::SYMBOL: return pr_str_symbol(val);
        case tag::KEYWORD: return pr_str_keyword(val);
        case tag::INT64: return create_string(std::to_string(get_int64_value(val)));
        case tag::FLOAT64: return pr_str_float(val);
        case tag::STRING: return pr_str_string(val);
        case tag::OBJECT_TYPE: return pr_str_object_type(val);
        default: // tag::OBJECT
            if (val.is_nil())
                return create_string("nil");
            return call_multimethod(*rt::pr_str_obj, &val, 1);
    }
}

Force print_str(Value val)
{
    Root bindings{*EMPTY_MAP};
    bindings = map_assoc(*bindings, PRINT_READABLY, nil);
    PushBindingsGuard guard{*bindings};
    return pr_str(val);
}

}
