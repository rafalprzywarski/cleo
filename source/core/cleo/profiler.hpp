#pragma once
#include "global.hpp"

namespace cleo
{
namespace prof
{

class Trace
{
public:
    Trace() = default;
    void do_trace(Value fn_name)
    {
        if (callstack_size == MAX_CALLSTACK_SIZE)
            return;
        skip = false;
        callstack[callstack_size] = fn_name;
        ++callstack_size;
    }
    ~Trace()
    {
        if (skip)
            return;
        assert(callstack_size > 0);
        --callstack_size;
    }
    Trace(const Trace&) = delete;
    Trace& operator=(const Trace&) = delete;
private:
    bool skip = true;
};

#define CLEO_PROF_TRACE(name, expr) \
    ::cleo::prof::Trace name{}; \
    if (::cleo::prof::enabled) name.do_trace(expr); else;

Value start();
Force finish();

}
}
