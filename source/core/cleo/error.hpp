#pragma once
#include <stdexcept>

namespace cleo
{

struct Error : std::runtime_error
{
    Error(const std::string& msg) : std::runtime_error(msg) {}
};

struct ReadError : Error
{
    ReadError(const std::string& msg) : Error(msg) {}
};

struct UnexpectedEndOfInput : ReadError
{
    UnexpectedEndOfInput() : ReadError("unexpected end of input") { }
};

struct IllegalArgument : Error
{
    IllegalArgument(const std::string& msg) : Error(msg) {}
};

struct SymbolNotFound : Error
{
    SymbolNotFound(const std::string& msg) : Error(msg) {}
};

struct CallError : Error
{
    CallError(const std::string& msg) : Error(msg) {}
};

}
