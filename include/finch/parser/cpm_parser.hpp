#pragma once

#include <finch/core/error.hpp>
#include <finch/core/result.hpp>
#include <finch/parser/ast/cpm_nodes.hpp>
#include <finch/parser/ast/node.hpp>
#include <finch/parser/parser.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace finch::parser {

/// CPM.cmake command parser
class CPMParser {
  private:
    Parser& parser_;

  public:
    explicit CPMParser(Parser& parser) : parser_(parser) {}

    /// Parse CPM commands
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError>
    parse_cpm_command(const std::string& command_name, const ast::ASTNodeList& args);

  private:
    // CPMAddPackage parsing
    [[nodiscard]] Result<std::unique_ptr<ast::CPMAddPackage>, ParseError>
    parse_cpm_add_package(const ast::ASTNodeList& args);

    [[nodiscard]] Result<std::unique_ptr<ast::CPMAddPackage>, ParseError>
    parse_cpm_add_package_shorthand(const std::string& shorthand);

    [[nodiscard]] Result<std::unique_ptr<ast::CPMAddPackage>, ParseError>
    parse_cpm_add_package_full(const ast::ASTNodeList& args);

    // CPMFindPackage parsing
    [[nodiscard]] Result<std::unique_ptr<ast::CPMFindPackage>, ParseError>
    parse_cpm_find_package(const ast::ASTNodeList& args);

    // CPMUsePackageLock parsing
    [[nodiscard]] Result<std::unique_ptr<ast::CPMUsePackageLock>, ParseError>
    parse_cpm_use_package_lock(const ast::ASTNodeList& args);

    // CPMDeclarePackage parsing
    [[nodiscard]] Result<std::unique_ptr<ast::CPMDeclarePackage>, ParseError>
    parse_cpm_declare_package(const ast::ASTNodeList& args);

    // Helper methods
    [[nodiscard]] Result<ast::CPMVersion, ParseError>
    parse_version_string(const std::string& version_str);

    [[nodiscard]] Result<void, ParseError> parse_options_block(ast::CPMAddPackage& package,
                                                               const ast::ASTNodeList& options);

    [[nodiscard]] bool is_github_shorthand(const std::string& str) const;

    [[nodiscard]] Result<std::pair<std::string, std::string>, ParseError>
    parse_github_shorthand(const std::string& shorthand);

    // Utility to extract string from AST node
    [[nodiscard]] Result<std::string, ParseError> get_string_value(const ast::ASTNode* node) const;

    // Utility to find argument by name
    [[nodiscard]] const ast::ASTNode* find_argument(const ast::ASTNodeList& args,
                                                    const std::string& name) const;

    // Utility to collect arguments after a key
    [[nodiscard]] std::vector<const ast::ASTNode*>
    collect_arguments_after(const ast::ASTNodeList& args, const std::string& key) const;
};

} // namespace finch::parser
