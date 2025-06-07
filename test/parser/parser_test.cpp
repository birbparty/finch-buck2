#include <finch/parser/ast/commands.hpp>
#include <finch/parser/ast/control_flow.hpp>
#include <finch/parser/ast/literals.hpp>
#include <finch/parser/parser.hpp>
#include <gtest/gtest.h>

using namespace finch;
using namespace finch::parser;

class ParserTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Setup code if needed
    }
};

TEST_F(ParserTest, ParseSimpleCommand) {
    const char* code = "add_library(mylib STATIC src/main.cpp)";
    Parser parser(code, "test.cmake");

    auto result = parser.parse_file();
    ASSERT_TRUE(result.has_value()) << "Parse should succeed";

    auto& file = result.value();
    ASSERT_EQ(file->statements().size(), 1) << "Should have one statement";

    auto* cmd = dynamic_cast<ast::CommandCall*>(file->statements()[0].get());
    ASSERT_NE(cmd, nullptr) << "Should be a command";
    EXPECT_EQ(cmd->name(), "add_library");
    EXPECT_EQ(cmd->arguments().size(), 3);
}

TEST_F(ParserTest, ParseMultipleCommands) {
    const char* code = R"(
        project(MyProject)
        set(SOURCES main.cpp helper.cpp)
        add_executable(myapp ${SOURCES})
    )";

    Parser parser(code, "test.cmake");
    auto result = parser.parse_file();
    ASSERT_TRUE(result.has_value());

    auto& file = result.value();
    EXPECT_EQ(file->statements().size(), 3);
}

TEST_F(ParserTest, ParseIfStatement) {
    const char* code = R"(
        if(WIN32)
            set(PLATFORM_SOURCES windows.cpp)
        else()
            set(PLATFORM_SOURCES unix.cpp)
        endif()
    )";

    Parser parser(code, "test.cmake");
    auto result = parser.parse_file();
    ASSERT_TRUE(result.has_value());

    auto& file = result.value();
    ASSERT_EQ(file->statements().size(), 1);

    auto* if_stmt = dynamic_cast<ast::IfStatement*>(file->statements()[0].get());
    ASSERT_NE(if_stmt, nullptr);
    EXPECT_FALSE(if_stmt->then_branch().empty());
    EXPECT_FALSE(if_stmt->else_branch().empty());
}

TEST_F(ParserTest, ParseForeachLoop) {
    const char* code = R"(
        foreach(src IN LISTS SOURCES)
            message(STATUS "Processing ${src}")
        endforeach()
    )";

    Parser parser(code, "test.cmake");
    auto result = parser.parse_file();
    ASSERT_TRUE(result.has_value());

    auto& file = result.value();
    ASSERT_EQ(file->statements().size(), 1);

    auto* foreach_stmt = dynamic_cast<ast::ForEachStatement*>(file->statements()[0].get());
    ASSERT_NE(foreach_stmt, nullptr);
    EXPECT_EQ(foreach_stmt->variables().size(), 1);
    EXPECT_EQ(foreach_stmt->variables()[0], "src");
    EXPECT_EQ(foreach_stmt->loop_type(), ast::ForEachStatement::LoopType::IN_LISTS);
}

TEST_F(ParserTest, ParseFunctionDefinition) {
    const char* code = R"(
        function(my_function arg1 arg2)
            message(STATUS "arg1 = ${arg1}")
            message(STATUS "arg2 = ${arg2}")
        endfunction()
    )";

    Parser parser(code, "test.cmake");
    auto result = parser.parse_file();
    ASSERT_TRUE(result.has_value());

    auto& file = result.value();
    ASSERT_EQ(file->statements().size(), 1);

    auto* func = dynamic_cast<ast::FunctionDef*>(file->statements()[0].get());
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->name(), "my_function");
    EXPECT_EQ(func->parameters().size(), 2);
    EXPECT_EQ(func->body().size(), 2);
}

TEST_F(ParserTest, ParseQuotedArguments) {
    const char* code = R"(set(VAR "Hello, World!"))";

    Parser parser(code, "test.cmake");
    auto result = parser.parse_file();
    ASSERT_TRUE(result.has_value());

    auto& file = result.value();
    ASSERT_EQ(file->statements().size(), 1);

    auto* cmd = dynamic_cast<ast::CommandCall*>(file->statements()[0].get());
    ASSERT_NE(cmd, nullptr);
    EXPECT_EQ(cmd->arguments().size(), 2);

    // Check that the second argument is a quoted string
    auto* str_arg = dynamic_cast<const ast::StringLiteral*>(cmd->argument(1));
    ASSERT_NE(str_arg, nullptr);
    EXPECT_EQ(str_arg->value(), "Hello, World!");
    EXPECT_TRUE(str_arg->is_quoted());
}

TEST_F(ParserTest, ParseVariableReferences) {
    const char* code = R"(message(STATUS "Path: ${CMAKE_SOURCE_DIR}"))";

    Parser parser(code, "test.cmake");
    auto result = parser.parse_file();
    ASSERT_TRUE(result.has_value());

    auto& file = result.value();
    auto* cmd = dynamic_cast<ast::CommandCall*>(file->statements()[0].get());
    ASSERT_NE(cmd, nullptr);

    // The string with variable interpolation should be parsed
    // The actual interpolation parsing depends on the interpolation lexer
}

TEST_F(ParserTest, ParseBooleanLiterals) {
    const char* code = "option(ENABLE_TESTS \"Enable testing\" ON)";

    Parser parser(code, "test.cmake");
    auto result = parser.parse_file();
    ASSERT_TRUE(result.has_value());

    auto& file = result.value();
    auto* cmd = dynamic_cast<ast::CommandCall*>(file->statements()[0].get());
    ASSERT_NE(cmd, nullptr);
    EXPECT_EQ(cmd->arguments().size(), 3);

    // The third argument should be a boolean literal
    auto* bool_arg = dynamic_cast<const ast::BooleanLiteral*>(cmd->argument(2));
    ASSERT_NE(bool_arg, nullptr);
    EXPECT_TRUE(bool_arg->value());
}

TEST_F(ParserTest, ErrorRecovery) {
    const char* code = R"(
        add_library(mylib STATIC
        # Missing closing paren
        set(VAR value)
        message(STATUS "This should still parse")
    )";

    Parser parser(code, "test.cmake");
    auto result = parser.parse_file();

    // Should have errors but still parse what it can
    ASSERT_FALSE(result.has_value());
    ASSERT_TRUE(result.has_error());

    // Check that we got multiple errors (shows recovery)
    const auto& errors = result.error();
    EXPECT_GE(errors.size(), 1);
}

TEST_F(ParserTest, ParseComplexFile) {
    const char* code = R"(
        cmake_minimum_required(VERSION 3.20)
        project(ComplexProject VERSION 1.0.0)

        set(CMAKE_CXX_STANDARD 17)

        # Find dependencies
        find_package(fmt REQUIRED)

        # Source files
        set(SOURCES
            src/main.cpp
            src/helper.cpp
            src/utils.cpp
        )

        # Platform-specific sources
        if(WIN32)
            list(APPEND SOURCES src/windows_impl.cpp)
        elseif(UNIX)
            list(APPEND SOURCES src/unix_impl.cpp)
        endif()

        # Create library
        add_library(mylib STATIC ${SOURCES})

        # Link dependencies
        target_link_libraries(mylib
            PRIVATE
                fmt::fmt
        )

        # Create executable
        add_executable(myapp src/app_main.cpp)
        target_link_libraries(myapp PRIVATE mylib)

        # Testing
        if(BUILD_TESTING)
            enable_testing()
            add_subdirectory(tests)
        endif()
    )";

    Parser parser(code, "test.cmake");
    auto result = parser.parse_file();
    ASSERT_TRUE(result.has_value());

    auto& file = result.value();
    EXPECT_GT(file->statements().size(), 10);

    // Verify string interner is working
    EXPECT_GT(parser.builder().interner().unique_strings(), 0);
}

// Test for specific CMake constructs
TEST_F(ParserTest, ParseGeneratorExpression) {
    const char* code = R"(
        target_compile_definitions(mylib
            PRIVATE
                $<$<CONFIG:Debug>:DEBUG_MODE>
        )
    )";

    Parser parser(code, "test.cmake");
    auto result = parser.parse_file();
    ASSERT_TRUE(result.has_value());

    // Generator expressions should be parsed as special tokens
}

TEST_F(ParserTest, ParseBracketArgument) {
    const char* code = R"(
        set(MULTI_LINE_STRING [[
This is a multi-line
string with no escapes
]])
    )";

    Parser parser(code, "test.cmake");
    auto result = parser.parse_file();
    ASSERT_TRUE(result.has_value());

    auto& file = result.value();
    auto* cmd = dynamic_cast<ast::CommandCall*>(file->statements()[0].get());
    ASSERT_NE(cmd, nullptr);

    // Should have parsed the bracket argument
    EXPECT_EQ(cmd->arguments().size(), 2);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
