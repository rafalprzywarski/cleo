#include <cleo/reader.hpp>
#include <cleo/eval.hpp>
#include <cleo/print.hpp>
#include <cleo/global.hpp>
#include <cleo/error.hpp>
#include <cleo/var.hpp>
#include <cleo/namespace.hpp>
#include <cleo/array.hpp>
#include <cleo/util.hpp>
#include <cleo/list.hpp>
#include <iostream>
#include <readline/readline.h>
#include <readline/history.h>

std::string get_current_ns()
{
    auto ns = ns_name(*cleo::rt::current_ns);
    cleo::Root text{cleo::pr_str(ns)};
    return std::string(cleo::get_string_ptr(*text), cleo::get_string_len(*text));
}

bool eval_source(const std::string& line)
{
    try
    {
        cleo::Root str{cleo::create_string(line)};
        try
        {
            cleo::ReaderStream stream{*str};
            while (!stream.eos())
                cleo::read(stream);
        }
        catch (const cleo::Exception& )
        {
            cleo::Root e{cleo::catch_exception()};
            auto type = cleo::get_value_type(*e);
            if (type.is(*cleo::type::UnexpectedEndOfInput))
                return false;
            cleo::throw_exception(*e);
        }
        cleo::ReaderStream stream{*str};
        while (!stream.eos())
        {
            cleo::Root expr{cleo::read(stream)};
            cleo::Root result{cleo::eval(*expr)};
            cleo::Root text{cleo::pr_str(*result)};
            std::cout << std::string(cleo::get_string_ptr(*text), cleo::get_string_len(*text)) << std::endl;
        }
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

cleo::Force create_ns_bindings(int argc, const char *const* argv)
{
    cleo::Root ns_bindings{cleo::map_assoc(*cleo::EMPTY_MAP, cleo::CURRENT_NS, *cleo::rt::current_ns)};
    cleo::Root root_lib_path{cleo::create_string(argv[1])};
    cleo::Root project_lib_path{cleo::create_string(".")};
    std::array<cleo::Value, 2> paths{{*root_lib_path, *project_lib_path}};
    cleo::Root lib_paths{cleo::create_array(paths.data(), paths.size())};
    ns_bindings = cleo::map_assoc(*ns_bindings, cleo::LIB_PATHS, *lib_paths);
    return *ns_bindings;
}

int main(int argc, const char *const* argv)
{
    char *line;

    if (argc < 2)
    {
        std::cout << "error: invalid configuration" << std::endl;
        return 1;
    }
    cleo::Root ns_bindings{create_ns_bindings(argc, argv)};
    cleo::PushBindingsGuard bindings_guard{*ns_bindings};
    cleo::require(cleo::CLEO_CORE, cleo::nil);
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
