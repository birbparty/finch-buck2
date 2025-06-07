#pragma once

#include "source_buffer.hpp"
#include "token.hpp"
#include <finch/core/result.hpp>
#include <string_view>
#include <vector>

namespace finch::lexer {

/// Main CMake lexer implementation
class Lexer {
  private:
    SourceBuffer buffer_;
    size_t position_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;

    // Token buffer for lookahead
    std::vector<Token> token_buffer_;
    size_t buffer_pos_ = 0;

  public:
    /// Constructor with source text
    Lexer(std::string_view source, std::string filename);

    /// Constructor with SourceBuffer
    explicit Lexer(SourceBuffer buffer);

    /// Get next token
    [[nodiscard]] Result<Token, ParseError> next_token();

    /// Peek at token without consuming (0 = current, 1 = next, etc.)
    [[nodiscard]] Result<Token, ParseError> peek_token(size_t ahead = 0);

    /// Skip whitespace but not newlines
    void skip_whitespace();

    /// Skip whitespace and optionally newlines
    void skip_whitespace(bool skip_newlines);

    /// Get current source location
    [[nodiscard]] SourceLocation current_location() const;

    /// Check if at end of input
    [[nodiscard]] bool at_end() const noexcept {
        return position_ >= buffer_.size();
    }

    /// Get the source buffer
    [[nodiscard]] const SourceBuffer& buffer() const noexcept {
        return buffer_;
    }

  private:
    // Character operations
    [[nodiscard]] char current() const {
        return buffer_.at(position_);
    }

    [[nodiscard]] char peek(size_t ahead = 1) const {
        return buffer_.at(position_ + ahead);
    }

    char advance();

    // Lexing methods
    [[nodiscard]] Result<Token, ParseError> lex_identifier();
    [[nodiscard]] Result<Token, ParseError> lex_string();
    [[nodiscard]] Result<Token, ParseError> lex_variable();
    [[nodiscard]] Result<Token, ParseError> lex_generator_expr();
    [[nodiscard]] Result<Token, ParseError> lex_number();
    [[nodiscard]] Result<Token, ParseError> lex_bracket_comment();
    [[nodiscard]] Result<Token, ParseError> lex_unquoted_argument();
    [[nodiscard]] Result<Token, ParseError> lex_bracket_argument();

    // Helper methods
    [[nodiscard]] bool is_identifier_start(char ch) const;
    [[nodiscard]] bool is_identifier_cont(char ch) const;
    [[nodiscard]] bool is_unquoted_elem(char ch) const;
    void skip_line_comment();
    void handle_line_continuation();

    // Create token at current position
    [[nodiscard]] Token make_token(TokenType type, size_t start_pos);
    [[nodiscard]] Token make_token(TokenType type, size_t start_pos, TokenValue value);
    [[nodiscard]] Token make_error_token(const std::string& message, size_t start_pos);
};

} // namespace finch::lexer
