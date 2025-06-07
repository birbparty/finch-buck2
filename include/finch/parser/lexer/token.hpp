#pragma once

#include <finch/core/error.hpp>
#include <string>
#include <string_view>
#include <variant>

namespace finch::lexer {

/// Token types for CMake lexical analysis
enum class TokenType {
    // Literals
    Identifier,    // command names, unquoted args
    String,        // quoted strings
    Number,        // numeric literals
    Variable,      // ${VAR} or $ENV{VAR}
    GeneratorExpr, // $<...>

    // Punctuation
    LeftParen,    // (
    RightParen,   // )
    LeftBracket,  // [
    RightBracket, // ]
    Semicolon,    // ;

    // Comments
    Comment,        // # comment
    BracketComment, // #[[...]]

    // Control
    Newline,    // line ending
    Whitespace, // spaces/tabs
    Eof,        // end of file

    // Error
    Invalid // invalid token
};

/// Token value types
using TokenValue = std::variant<std::monostate, // no value
                                std::string,    // string value
                                double,         // numeric value
                                char            // single char
                                >;

/// Token structure
struct Token {
    TokenType type;
    TokenValue value;
    SourceLocation location;
    std::string_view text; // Original text from source

    /// Check if token is of specific type
    [[nodiscard]] bool is(TokenType t) const noexcept {
        return type == t;
    }

    /// Check if token is not of specific type
    [[nodiscard]] bool is_not(TokenType t) const noexcept {
        return type != t;
    }

    /// Get value as specific type
    template <typename T>
    [[nodiscard]] const T* get_value() const {
        return std::get_if<T>(&value);
    }

    /// Convert token to string representation
    [[nodiscard]] std::string to_string() const;

    /// Get human-readable token type name
    [[nodiscard]] static std::string type_name(TokenType type);

    /// Check if token represents an error
    [[nodiscard]] bool is_error() const noexcept {
        return type == TokenType::Invalid;
    }

    /// Check if token is skippable whitespace/comment
    [[nodiscard]] bool is_trivia() const noexcept {
        return type == TokenType::Whitespace || type == TokenType::Comment ||
               type == TokenType::BracketComment;
    }
};

/// String part for interpolated strings
struct StringPart {
    enum Type { Literal, Variable };
    Type type;
    std::string value;
    SourceLocation location;
};

} // namespace finch::lexer
