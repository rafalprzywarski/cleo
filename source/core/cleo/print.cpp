#include "print.hpp"
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
            return nil;
    }
}

}
