#pragma once

#include <finch/core/error.hpp>
#include <finch/core/result.hpp>
#include <finch/parser/ast/builder.hpp>
#include <finch/parser/ast/node.hpp>
#include <finch/parser/lexer/lexer.hpp>
#include <memory>
#include <string_view>
#include <vector>

namespace finch::parser {

/// Main CMake parser implementation
class Parser {
  private:
    lexer::Lexer lexer_;
    ast::ASTBuilder builder_;
    std::vector<lexer::Token> token_buffer_;
    size_t current_ = 0;

    // Error recovery state
    bool panic_mode_ = false;
    std::vector<ParseError> errors_;

    // File information
    std::string filename_;

  public:
    /// Constructor with source text
    Parser(std::string_view source, std::string filename);

    /// Constructor with SourceBuffer
    explicit Parser(lexer::SourceBuffer buffer);

    /// Main parsing interface
    [[nodiscard]] Result<std::unique_ptr<ast::File>, std::vector<ParseError>> parse_file();

    /// Get the AST builder (for string interning statistics)
    [[nodiscard]] const ast::ASTBuilder& builder() const {
        return builder_;
    }

  private:
    // Token management
    [[nodiscard]] const lexer::Token& current();
    [[nodiscard]] const lexer::Token& peek(size_t ahead = 0);
    [[nodiscard]] Result<lexer::Token, ParseError> advance();
    [[nodiscard]] bool check(lexer::TokenType type) const;
    [[nodiscard]] bool match(lexer::TokenType type);
    [[nodiscard]] Result<void, ParseError> consume(lexer::TokenType type,
                                                   const std::string& message);
    void skip_trivia(); // Skip whitespace and comments

    // Error handling
    void synchronize();
    [[nodiscard]] ParseError error(const std::string& message);
    void report_error(ParseError err);

    // Statement parsing
    [[nodiscard]] Result<ast::ASTNodeList, ParseError> parse_file_elements();
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_statement();
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_command_invocation();
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_if_statement();
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_foreach_statement();
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_while_statement();
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_function_def();
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_macro_def();

    // Argument parsing
    [[nodiscard]] Result<ast::ASTNodeList, ParseError> parse_arguments();
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_argument();
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_quoted_argument();
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_unquoted_argument();
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_bracket_argument();

    // Expression parsing (for conditions)
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_expression();
    [[nodiscard]] Result<ast::ASTNodePtr, ParseError> parse_primary_expression();

    // Control flow helpers
    [[nodiscard]] Result<ast::ASTNodeList, ParseError>
    parse_block_until(const std::vector<std::string_view>& terminators);
    [[nodiscard]] bool is_at_block_terminator(const std::vector<std::string_view>& terminators);

    // Foreach specific parsing
    [[nodiscard]] Result<ast::ForEachStatement::LoopType, ParseError> parse_foreach_loop_type();
};

} // namespace finch::parser
