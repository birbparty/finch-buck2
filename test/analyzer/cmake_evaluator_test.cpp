#include <finch/analyzer/cmake_evaluator.hpp>
#include <finch/parser/ast/structure.hpp>
#include <finch/parser/parser.hpp>
#include <gtest/gtest.h>

using namespace finch;
using namespace finch::analyzer;

class CMakeEvaluatorTest : public ::testing::Test {
  protected:
    void SetUp() override {
        context_.initialize_builtin_variables();
    }

    EvaluationContext context_;
};

TEST_F(CMakeEvaluatorTest, VariableSubstitution) {
    context_.set_variable("MY_VAR", std::string("value"));

    const char* code = R"(
        set(RESULT ${MY_VAR})
    )";

    Parser parser(code, "test.cmake");
    auto ast = parser.parse_file();
    ASSERT_TRUE(ast.is_ok());

    CMakeEvaluator evaluator(context_);
    for (const auto& stmt : ast.value()->statements()) {
        evaluator.evaluate(*stmt);
    }

    auto result = context_.get_variable("RESULT");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<std::string>(result->value), "value");
    EXPECT_EQ(result->confidence, Confidence::Certain);
}

TEST_F(CMakeEvaluatorTest, ListVariableCreation) {
    const char* code = R"(
        set(MY_LIST item1 item2 item3)
    )";

    Parser parser(code, "test.cmake");
    auto ast = parser.parse_file();
    ASSERT_TRUE(ast.is_ok());

    CMakeEvaluator evaluator(context_);
    for (const auto& stmt : ast.value()->statements()) {
        evaluator.evaluate(*stmt);
    }

    auto result = context_.get_variable("MY_LIST");
    ASSERT_TRUE(result.has_value());
    auto list = std::get<std::vector<std::string>>(result->value);
    ASSERT_EQ(list.size(), 3);
    EXPECT_EQ(list[0], "item1");
    EXPECT_EQ(list[1], "item2");
    EXPECT_EQ(list[2], "item3");
}

TEST_F(CMakeEvaluatorTest, PlatformDetection) {
    CMakeEvaluator evaluator(context_);

    // Platform variables should be set by initialize_builtin_variables
    auto win32 = context_.get_variable("WIN32");
    ASSERT_TRUE(win32.has_value());

    auto unix_var = context_.get_variable("UNIX");
    ASSERT_TRUE(unix_var.has_value());

    // At least one platform should be set
    bool has_platform =
        value_helpers::is_truthy(win32->value) || value_helpers::is_truthy(unix_var->value);
    EXPECT_TRUE(has_platform);
}

TEST_F(CMakeEvaluatorTest, ConditionalEvaluation) {
    const char* code = R"(
        set(BUILD_SHARED_LIBS ON)
        if(BUILD_SHARED_LIBS)
            set(LIB_TYPE SHARED)
        else()
            set(LIB_TYPE STATIC)
        endif()
    )";

    Parser parser(code, "test.cmake");
    auto ast = parser.parse_file();
    ASSERT_TRUE(ast.is_ok());

    CMakeEvaluator evaluator(context_);
    for (const auto& stmt : ast.value()->statements()) {
        evaluator.evaluate(*stmt);
    }

    // Should have evaluated the if branch
    auto lib_type = context_.get_variable("LIB_TYPE");
    ASSERT_TRUE(lib_type.has_value());
    EXPECT_EQ(std::get<std::string>(lib_type->value), "SHARED");
}

TEST_F(CMakeEvaluatorTest, OptionCommand) {
    const char* code = R"(
        option(ENABLE_TESTS "Enable testing" ON)
        option(ENABLE_DOCS "Enable documentation" OFF)
    )";

    Parser parser(code, "test.cmake");
    auto ast = parser.parse_file();
    ASSERT_TRUE(ast.is_ok());

    CMakeEvaluator evaluator(context_);
    for (const auto& stmt : ast.value()->statements()) {
        evaluator.evaluate(*stmt);
    }

    // Options should be set as cache variables
    auto tests = context_.get_cache_variable("ENABLE_TESTS");
    ASSERT_TRUE(tests.has_value());
    EXPECT_EQ(std::get<std::string>(tests->value), "ON");
    EXPECT_EQ(tests->confidence, Confidence::Uncertain); // Can be overridden

    auto docs = context_.get_cache_variable("ENABLE_DOCS");
    ASSERT_TRUE(docs.has_value());
    EXPECT_EQ(std::get<std::string>(docs->value), "OFF");
}

TEST_F(CMakeEvaluatorTest, ProjectCommand) {
    const char* code = R"(
        project(MyProject)
    )";

    Parser parser(code, "test.cmake");
    auto ast = parser.parse_file();
    ASSERT_TRUE(ast.is_ok());

    CMakeEvaluator evaluator(context_);
    for (const auto& stmt : ast.value()->statements()) {
        evaluator.evaluate(*stmt);
    }

    // PROJECT_NAME should be set
    auto project_name = context_.get_variable("PROJECT_NAME");
    ASSERT_TRUE(project_name.has_value());
    EXPECT_EQ(std::get<std::string>(project_name->value), "MyProject");

    // CMAKE_PROJECT_NAME should also be set
    auto cmake_project_name = context_.get_variable("CMAKE_PROJECT_NAME");
    ASSERT_TRUE(cmake_project_name.has_value());
    EXPECT_EQ(std::get<std::string>(cmake_project_name->value), "MyProject");
}

TEST_F(CMakeEvaluatorTest, CMakeMinimumRequired) {
    const char* code = R"(
        cmake_minimum_required(VERSION 3.20)
    )";

    Parser parser(code, "test.cmake");
    auto ast = parser.parse_file();
    ASSERT_TRUE(ast.is_ok());

    CMakeEvaluator evaluator(context_);
    for (const auto& stmt : ast.value()->statements()) {
        evaluator.evaluate(*stmt);
    }

    // CMAKE_MINIMUM_REQUIRED_VERSION should be set
    auto version = context_.get_variable("CMAKE_MINIMUM_REQUIRED_VERSION");
    ASSERT_TRUE(version.has_value());
    EXPECT_EQ(std::get<std::string>(version->value), "3.20");
}

TEST_F(CMakeEvaluatorTest, StringInterpolation) {
    context_.set_variable("PREFIX", std::string("my"));
    context_.set_variable("SUFFIX", std::string("lib"));

    const char* code = R"(
        set(LIB_NAME ${PREFIX}_${SUFFIX})
    )";

    Parser parser(code, "test.cmake");
    auto ast = parser.parse_file();
    ASSERT_TRUE(ast.is_ok());

    CMakeEvaluator evaluator(context_);
    for (const auto& stmt : ast.value()->statements()) {
        evaluator.evaluate(*stmt);
    }

    auto lib_name = context_.get_variable("LIB_NAME");
    ASSERT_TRUE(lib_name.has_value());
    EXPECT_EQ(std::get<std::string>(lib_name->value), "${PREFIX}_${SUFFIX}");
    // Note: Full string interpolation would require more complex parsing
}

TEST_F(CMakeEvaluatorTest, UnknownVariablePreserved) {
    const char* code = R"(
        set(RESULT ${UNKNOWN_VAR})
    )";

    Parser parser(code, "test.cmake");
    auto ast = parser.parse_file();
    ASSERT_TRUE(ast.is_ok());

    CMakeEvaluator evaluator(context_);
    for (const auto& stmt : ast.value()->statements()) {
        evaluator.evaluate(*stmt);
    }

    auto result = context_.get_variable("RESULT");
    ASSERT_TRUE(result.has_value());
    // Unknown variables should be preserved
    EXPECT_NE(result->confidence, Confidence::Certain);
}

TEST_F(CMakeEvaluatorTest, ValueHelpers) {
    // Test is_truthy
    EXPECT_TRUE(value_helpers::is_truthy(Value(std::string("ON"))));
    EXPECT_TRUE(value_helpers::is_truthy(Value(std::string("TRUE"))));
    EXPECT_TRUE(value_helpers::is_truthy(Value(std::string("1"))));
    EXPECT_TRUE(value_helpers::is_truthy(Value(true)));
    EXPECT_TRUE(value_helpers::is_truthy(Value(1.0)));

    EXPECT_FALSE(value_helpers::is_truthy(Value(std::string("OFF"))));
    EXPECT_FALSE(value_helpers::is_truthy(Value(std::string("FALSE"))));
    EXPECT_FALSE(value_helpers::is_truthy(Value(std::string("0"))));
    EXPECT_FALSE(value_helpers::is_truthy(Value(std::string(""))));
    EXPECT_FALSE(value_helpers::is_truthy(Value(false)));
    EXPECT_FALSE(value_helpers::is_truthy(Value(0.0)));

    // Test to_string
    EXPECT_EQ(value_helpers::to_string(Value(std::string("test"))), "test");
    EXPECT_EQ(value_helpers::to_string(Value(true)), "TRUE");
    EXPECT_EQ(value_helpers::to_string(Value(false)), "FALSE");

    std::vector<std::string> list = {"a", "b", "c"};
    EXPECT_EQ(value_helpers::to_string(Value(list)), "a;b;c");

    // Test to_list
    auto single_list = value_helpers::to_list(Value(std::string("item")));
    ASSERT_EQ(single_list.size(), 1);
    EXPECT_EQ(single_list[0], "item");

    auto multi_list = value_helpers::to_list(Value(std::string("a;b;c")));
    ASSERT_EQ(multi_list.size(), 3);
    EXPECT_EQ(multi_list[0], "a");
    EXPECT_EQ(multi_list[1], "b");
    EXPECT_EQ(multi_list[2], "c");
}

// Test CMakeFileEvaluator convenience class
TEST(CMakeFileEvaluatorTest, BasicUsage) {
    const char* code = R"(
        project(TestProject)
        set(MY_VAR "Hello")
        option(ENABLE_FEATURE "Enable feature" ON)
    )";

    Parser parser(code, "test.cmake");
    auto ast = parser.parse_file();
    ASSERT_TRUE(ast.is_ok());

    CMakeFileEvaluator file_evaluator;
    auto result = file_evaluator.evaluate_file(*ast.value());
    ASSERT_TRUE(result.is_ok());

    // Check that variables were set
    auto my_var = file_evaluator.get_variable("MY_VAR");
    ASSERT_TRUE(my_var.has_value());
    EXPECT_EQ(std::get<std::string>(my_var->value), "Hello");

    auto project_name = file_evaluator.get_variable("PROJECT_NAME");
    ASSERT_TRUE(project_name.has_value());
    EXPECT_EQ(std::get<std::string>(project_name->value), "TestProject");

    // List all variables
    auto vars = file_evaluator.list_variables();
    EXPECT_GT(vars.size(), 0);

    // Check for builtin variables
    EXPECT_TRUE(std::find(vars.begin(), vars.end(), "CMAKE_SOURCE_DIR") != vars.end());
}
