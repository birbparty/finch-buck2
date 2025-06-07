// dummy_test.cpp - Temporary test file for finch-tests
// This file exists to allow the CMake build to succeed.
// It will be replaced with actual test files.

#include <gtest/gtest.h>

namespace finch {

// Simple test to verify GoogleTest is working
TEST(DummyTest, BasicAssertion) {
    EXPECT_EQ(1, 1);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

TEST(DummyTest, StringComparison) {
    std::string expected = "finch";
    std::string actual = "finch";
    EXPECT_EQ(expected, actual);
}

} // namespace finch
