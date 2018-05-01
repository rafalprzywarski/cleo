#include <cleo/error.hpp>
#include <cleo/eval.hpp>
#include <cleo/global.hpp>
#include <cleo/list.hpp>
#include <cleo/namespace.hpp>
#include <cleo/array.hpp>
#include <cleo/print.hpp>
#include <cleo/util.hpp>
#include <iostream>

cleo::Force create_command_line_args(int argc, const char *const* argv)
{
    cleo::Root command_line_args{argc > 4 ? *cleo::EMPTY_VECTOR : cleo::nil};
    cleo::Root arg;
    for (int i = 4; i < argc; ++i)
    {
        arg = cleo::create_string(argv[i]);
        command_line_args = cleo::array_conj(*command_line_args, *arg);
    }
    return *command_line_args;
}

cleo::Force create_ns_bindings(int argc, const char *const* argv)
{
    cleo::Root ns_bindings{cleo::map_assoc(*cleo::EMPTY_MAP, cleo::CURRENT_NS, *cleo::rt::current_ns)};
    cleo::Root command_line_args{create_command_line_args(argc, argv)};
    ns_bindings = cleo::map_assoc(*ns_bindings, cleo::COMMAND_LINE_ARGS, *command_line_args);

    cleo::Root root_lib_path{cleo::create_string(argv[1])};
    cleo::Root project_lib_path{cleo::create_string(argv[2])};
    std::array<cleo::Value, 2> paths{{*root_lib_path, *project_lib_path}};
    cleo::Root lib_paths{cleo::create_array(paths.data(), paths.size())};
    ns_bindings = cleo::map_assoc(*ns_bindings, cleo::LIB_PATHS, *lib_paths);
    return *ns_bindings;
}

int main(int argc, const char *const* argv)
{
    if (argc < 4)
    {
        std::cout << "usage: cleo <project_lib_path> <project_namespace>" << std::endl;
        return 1;
    }

    try
    {
        std::string ns_name = argv[3];
        auto ns = cleo::create_symbol(ns_name);
        cleo::Root ns_bindings{create_ns_bindings(argc, argv)};
        cleo::PushBindingsGuard bindings_guard{*ns_bindings};
        cleo::require(cleo::CLEO_CORE);
        cleo::in_ns(cleo::create_symbol("cleo.core.run"));
        cleo::refer(cleo::CLEO_CORE);
        cleo::require(ns);
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
        std::cout << std::string(cleo::get_string_ptr(*text), cleo::get_string_len(*text)) << std::endl;
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
