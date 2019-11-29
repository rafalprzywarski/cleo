#include "profiler.hpp"
#include "array.hpp"
#include <signal.h>
#include <sys/time.h>

namespace cleo
{
namespace prof
{
namespace
{

extern "C" void handle_sigprof(int, siginfo_t*, void *)
{
    if (main_thread_id != std::this_thread::get_id() || !callstack_copy_needed)
        return;
    callstack_copy_needed = false;
    std::copy(callstack, callstack + callstack_size, callstack_copy);
    callstack_size_copy = callstack_size;
    callstack_copy_ready = true;
}

void collector_thread()
{
    while (!finished)
    {
        callstack_copy_ready = false;
        callstack_copy_needed = true;
        while (!callstack_copy_ready && !finished)
            std::this_thread::yield();
        if (callstack_copy_ready)
            callstacks.emplace_back(callstack_copy, callstack_copy + callstack_size_copy);
    }
}

void set_sigprof_handler()
{
    struct sigaction action{};
    action.sa_sigaction = handle_sigprof;
    action.sa_flags = SA_RESTART | SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGPROF, &action, nullptr);
}

void set_timer(std::uint32_t msec)
{
    struct itimerval timer{};
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = msec * 1000;
    timer.it_interval = timer.it_value;
    setitimer(ITIMER_PROF, &timer, nullptr);
}

void reset_timer()
{
    struct itimerval timer{};
    setitimer(ITIMER_PROF, &timer, nullptr);
}

Force convert_callstacks()
{
    Root vs{transient_array(*EMPTY_VECTOR)};
    Root v;
    for (auto& cs : callstacks)
    {
        v = create_array(cs.data(), cs.size());
        vs = transient_array_conj(*vs, *v);
    }
    return transient_array_persistent(*vs);
}

}

Value start()
{
    if (collector.joinable())
        return nil;

    enabled = true;
    callstack_size = 0;
    callstack_copy_ready = false;
    callstack_copy_needed = false;
    callstacks.clear();
    callstacks.reserve(8192);
    finished = false;

    main_thread_id = std::this_thread::get_id();
    collector = std::thread{collector_thread};

    set_sigprof_handler();
    set_timer(10);

    return nil;
}

Force finish()
{
    if (!collector.joinable())
        return nil;

    reset_timer();

    enabled = false;
    finished = true;
    collector.join();

    Root converted{convert_callstacks()};
    callstacks.clear();
    return *converted;
}

}
}
