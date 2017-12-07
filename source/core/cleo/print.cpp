#include "print.hpp"
#include "global.hpp"
#include "var.hpp"
#include "multimethod.hpp"
#include "small_vector.hpp"
#include <sstream>

namespace cleo
{
namespace
{

Value pr_str_native_function(Value fn)
{
    std::ostringstream os;
    os << "#cleo.core/NativeFunction[0x" << std::hex << fn << "]";
    return create_string(os.str());
}

Value pr_str_symbol(Value sym)
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

Value pr_str_keyword(Value kw)
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

Value pr_str_float(Value val)
{
    std::ostringstream os;
    os << get_float64_value(val);
    auto s = os.str();
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos)
        s += ".0";
    return create_string(s);
}

Value pr_str_string(Value val)
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

Value pr_str_object(Value val)
{
    auto type = pr_str(get_object_type(val));
    std::ostringstream os;
    os << '#' << std::string(get_string_ptr(type), get_string_len(type)) << "[0x" << std::hex << val << "]";
    return create_string(os.str());
}

Value pr_str_small_vector(Value v)
{
    std::string str;
    str += '[';
    auto size = get_small_vector_size(v);
    for (decltype(size) i = 0; i != size; ++i)
    {
        if (i > 0)
            str += ' ';
        auto s = pr_str(get_small_vector_elem(v, i));
        str.append(get_string_ptr(s), get_string_len(s));
    }
    str += ']';
    return create_string(str);
}

Value pr_str_sequable(Value v)
{
    auto seq = lookup(SEQ);
    auto first = lookup(FIRST);
    auto next = lookup(NEXT);
    std::string str;
    str += '(';
    bool first_elem = true;
    for (Root s{force(call_multimethod(seq, &v, 1))}; *s != nil; *s = call_multimethod(next, &*s, 1))
    {
        if (first_elem)
            first_elem = false;
        else
            str += ' ';

        Root f{force(call_multimethod(first, &*s, 1))};
        auto ss = pr_str(*f);
        str.append(get_string_ptr(ss), get_string_len(ss));
    }
    str += ')';
    return create_string(str);
}

Value pr_str(Value val)
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
            return call_multimethod(lookup(PR_STR_OBJ), &val, 1);
    }
}

}
