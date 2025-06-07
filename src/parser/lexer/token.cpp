#include <finch/parser/lexer/token.hpp>
#include <fmt/format.h>

namespace finch::lexer {

std::string Token::to_string() const {
    switch (type) {
    case TokenType::Identifier:
        if (auto* str = get_value<std::string>()) {
            return fmt::format("Identifier({})", *str);
        }
        return "Identifier()";

    case TokenType::String:
        if (auto* str = get_value<std::string>()) {
            return fmt::format("String(\"{}\")", *str);
        }
        return "String()";

    case TokenType::Number:
        if (auto* num = get_value<double>()) {
            return fmt::format("Number({})", *num);
        }
        return "Number()";

    case TokenType::Variable:
        if (auto* str = get_value<std::string>()) {
            return fmt::format("Variable(${{{}}})", *str);
        }
        return "Variable()";

    case TokenType::GeneratorExpr:
        if (auto* str = get_value<std::string>()) {
            return fmt::format("GeneratorExpr($<{}>)", *str);
        }
        return "GeneratorExpr()";

    case TokenType::LeftParen:
        return "LeftParen";
    case TokenType::RightParen:
        return "RightParen";
    case TokenType::LeftBracket:
        return "LeftBracket";
    case TokenType::RightBracket:
        return "RightBracket";
    case TokenType::Semicolon:
        return "Semicolon";
    case TokenType::Comment:
        return "Comment";
    case TokenType::BracketComment:
        return "BracketComment";
    case TokenType::Newline:
        return "Newline";
    case TokenType::Whitespace:
        return "Whitespace";
    case TokenType::Eof:
        return "Eof";
    case TokenType::Invalid:
        if (auto* str = get_value<std::string>()) {
            return fmt::format("Invalid({})", *str);
        }
        return "Invalid";
    default:
        return "Unknown";
    }
}

std::string Token::type_name(TokenType type) {
    switch (type) {
    case TokenType::Identifier:
        return "identifier";
    case TokenType::String:
        return "string literal";
    case TokenType::Number:
        return "number";
    case TokenType::Variable:
        return "variable reference";
    case TokenType::GeneratorExpr:
        return "generator expression";
    case TokenType::LeftParen:
        return "left parenthesis";
    case TokenType::RightParen:
        return "right parenthesis";
    case TokenType::LeftBracket:
        return "left bracket";
    case TokenType::RightBracket:
        return "right bracket";
    case TokenType::Semicolon:
        return "semicolon";
    case TokenType::Comment:
        return "comment";
    case TokenType::BracketComment:
        return "bracket comment";
    case TokenType::Newline:
        return "newline";
    case TokenType::Whitespace:
        return "whitespace";
    case TokenType::Eof:
        return "end of file";
    case TokenType::Invalid:
        return "invalid token";
    default:
        return "unknown token";
    }
}

} // namespace finch::lexer
