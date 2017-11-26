#pragma once

namespace cleo
{

template <typename T, typename Tag>
struct singleton
{
    T& operator*()
    {
        static T instance;
        return instance;
    }

    T *operator->() { return &**this; }
};

}
