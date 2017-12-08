#include <cleo/reader.hpp>
#include <cleo/eval.hpp>
#include <cleo/print.hpp>
#include <cleo/global.hpp>
#include <cleo/error.hpp>
#include <iostream>
#include <readline/readline.h>

void eval_line(const std::string& line)
{
    try
    {
        cleo::Root str{cleo::create_string(line)};
        cleo::Root expr{cleo::read(*str)};
        cleo::Root result{cleo::eval(*expr)};
        cleo::Root text{cleo::pr_str(*result)};
        std::cout << "> " << std::string(cleo::get_string_ptr(*text), cleo::get_string_len(*text)) << std::endl;
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
}

int main()
{
    char *line;

    while ((line = readline("$ ")) != nullptr)
    {
        add_history(line);
        eval_line(line);
        std::free(line);
    }
}
