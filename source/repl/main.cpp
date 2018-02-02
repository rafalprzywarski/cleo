#include <cleo/reader.hpp>
#include <cleo/eval.hpp>
#include <cleo/print.hpp>
#include <cleo/global.hpp>
#include <cleo/error.hpp>
#include <cleo/var.hpp>
#include <cleo/namespace.hpp>
#include <cleo/small_map.hpp>
#include <iostream>
#include <readline/readline.h>

std::string get_current_ns()
{
    auto ns = *cleo::rt::current_ns;
    cleo::Root text{cleo::pr_str(ns)};
    return std::string(cleo::get_string_ptr(*text), cleo::get_string_len(*text));
}

bool eval_source(const std::string& line)
{
    try
    {
        cleo::Root str{cleo::create_string(line)};
        cleo::Root expr;
        try
        {
            expr = cleo::read(*str);
        }
        catch (const cleo::Exception& )
        {
            cleo::Root e{cleo::catch_exception()};
            auto type = cleo::get_value_type(*e);
            if (type.is(*cleo::type::UnexpectedEndOfInput))
                return false;
            cleo::throw_exception(*e);
        }
        cleo::Root result{cleo::eval(*expr)};
        cleo::Root text{cleo::pr_str(*result)};
        std::cout << std::string(cleo::get_string_ptr(*text), cleo::get_string_len(*text)) << std::endl;
    }
    catch (const cleo::Exception& )
    {
        cleo::Root e{cleo::catch_exception()};
        cleo::Root text{cleo::pr_str(*e)};
        std::cout << std::string(cleo::get_string_ptr(*text), cleo::get_string_len(*text)) << std::endl;
    }
    catch (std::exception const& e)
    {
        std::cout << "internal error: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "unknown internal error" << std::endl;
    }
    return true;
}

int main()
{
    char *line;

    cleo::Root ns_bindings{cleo::small_map_assoc(*cleo::EMPTY_MAP, cleo::CURRENT_NS, *cleo::rt::current_ns)};
    cleo::PushBindingsGuard bindings_guard{*ns_bindings};
    cleo::in_ns(cleo::create_symbol("user"));
    cleo::refer(cleo::CLEO_CORE);
    while ((line = readline((get_current_ns() + "=> ").c_str())) != nullptr)
    {
        std::string source = line;
        std::free(line);

        char *extra;
        while (
            !eval_source(source) &&
            (extra = readline((get_current_ns() + " > ").c_str())) != nullptr)
        {
            source += '\n';
            source += extra;
            std::free(extra);
        }

        add_history(source.c_str());
    }
}
