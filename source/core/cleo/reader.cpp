#include "reader.hpp"
#include "list.hpp"
#include "array.hpp"
#include "persistent_hash_map.hpp"
#include "array_set.hpp"
#include "global.hpp"
#include "error.hpp"
#include "print.hpp"
#include "namespace.hpp"
#include "util.hpp"
#include "multimethod.hpp"
#include <cctype>
#include <sstream>

namespace cleo
{

namespace
{

using Stream = ReaderStream;

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
        c == '.' || c == '*' || c == '=' || c == '<' || c == '>' || c == '&' ||
        c == '!' || c == '?' || c == '#' || c == '_';
}

bool is_char_char(char c)
{
    return std::isalnum(c);
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
        n.find('.') != std::string::npos ||
        (n.find("0x") != 0 &&
         n.find("0X") != 0 &&
         (n.find('e') != std::string::npos || n.find('E') != std::string::npos)) ?
        read_float(n, pos) :
        read_integer(n, pos);
}

std::pair<std::string, std::string> read_raw_symbol(Stream& s)
{
    std::string str;
    if (s.peek() == '/')
    {
        s.next();
        return {{}, "/"};
    }
    while (is_symbol_char(s.peek()))
        str += s.next();
    if (s.peek() == '/')
    {
        s.next();
        std::string name;
        while (is_symbol_char(s.peek()) || s.peek() == '/')
            name += s.next();
        return {str, name};
    }
    return {{}, str};
}

Force read_symbol(Stream& s)
{
    auto rs = read_raw_symbol(s);
    if (rs.first.empty())
        return rs.second == "nil" ? nil : create_symbol(rs.second);
    return create_symbol(rs.first, rs.second);
}

Force read_keyword(Stream& s)
{
    s.next(); // ':'
    if (s.peek() == ':')
    {
        s.next(); // ':'
        auto rs = read_raw_symbol(s);
        auto ns = rs.first.empty() ?
            get_symbol_name(ns_name(*rt::current_ns)) :
            get_symbol_name(ns_name(map_get(ns_aliases(ns_name(*rt::current_ns)), create_symbol(rs.first))));
        return create_keyword({get_string_ptr(ns), get_string_len(ns)}, rs.second);
    }
    auto rs = read_raw_symbol(s);
    if (rs.first.empty())
        return create_keyword(rs.second);
    return create_keyword(rs.first, rs.second);
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
    Root v{transient_array(*EMPTY_VECTOR)};

    while (!s.eos() && s.peek() != ']')
    {
        Root e{read(s)};
        v = transient_array_conj(*v, *e);
        eat_ws(s);
    }
    if (s.eos())
        throw_unexpected_end_of_input(s.pos());
    s.next(); // ']'
    v = transient_array_persistent(*v);
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

        m = map_assoc(*m, *k, *v);
    }
    if (s.eos())
        throw_unexpected_end_of_input(s.pos());
    s.next(); // '}'
    return *m;
}

std::uint32_t parse_hex_digit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - ('a' - 10);
    if (c >= 'A' && c <= 'F')
        return c - ('A' - 10);
    throw std::invalid_argument("Invalid hex digit");
}

std::uint32_t read_hex_digit(Stream& s, int n)
{
    auto pos = s.pos();
    try
    {
        return parse_hex_digit(s.next());
    }
    catch (const std::invalid_argument& )
    {
        throw_read_error(std::string("invalid character length: ") + std::to_string(n), pos);
    }
}

void append_utf8(std::string& s, std::uint32_t c)
{
    if (c < 0x80)
    {
        s += char(c);
        return;
    }
    if (c < 0x800)
    {
        s += 0xc0 | (c >> 6);
        s += 0x80 | (c & 0x3f);
        return;
    }
    s += 0xe0 | (c >> 12);
    s += 0x80 | ((c >> 6) & 0x3f);
    s += 0x80 | (c & 0x3f);
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
            switch (c) {
            case 'n': str += '\n'; break;
            case 'u':
            {
                std::uint32_t code = read_hex_digit(s, 0) << 12;
                code |= read_hex_digit(s, 1) << 8;
                code |= read_hex_digit(s, 2) << 4;
                code |= read_hex_digit(s, 3);
                append_utf8(str, code);
                break;
            }
            default:
                str += c;
            }
        }
        else
            str += s.next();
    }
    if (s.eos())
        throw_unexpected_end_of_input(s.pos());
    s.next();
    return create_string(str);
}

Value read_char(Stream& s)
{
    auto pos = s.pos();
    s.next();
    if (s.eos())
        throw_unexpected_end_of_input(s.pos());
    std::string ch;
    ch += s.next();
    while (!s.eos() && is_char_char(s.peek()))
    {
        ch += s.next();
    }
    if (ch.length() == 1)
        return create_char32(ch[0]);
    if (ch == "newline")
        return create_char32('\n');
    if (ch == "space")
        return create_char32(' ');
    if (ch == "tab")
        return create_char32('\t');
    if (ch == "formfeed")
        return create_char32('\f');
    if (ch == "backspace")
        return create_char32('\b');
    if (ch == "return")
        return create_char32('\r');
    if (ch[0] == 'u')
    {
        try
        {
            if (ch.length() == 3)
                return create_char32((parse_hex_digit(ch[1]) << 4) |
                                     parse_hex_digit(ch[2]));
            if (ch.length() == 5)
                return create_char32((parse_hex_digit(ch[1]) << 12) |
                                     (parse_hex_digit(ch[2]) << 8) |
                                     (parse_hex_digit(ch[3]) << 4) |
                                     parse_hex_digit(ch[4]));
            if (ch.length() == 7)
            {
                auto code =
                    (parse_hex_digit(ch[1]) << 20) |
                    (parse_hex_digit(ch[2]) << 16) |
                    (parse_hex_digit(ch[3]) << 12) |
                    (parse_hex_digit(ch[4]) << 8) |
                    (parse_hex_digit(ch[5]) << 4) |
                    parse_hex_digit(ch[6]);
                if (code < 0x110000)
                    return create_char32(code);
            }
        }
        catch (const std::invalid_argument& ) { }
    }
    throw_read_error("invalid character: \\" + ch, pos);
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

Force syntax_quote(Root& generated, Value val);

bool is_generating(Value sym)
{
    auto name = get_symbol_name(sym);
    return
        !get_symbol_namespace(sym) &&
        get_string_len(name) > 0 &&
        get_string_ptr(name)[get_string_len(name) - 1] == '#';
}

Force generate_symbol(Root& generated, Value sym)
{
    Root found{call_multimethod2(*rt::get, *generated, sym)};
    if (*found)
        return *found;
    auto name = get_symbol_name(sym);
    auto id = gen_id();
    Root g{create_symbol(std::string(get_string_ptr(name), get_string_len(name) - 1) + "__" + std::to_string(id) + "__auto__")};
    generated = map_assoc(*generated, sym, *g);
    return *g;
}

Force syntax_quote_resolve_symbol(Root& generated, Value sym)
{
    if (get_symbol_namespace(sym))
        return sym;
    if (SPECIAL_SYMBOLS.count(sym))
        return sym;
    if (is_generating(sym))
        return generate_symbol(generated, sym);
    auto ns{ns_name(*rt::current_ns)};
    auto var = maybe_resolve_var(ns, sym);
    if (var)
        return get_var_name(var);
    auto sym_ns = get_symbol_name(ns);
    auto sym_name = get_symbol_name(sym);
    return create_symbol({get_string_ptr(sym_ns), get_string_len(sym_ns)}, {get_string_ptr(sym_name), get_string_len(sym_name)});
}

Force syntax_quote_symbol(Root& generated, Value sym)
{
    Root val{syntax_quote_resolve_symbol(generated, sym)};
    return quote(*val);
}

Force syntax_quote_seq_elem(Root& generated, Value v)
{
    if (get_value_type(v).is(*type::List) && get_list_first(v).is(UNQUOTE_SPLICING))
    {
        auto n = get_list_next(v);
        return n ? get_list_first(n) : nil;
    }
    Root q{syntax_quote(generated, v)};
    std::array<Value, 2> l{{LIST, *q}};
    return create_list(l.data(), l.size());
}

Force list_reverse(Value l)
{
    if (get_list_size(l) == 0)
        return l;
    Root r{*EMPTY_LIST};
    for (; l; l = get_list_next(l))
        r = list_conj(*r, get_list_first(l));
    return *r;
}

Force syntax_quote_seq(Root& generated, Value conv, Value l)
{
    Root seq{*EMPTY_LIST}, val;
    for (Root s{call_multimethod1(*rt::seq, l)}; *s; s = call_multimethod1(*rt::next, *s))
    {
        val = call_multimethod1(*rt::first, *s);
        val = syntax_quote_seq_elem(generated, *val);
        seq = list_conj(*seq, *val);
    }

    seq = list_reverse(*seq);
    seq = list_conj(*seq, CONCATI);
    std::array<Value, 3> apply_vector{{APPLY, conv, *seq}};
    return create_list(apply_vector.data(), apply_vector.size());
}

Force flatten_kv(Value m)
{
    Root seq{*EMPTY_LIST}, kv;
    for (Root s{call_multimethod1(*rt::seq, m)}; *s; s = call_multimethod1(*rt::next, *s))
    {
        kv = call_multimethod1(*rt::first, *s);
        seq = list_conj(*seq, get_array_elem(*kv, 1));
        seq = list_conj(*seq, get_array_elem(*kv, 0));
    }
    return *seq;
}

Force syntax_quote(Root& generated, Value val)
{
    if (get_value_tag(val) == tag::SYMBOL)
        return syntax_quote_symbol(generated, val);
    auto type = get_value_type(val);
    if (type.is(*type::Array))
        return syntax_quote_seq(generated, VECTOR, val);
    if (type.is(*type::List))
    {
        if (get_list_first(val).is(UNQUOTE))
        {
            auto n = get_list_next(val);
            return n ? get_list_first(n) : nil;
        }
        if (get_list_first(val).is(UNQUOTE_SPLICING))
            throw_illegal_state("splice not in list");
        return syntax_quote_seq(generated, LIST, val);
    }
    if (type.is(*type::ArraySet))
        return syntax_quote_seq(generated, HASH_SET, val);
    if (isa(type, *type::PersistentMap))
    {
        Root kvs{flatten_kv(val)};
        return syntax_quote_seq(generated, HASH_MAP, *kvs);
    }
    return val;
}

Force syntax_quote(Value val)
{
    Root generated{*EMPTY_MAP};
    return syntax_quote(generated, val);
}

Force read_syntax_quote(Stream& s)
{
    s.next(); // '`'
    Root val{read(s)};
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
    return resolve_var(*sym);
}

[[noreturn]] void throw_unexpected(char c, Stream::Position pos)
{
    Root e{create_string(std::string("unexpected ") + c)};
    Root linev{create_int64(pos.line)}, colv{create_int64(pos.col)};
    throw_exception(new_read_error(*e, *linev, *colv));
}

Force read_form(Stream& s)
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
        case '\\': return read_char(s);
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
    if (is_symbol_char(s.peek()) || s.peek() == '/')
        return read_symbol(s);
    throw_unexpected(s.peek(), s.pos());
}

}

Force read(ReaderStream& stream)
{
    Root form{read_form(stream)};
    eat_ws(stream);
    return *form;
}

Force read(Value source)
{
    if (get_value_tag(source) != tag::UTF8STRING)
    {
        Root msg{create_string("expected a string")};
        throw_exception(new_illegal_argument(*msg));
    }
    Stream s(source);
    return read(s);
}

}
