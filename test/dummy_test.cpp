#include <gtest/gtest.h>
#include <cleo/dummy.hpp>

TEST(dummy_test, passes)
{
    ASSERT_EQ(7, dummy());
}
