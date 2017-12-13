#include <cleo/reader.hpp>
#include <cleo/eval.hpp>
#include <cleo/print.hpp>
#include <cleo/global.hpp>
#include <cleo/error.hpp>
#include <iostream>
#include <readline/readline.h>

bool eval_source(const std::string& line)
{
    try
    {
        cleo::Root str{cleo::create_string(line)};
        cleo::Root expr{cleo::read(*str)};
        cleo::Root result{cleo::eval(*expr)};
        cleo::Root text{cleo::pr_str(*result)};
        std::cout << std::string(cleo::get_string_ptr(*text), cleo::get_string_len(*text)) << std::endl;
    }
    catch (const cleo::Exception& )
    {
        cleo::Root value{cleo::pr_str(*cleo::current_exception)};
        cleo::current_exception = cleo::nil;
        std::cout << "exception: " << std::string(cleo::get_string_ptr(*value), cleo::get_string_len(*value)) << std::endl;
    }
    catch (const cleo::UnexpectedEndOfInput& )
    {
        return false;
    }
    catch (cleo::Error const& e)
    {
        std::cout << "error: " << e.what() << std::endl;
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

    while ((line = readline("$ ")) != nullptr)
    {
        std::string source = line;
        std::free(line);

        char *extra;
        while (
            !eval_source(source) &&
            (extra = readline("> ")) != nullptr)
        {
            source += '\n';
            source += extra;
            std::free(extra);
        }

        add_history(source.c_str());
    }
}
