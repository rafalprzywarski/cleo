#pragma once
#include <sstream>
#include <iostream>
#include <gtest/gtest.h>

class test_listener : public testing::EmptyTestEventListener
{
public:
    void OnTestEnd(const testing::TestInfo& testInfo) override
    {
        if (testInfo.result()->Passed())
        {
            ++ok;
            std::cout << "." << std::flush;
            return;
        }

        ++failed;
        std::cout << "F" << std::flush;

        for (int k = 0; k != testInfo.result()->total_part_count(); ++k)
        {
            errors << "\n" << testInfo.test_case_name() << " " << testInfo.name() << ":\n";

            auto& result = testInfo.result()->GetTestPartResult(k);
            if (result.file_name())
                errors << result.file_name() << ":" << result.line_number() << std::endl;

            errors << result.summary() << std::endl;
        }
    }

    void OnTestProgramEnd(const testing::UnitTest& ) override
    {
        std::cout << errors.str() << std::endl;

        if (ok)
            std::cout << "ok: " << ok;
        if (failed)
            std::cout << " failed: " << failed;

        std::cout << std::endl;
    }

private:
    std::ostringstream errors;
    int ok{}, failed{};
};
