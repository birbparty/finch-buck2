#include <finch/parser/ast/commands.hpp>
#include <finch/parser/ast/control_flow.hpp>
#include <finch/parser/ast/structure.hpp>
#include <finch/parser/parser.hpp>
#include <iostream>

using namespace finch;
using namespace finch::parser;

void print_ast_node(const ast::ASTNode* node, int indent = 0) {
    if (!node)
        return;

    std::string spaces(indent * 2, ' ');

    if (auto* cmd = dynamic_cast<const ast::CommandCall*>(node)) {
        std::cout << spaces << "Command: " << cmd->name() << " (args: " << cmd->arguments().size()
                  << ")\n";
        for (size_t i = 0; i < cmd->arguments().size(); ++i) {
            std::cout << spaces << "  Arg " << i << ":\n";
            print_ast_node(cmd->argument(i), indent + 2);
        }
    } else if (auto* str = dynamic_cast<const ast::StringLiteral*>(node)) {
        std::cout << spaces << "String: \"" << str->value()
                  << "\" (quoted: " << (str->is_quoted() ? "yes" : "no") << ")\n";
    } else if (auto* num = dynamic_cast<const ast::NumberLiteral*>(node)) {
        std::cout << spaces << "Number: " << num->text() << "\n";
    } else if (auto* var = dynamic_cast<const ast::Variable*>(node)) {
        std::cout << spaces << "Variable: ${" << var->name() << "}\n";
    } else if (auto* if_stmt = dynamic_cast<const ast::IfStatement*>(node)) {
        std::cout << spaces << "If statement:\n";
        std::cout << spaces << "  Condition:\n";
        print_ast_node(if_stmt->condition().get(), indent + 2);
        std::cout << spaces << "  Then branch (" << if_stmt->then_branch().size()
                  << " statements):\n";
        for (const auto& stmt : if_stmt->then_branch()) {
            print_ast_node(stmt.get(), indent + 2);
        }
        if (!if_stmt->else_branch().empty()) {
            std::cout << spaces << "  Else branch (" << if_stmt->else_branch().size()
                      << " statements):\n";
            for (const auto& stmt : if_stmt->else_branch()) {
                print_ast_node(stmt.get(), indent + 2);
            }
        }
    } else {
        std::cout << spaces << "Unknown node type\n";
    }
}

void print_parse_result(const std::string& name, const std::string& code) {
    std::cout << "\n=== Parsing: " << name << " ===\n";
    std::cout << "Code:\n" << code << "\n\n";

    Parser parser(code, name + ".cmake");
    auto result = parser.parse_file();

    if (result.has_value()) {
        std::cout << "Parse successful!\n";
        auto& file = result.value();
        std::cout << "File: " << std::string(file->path()) << "\n";
        std::cout << "Statements: " << file->statements().size() << "\n\n";

        for (size_t i = 0; i < file->statements().size(); ++i) {
            std::cout << "Statement " << i << ":\n";
            print_ast_node(file->statements()[i].get(), 1);
        }

        std::cout << "\nString interner stats: " << parser.builder().interner().unique_strings()
                  << " unique strings\n";
    } else {
        std::cout << "Parse failed with " << result.error().size() << " errors:\n";
        for (const auto& error : result.error()) {
            std::cout << "  - " << error.message();
            if (error.location().has_value()) {
                std::cout << " at line " << error.location()->line;
            }
            std::cout << "\n";
        }
    }
}

int main() {
    std::cout << "CMake Parser Example\n";
    std::cout << "===================\n";

    // Example 1: Simple command
    print_parse_result("simple_command", "add_library(mylib STATIC src/main.cpp src/helper.cpp)");

    // Example 2: Multiple commands
    print_parse_result("multiple_commands", R"(
        project(MyProject VERSION 1.0.0)
        set(CMAKE_CXX_STANDARD 17)
        add_executable(myapp main.cpp)
    )");

    // Example 3: If statement
    print_parse_result("conditional", R"(
        if(WIN32)
            set(PLATFORM_SOURCES windows.cpp)
        else()
            set(PLATFORM_SOURCES unix.cpp)
        endif()
    )");

    // Example 4: Function definition
    print_parse_result("function_def", R"(
        function(my_helper arg1 arg2)
            message(STATUS "Processing ${arg1} and ${arg2}")
            set(RESULT "${arg1}_${arg2}" PARENT_SCOPE)
        endfunction()
    )");

    // Example 5: Error recovery
    print_parse_result("error_recovery", R"(
        add_library(broken STATIC
        # Missing closing paren - should show error recovery
        set(VAR "This should still parse")
        message(STATUS "Recovery works!")
    )");

    return 0;
}
