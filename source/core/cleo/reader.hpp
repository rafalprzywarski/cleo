#pragma once
#include "value.hpp"
#include "error.hpp"

namespace cleo
{

class ReaderStream
{
public:
    struct Position
    {
        std::uint32_t line{}, col{};
    };

    ReaderStream(Value text) : text(text) { }

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

Force read(ReaderStream& stream);
Force read(Value source);

}
