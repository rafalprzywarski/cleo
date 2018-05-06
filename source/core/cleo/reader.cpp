#include "reader.hpp"
#include "list.hpp"
#include "array.hpp"
#include "persistent_hash_map.hpp"
#include "array_set.hpp"
#include "global.hpp"
#include "error.hpp"
#include "print.hpp"
#include "namespace.hpp"
#include <cctype>
#include <sstream>

namespace cleo
{

namespace
{

class Stream
{
public:
    struct Position
    {
        std::uint32_t line{}, col{};
    };

    Stream(Value text) : text(text) { }

    char peek(std::uint32_t n = 0) const { return eos(n) ? 0 : get_string_ptr(text)[index + n]; }
    bool eos(std::uint32_t n = 0) const { return (index + n) >= get_string_len(text); }
    char next()
    {
        char c = peek();
        if (c == '\n')
        {
            ++line;
            col = 1;
        }
        else
            ++col;
        if (!eos())
            index++;
        return c;
    }

    Position pos() const
    {
        return {line, col};
    }

private:
    std::uint32_t index = 0;
    std::uint32_t line = 1, col = 1;
    Value text{nil};
};

Force read(Stream& s);

[[noreturn]] void throw_read_error(const std::string& msg, Stream::Position pos)
{
    Root msgv{create_string(msg)};
    Root linev{create_int64(pos.line)}, colv{create_int64(pos.col)};
    throw_exception(new_read_error(*msgv, *linev, *colv));
}

bool is_symbol_char(char c)
{
    return
        std::isalpha(c) || std::isdigit(c) || c == '-' || c == '+' ||
        c == '.' || c == '*' || c == '=' || c == '<' || c == '&' ||
        c == '!' || c == '?' || c == '#' || c == '_';
}

bool is_ws(char c)
{
    return c <= ' ' || c == ',';
}

Force read_integer(const std::string& n, Stream::Position pos)
{
    char *end = nullptr;
    errno = 0;
    auto val = std::strtoll(n.c_str(), &end, 0);
    if (errno)
        throw_read_error("integer out of range: " + n, pos);
    if (end != n.c_str() + n.length())
        throw_read_error("malformed number: " + n, pos);
    return create_int64(val);
}

Force read_float(const std::string& n, Stream::Position pos)
{
    char *end = nullptr;
    errno = 0;
    auto val = std::strtod(n.c_str(), &end);
    if (errno)
        throw_read_error("floating-point value out of range: " + n, pos);
    if (end != n.c_str() + n.length())
        throw_read_error("malformed number: " + n, pos);
    return create_float64(val);
}

Force read_number(Stream& s)
{
    auto pos = s.pos();
    std::string n;
    while (is_symbol_char(s.peek()))
        n += s.next();
    return
        n.find('.') != std::string::npos || n.find('e') != std::string::npos || n.find('E') != std::string::npos ?
        read_float(n, pos) :
        read_integer(n, pos);
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
    while (!s.eos() && (is_ws(s.peek()) || s.peek() == ';'))
    {
        if (s.peek() == ';')
        {
            while (!s.eos() && s.peek() != '\n')
                s.next();
            if (!s.eos())
                s.next();
        }
        while (!s.eos() && is_ws(s.peek()))
            s.next();
    }
}

[[noreturn]] void throw_unexpected_end_of_input(Stream::Position pos)
{
    Root linev{create_int64(pos.line)}, colv{create_int64(pos.col)};
    throw_exception(new_unexpected_end_of_input(*linev, *colv));
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
        throw_unexpected_end_of_input(s.pos());
    s.next(); // ')'
    l = list_seq(*l);
    Root lr{*EMPTY_LIST};
    while (*l)
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
        v = array_conj(*v, *e);
        eat_ws(s);
    }
    if (s.eos())
        throw_unexpected_end_of_input(s.pos());
    s.next(); // ']'
    return *v;
}

Force read_map(Stream& s)
{
    auto brace_pos = s.pos();
    s.next(); // '{'
    eat_ws(s);
    Root m{*EMPTY_MAP};

    while (!s.eos() && s.peek() != '}')
    {
        Root k{read(s)};
        eat_ws(s);

        if (s.peek() == '}')
            throw_read_error("map literal must contain an even number of forms", brace_pos);

        Root v{read(s)};
        eat_ws(s);

        m = persistent_hash_map_assoc(*m, *k, *v);
    }
    if (s.eos())
        throw_unexpected_end_of_input(s.pos());
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
        throw_unexpected_end_of_input(s.pos());
    s.next();
    return create_string(str);
}

Force quote(Value val)
{
    std::array<Value, 2> elems{{QUOTE, val}};
    return create_list(elems.data(), elems.size());
}

Force read_quote(Stream& s)
{
    s.next(); // '\''
    Root val{read(s)};
    return quote(*val);
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

Force syntax_quote_resolve_symbol(Value sym)
{
    if (SPECIAL_SYMBOLS.count(sym))
        return sym;
    auto ns{*rt::current_ns};
    sym = resolve(ns, sym);
    if (get_symbol_namespace(sym))
        return sym;
    auto sym_ns = get_symbol_name(ns);
    auto sym_name = get_symbol_name(sym);
    return create_symbol({get_string_ptr(sym_ns), get_string_len(sym_ns)}, {get_string_ptr(sym_name), get_string_len(sym_name)});
}

Force syntax_quote_symbol(Value sym)
{
    Root val{syntax_quote_resolve_symbol(sym)};
    return quote(*val);
}

Force syntax_quote(Value val)
{
    if (get_value_tag(val) == tag::SYMBOL)
        return syntax_quote_symbol(val);
    return val;
}

Force read_syntax_quote(Stream& s)
{
    s.next(); // '`'
    Root val{read(s)};
    if (!*rt::syntax_quote_in_reader)
    {
        std::array<Value, 2> elems{{SYNTAX_QUOTE, *val}};
        return create_list(elems.data(), elems.size());
    }
    return syntax_quote(*val);
}

Force read_set(Stream& s)
{
    s.next(); // '#'
    s.next(); // '{'
    eat_ws(s);
    Root set{create_array_set()};

    while (!s.eos() && s.peek() != '}')
    {
        auto key_pos = s.pos();
        Root e{read(s)};
        if (array_set_contains(*set, *e))
        {
            Root text{pr_str(*e)};
            throw_read_error("duplicate key: " + std::string(get_string_ptr(*text), get_string_len(*text)), key_pos);
        }
        set = array_set_conj(*set, *e);
        eat_ws(s);
    }
    if (s.eos())
        throw_unexpected_end_of_input(s.pos());
    s.next(); // '}'
    return *set;
}

Force read_var(Stream& s)
{
    s.next(); // '#'
    s.next(); // '\''
    eat_ws(s);
    if (s.eos())
        throw_unexpected_end_of_input(s.pos());
    auto sym_pos = s.pos();
    Root sym{read(s)};
    if (get_value_tag(*sym) != tag::SYMBOL)
        throw_read_error("expected a symbol", sym_pos);
    return lookup_var(resolve(*sym));
}

[[noreturn]] void throw_unexpected(char c, Stream::Position pos)
{
    Root e{create_string(std::string("unexpected ") + c)};
    Root linev{create_int64(pos.line)}, colv{create_int64(pos.col)};
    throw_exception(new_read_error(*e, *linev, *colv));
}

Force read(Stream& s)
{
    eat_ws(s);

    if (std::isdigit(s.peek()) || (s.peek() == '-' && std::isdigit(s.peek(1))))
        return read_number(s);
    switch (s.peek())
    {
        case ':': return read_keyword(s);
        case '(': return read_list(s);
        case '[': return read_vector(s);
        case '{': return read_map(s);
        case '\"': return read_string(s);
        case '\'': return read_quote(s);
        case '@': return read_deref(s);
        case '~': return (s.peek(1) == '@') ? read_unquote_splicing(s) : read_unquote(s);
        case '`': return read_syntax_quote(s);
        case '#':
            switch (s.peek(1))
            {
                case '{': return read_set(s);
                case '\'': return read_var(s);
            }
            throw_unexpected(s.peek(), s.pos());
            break;
    }
    if (is_symbol_char(s.peek()))
        return read_symbol(s);
    throw_unexpected(s.peek(), s.pos());
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
        forms = array_conj(*forms, *form);
        eat_ws(s);
    }

    return *forms;
}

}
