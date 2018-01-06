#include "reader.hpp"
#include "list.hpp"
#include "small_vector.hpp"
#include "small_map.hpp"
#include "small_set.hpp"
#include "global.hpp"
#include "error.hpp"
#include "print.hpp"
#include <cctype>
#include <sstream>

namespace cleo
{

namespace
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
    return
        std::isalpha(c) || std::isdigit(c) || c == '-' || c == '+' ||
        c == '.' || c == '*' || c == '=' || c == '<' || c == '&' ||
        c == '!' || c == '?';
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
    if (s.peek() == '/')
    {
        s.next();
        std::string name;
        while (is_symbol_char(s.peek()))
            name += s.next();
        return create_symbol(str, name);
    }
    return str == "nil" ? nil : create_symbol(str);
}

Force read_keyword(Stream& s)
{
    s.next(); // ':'
    std::string str;
    while (is_symbol_char(s.peek()))
        str += s.next();
    return create_keyword(str);
}

void eat_ws(Stream& s)
{
    while ((!s.eos() && s.peek() <= ' ') || s.peek() == ',')
        s.next();
}

Force read_list(Stream& s)
{
    s.next(); // '('
    eat_ws(s);
    Root l{*EMPTY_LIST};

    while (!s.eos() && s.peek() != ')')
    {
        Root e{read(s)};
        l = list_conj(*l, *e);
        eat_ws(s);
    }
    if (s.eos())
        throw_exception(new_unexpected_end_of_input());
    s.next(); // ')'
    l = list_seq(*l);
    Root lr{*EMPTY_LIST};
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
    Root v{*EMPTY_VECTOR};

    while (!s.eos() && s.peek() != ']')
    {
        Root e{read(s)};
        v = small_vector_conj(*v, *e);
        eat_ws(s);
    }
    if (s.eos())
        throw_exception(new_unexpected_end_of_input());
    s.next(); // ']'
    return *v;
}

Force read_map(Stream& s)
{
    s.next(); // '{'
    eat_ws(s);
    Root m{*EMPTY_MAP};

    while (!s.eos() && s.peek() != '}')
    {
        Root k{read(s)};
        eat_ws(s);

        if (s.peek() == '}')
        {
            Root e{create_string("map literal must contain an even number of forms")};
            e = new_read_error(*e);
            throw_exception(*e);
        }

        Root v{read(s)};
        eat_ws(s);

        m = small_map_assoc(*m, *k, *v);
    }
    if (s.eos())
        throw_exception(new_unexpected_end_of_input());
    s.next(); // '}'
    return *m;
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
        throw_exception(new_unexpected_end_of_input());
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

Force read_deref(Stream& s)
{
    s.next(); // '@'
    Root val{read(s)};
    std::array<Value, 2> elems{{DEREF, *val}};
    return create_list(elems.data(), elems.size());
}

Force read_unquote(Stream& s)
{
    s.next(); // '~'
    Root val{read(s)};
    std::array<Value, 2> elems{{UNQUOTE, *val}};
    return create_list(elems.data(), elems.size());
}

Force read_unquote_splicing(Stream& s)
{
    s.next(); // '~'
    s.next(); // '@'
    Root val{read(s)};
    std::array<Value, 2> elems{{UNQUOTE_SPLICING, *val}};
    return create_list(elems.data(), elems.size());
}

Force read_set(Stream& s)
{
    s.next(); // '#'
    s.next(); // '{'
    eat_ws(s);
    Root set{create_small_set()};

    while (!s.eos() && s.peek() != '}')
    {
        Root e{read(s)};
        if (small_set_contains(*set, *e))
        {
            Root text{pr_str(*e)};
            Root e{create_string("duplicate key: " + std::string(get_string_ptr(*text), get_string_len(*text)))};
            throw_exception(new_read_error(*e));
        }
        set = small_set_conj(*set, *e);
        eat_ws(s);
    }
    if (s.eos())
        throw_exception(new_unexpected_end_of_input());
    s.next(); // '}'
    return *set;
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
    if (s.peek() == '{')
        return read_map(s);
    if (s.peek() == '\"')
        return read_string(s);
    if (s.peek() == '\'')
        return read_quote(s);
    if (s.peek() == '@')
        return read_deref(s);
    if (s.peek() == '~' && s.peek(1) == '@')
        return read_unquote_splicing(s);
    if (s.peek() == '~')
        return read_unquote(s);
    if (s.peek() == '#' && s.peek(1) == '{')
        return read_set(s);
    Root e{create_string(std::string("unexpected ") + s.peek())};
    e = new_read_error(*e);
    throw_exception(*e);
}

}

Force read(Value source)
{
    if (get_value_tag(source) != tag::STRING)
    {
        Root msg{create_string("expected a string")};
        throw_exception(new_illegal_argument(*msg));
    }
    Stream s(source);
    return read(s);
}

Force read_forms(Value source)
{
    if (get_value_tag(source) != tag::STRING)
    {
        Root msg{create_string("expected a string")};
        throw_exception(new_illegal_argument(*msg));
    }

    Stream s(source);
    Root forms{*EMPTY_VECTOR}, form;

    eat_ws(s);
    while (!s.eos())
    {
        form = read(s);
        forms = small_vector_conj(*forms, *form);
        eat_ws(s);
    }

    return *forms;
}

}
