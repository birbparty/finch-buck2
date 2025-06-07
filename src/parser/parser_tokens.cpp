#include <finch/core/logging.hpp>
#include <finch/parser/parser.hpp>

namespace finch::parser {

const lexer::Token& Parser::current() {
    // Ensure we have at least one token in the buffer
    while (current_ >= token_buffer_.size()) {
        auto result = lexer_.next_token();
        if (!result.has_value()) {
            // Create error token for error recovery
            lexer::Token error_token{lexer::TokenType::Invalid, std::monostate{},
                                     lexer_.current_location(), ""};
            token_buffer_.push_back(error_token);
            report_error(std::move(result.error()));
        } else {
            token_buffer_.push_back(std::move(result.value()));
        }
    }
    return token_buffer_[current_];
}

const lexer::Token& Parser::peek(size_t ahead) {
    // Ensure we have enough tokens for lookahead
    while (current_ + ahead >= token_buffer_.size()) {
        auto result = lexer_.next_token();
        if (!result.has_value()) {
            // Create error token
            lexer::Token error_token{lexer::TokenType::Invalid, std::monostate{},
                                     lexer_.current_location(), ""};
            token_buffer_.push_back(error_token);
        } else {
            token_buffer_.push_back(std::move(result.value()));
        }
    }
    return token_buffer_[current_ + ahead];
}

Result<lexer::Token, ParseError> Parser::advance() {
    if (!check(lexer::TokenType::Eof)) {
        current_++;
    }
    if (current_ > 0) {
        // Make a copy of the token
        lexer::Token tok = token_buffer_[current_ - 1];
        return Ok<lexer::Token, ParseError>(std::move(tok));
    }
    return Err<ParseError, lexer::Token>(error("No token to advance"));
}

bool Parser::check(lexer::TokenType type) const {
    // Const cast is safe here because current() only modifies the buffer, not observable state
    return const_cast<Parser*>(this)->current().type == type;
}

bool Parser::match(lexer::TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Result<void, ParseError> Parser::consume(lexer::TokenType type, const std::string& message) {
    if (check(type)) {
        advance();
        return Ok<ParseError>();
    }

    return Err<ParseError, void>(error(message));
}

void Parser::skip_trivia() {
    while (true) {
        if (check(lexer::TokenType::Whitespace) || check(lexer::TokenType::Comment) ||
            check(lexer::TokenType::BracketComment) || check(lexer::TokenType::Newline)) {
            advance();
        } else {
            break;
        }
    }
}

} // namespace finch::parser
