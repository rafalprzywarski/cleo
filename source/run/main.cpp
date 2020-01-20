#include <cleo/error.hpp>
#include <cleo/eval.hpp>
#include <cleo/global.hpp>
#include <cleo/list.hpp>
#include <cleo/namespace.hpp>
#include <cleo/array.hpp>
#include <cleo/print.hpp>
#include <cleo/util.hpp>
#include <iostream>

cleo::Force create_command_line_args(const std::vector<std::string>& args)
{
    cleo::Root command_line_args{args.size() > 4 ? *cleo::EMPTY_VECTOR : cleo::nil};
    cleo::Root arg;
    for (std::size_t i = 3; i < args.size(); ++i)
    {
        arg = cleo::create_string(args[i]);
        command_line_args = cleo::array_conj(*command_line_args, *arg);
    }
    return *command_line_args;
}

void split_paths(const std::string& s, cleo::Root& out)
{
    cleo::Root path;
    std::string::size_type spos = std::string::npos;
    do
    {
        ++spos;
        auto next_pos = s.find_first_of(":;", spos);
        path = cleo::create_string(s.substr(spos, next_pos - spos));
        out = array_conj(*out, *path);
        spos = next_pos;
    }
    while (spos != std::string::npos);
}

cleo::Force create_ns_bindings(const std::vector<std::string>& args)
{
    cleo::Root ns_bindings{cleo::map_assoc(*cleo::EMPTY_MAP, cleo::CURRENT_NS, *cleo::rt::current_ns)};
    cleo::Root command_line_args{create_command_line_args(args)};
    ns_bindings = cleo::map_assoc(*ns_bindings, cleo::COMMAND_LINE_ARGS, *command_line_args);

    cleo::Root lib_paths{*cleo::EMPTY_VECTOR};
    cleo::Root root_lib_path{cleo::create_string(args[0])};
    lib_paths = array_conj(*lib_paths, *root_lib_path);
    split_paths(args[1], lib_paths);
    ns_bindings = cleo::map_assoc(*ns_bindings, cleo::LIB_PATHS, *lib_paths);
    return *ns_bindings;
}

int main(int argc, const char *const* argv)
{
    std::vector<std::string> args{argv + 1, argv + argc};
    if (args.size() == 4 && args[1] == "--not-self-hosting")
    {
        define(cleo::SHOULD_RECOMPILE, cleo::nil);
        args.erase(begin(args) + 1);
    }
    if (args.size() != 3)
    {
        std::cout << "usage: cleo [--not-self-hosting] <project_lib_path> <project_namespace>" << std::endl;
        return 1;
    }

    try
    {
        std::string ns_name = args[2];
        auto ns = cleo::create_symbol(ns_name);
        cleo::Root ns_bindings{create_ns_bindings(args)};
        cleo::PushBindingsGuard bindings_guard{*ns_bindings};
        cleo::require(cleo::CLEO_CORE, cleo::nil);
        cleo::in_ns(cleo::create_symbol("cleo.core.run"));
        cleo::refer(cleo::CLEO_CORE);
        cleo::require(ns, cleo::nil);
        auto main_fn = cleo::create_symbol(ns_name, "main");
        cleo::Root call_main{cleo::create_list(&main_fn, 1)};
        cleo::Root exit_code{cleo::eval(*call_main)};
        if (get_value_tag(*exit_code) == cleo::tag::INT64)
            return get_int64_value(*exit_code);
    }
    catch (const cleo::Exception& )
    {
        cleo::Root e{cleo::catch_exception()};
        cleo::Root text{cleo::pr_str(*e)};
        std::cout << std::string(cleo::get_string_ptr(*text), cleo::get_string_size(*text)) << std::endl;
        return 2;
    }
    catch (std::exception const& e)
    {
        std::cout << "internal error: " << e.what() << std::endl;
        return 3;
    }
    catch (...)
    {
        std::cout << "unknown internal error" << std::endl;
        return 3;
    }
    return 0;
}
