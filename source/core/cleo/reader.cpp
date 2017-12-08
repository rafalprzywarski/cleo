#include "reader.hpp"
#include "list.hpp"
#include "small_vector.hpp"
#include "global.hpp"
#include "error.hpp"
#include <cctype>
#include <sstream>

namespace cleo
{

class Stream
{
public:
    Stream(Value text) : text(text) { }

    char peek(std::uint32_t n = 0) const { return eos(n) ? 0 : get_string_ptr(text)[index + n]; }
    bool eos(std::uint32_t n = 0) const { return (index + n) >= get_string_len(text); }
    char next()
    {
        char c = peek();
        if (!eos())
            index++;
        return c;
    }

private:
    std::uint32_t index = 0;
    Value text{nil};
};

Force read(Stream& s);

bool is_symbol_char(char c)
{
    return std::isalpha(c) || std::isdigit(c) || c == '-' || c == '+';
}

Force read_number(Stream& s)
{
    std::stringstream ss;
    if (s.peek() == '-')
        ss << s.next();
    while (std::isdigit(s.peek()))
        ss << s.next();
    Int64 n;
    ss >> n;
    return create_int64(n);
}

Force read_symbol(Stream& s)
{
    std::string str;
    while (is_symbol_char(s.peek()))
        str += s.next();
    return str == "nil" ? nil : create_symbol(str);
}

Force read_keyword(Stream& s)
{
    s.next(); // ':'
    std::string str;
    while (is_symbol_char(s.peek()))
        str += s.next();
    return str == "nil" ? nil : create_keyword(str);
}

void eat_ws(Stream& s)
{
    while (!s.eos() && s.peek() <= ' ')
        s.next();
}

Force read_list(Stream& s)
{
    s.next(); // '('
    eat_ws(s);
    Root l{create_list(nullptr, 0)};

    while (!s.eos() && s.peek() != ')')
    {
        Root e{read(s)};
        l = list_conj(*l, *e);
        eat_ws(s);
    }
    if (s.eos())
        throw ReadError("unexpected end of input");
    s.next(); // ')'
    l = list_seq(*l);
    Root lr{create_list(nullptr, 0)};
    while (*l != nil)
    {
        lr = list_conj(*lr, get_list_first(*l));
        l = get_list_next(*l);
    }
    return *lr;
}

Force read_vector(Stream& s)
{
    s.next(); // '['
    eat_ws(s);
    Root v{create_small_vector(nullptr, 0)};

    while (!s.eos() && s.peek() != ']')
    {
        Root e{read(s)};
        v = small_vector_conj(*v, *e);
        eat_ws(s);
    }
    if (s.eos())
        throw ReadError("unexpected end of input");
    s.next(); // ']'
    return *v;
}

Force read_string(Stream& s)
{
    std::string str;
    s.next();
    while (!s.eos() && s.peek() != '\"')
    {
        if (s.peek() == '\\')
        {
            s.next();
            char c = s.next();
            if (c == 'n')
                str += '\n';
            else
                str += c;
        }
        else
            str += s.next();
    }
    if (s.eos())
        throw ReadError("unexpected end of input");
    s.next();
    return create_string(str);
}

Force read_quote(Stream& s)
{
    s.next(); // '\''
    Root val{read(s)};
    std::array<Value, 2> elems{{QUOTE, *val}};
    return create_list(elems.data(), elems.size());
}

Force read(Stream& s)
{
    eat_ws(s);

    if (std::isdigit(s.peek()) || (s.peek() == '-' && std::isdigit(s.peek(1))))
        return read_number(s);
    if (is_symbol_char(s.peek()))
        return read_symbol(s);
    if (s.peek() == ':')
        return read_keyword(s);
    if (s.peek() == '(')
        return read_list(s);
    if (s.peek() == '[')
        return read_vector(s);
    if (s.peek() == '\"')
        return read_string(s);
    if (s.peek() == '\'')
        return read_quote(s);
    throw ReadError(std::string("unexpected ") + s.peek());
}

Force read(Value source)
{
    if (get_value_tag(source) != tag::STRING)
        throw IllegalArgument("expected a string");
    Stream s(source);
    return read(s);
}

}
