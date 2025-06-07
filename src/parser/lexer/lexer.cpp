#include <cctype>
#include <charconv>
#include <finch/core/logging.hpp>
#include <finch/parser/lexer/lexer.hpp>

using finch::ParseError;

namespace finch::lexer {

Lexer::Lexer(std::string_view source, std::string filename)
    : buffer_(std::string(source), std::move(filename)) {}

Lexer::Lexer(SourceBuffer buffer) : buffer_(std::move(buffer)) {}

SourceLocation Lexer::current_location() const {
    return buffer_.location_at(position_);
}

char Lexer::advance() {
    if (at_end()) {
        return '\0';
    }

    char ch = current();
    ++position_;

    if (ch == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }

    return ch;
}

void Lexer::skip_whitespace() {
    skip_whitespace(false);
}

void Lexer::skip_whitespace(bool skip_newlines) {
    while (!at_end()) {
        char ch = current();

        // Handle line continuation
        if (ch == '\\' && peek() == '\n') {
            advance(); // Skip backslash
            advance(); // Skip newline
            continue;
        }

        if (ch == ' ' || ch == '\t' || ch == '\r') {
            advance();
        } else if (ch == '\n' && skip_newlines) {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skip_line_comment() {
    // Skip # character
    advance();

    // Skip until end of line
    while (!at_end() && current() != '\n') {
        advance();
    }
}

void Lexer::handle_line_continuation() {
    while (current() == '\\' && peek() == '\n') {
        advance(); // Skip backslash
        advance(); // Skip newline
    }
}

Token Lexer::make_token(TokenType type, size_t start_pos) {
    return Token{type, std::monostate{}, buffer_.location_at(start_pos),
                 buffer_.slice(start_pos, position_)};
}

Token Lexer::make_token(TokenType type, size_t start_pos, TokenValue value) {
    return Token{type, std::move(value), buffer_.location_at(start_pos),
                 buffer_.slice(start_pos, position_)};
}

Token Lexer::make_error_token(const std::string& message, size_t start_pos) {
    return Token{TokenType::Invalid, message, buffer_.location_at(start_pos),
                 buffer_.slice(start_pos, position_)};
}

bool Lexer::is_identifier_start(char ch) const {
    return std::isalpha(ch) || ch == '_';
}

bool Lexer::is_identifier_cont(char ch) const {
    return std::isalnum(ch) || ch == '_' || ch == '-';
}

bool Lexer::is_unquoted_elem(char ch) const {
    // CMake unquoted arguments can contain most characters except:
    // whitespace, (), #, ", \, and sometimes ;
    return ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n' && ch != '(' && ch != ')' &&
           ch != '#' && ch != '"' && ch != '\\' && ch != '\0';
}

Result<Token, ParseError> Lexer::next_token() {
    // Skip whitespace but not newlines (they can be significant)
    skip_whitespace(false);

    if (at_end()) {
        return Ok<Token, ParseError>(
            Token{TokenType::Eof, std::monostate{}, current_location(), ""});
    }

    auto start_pos = position_;
    auto start_loc = current_location();
    char ch = current();

    // Single character tokens
    switch (ch) {
    case '(':
        advance();
        return Ok<Token, ParseError>(make_token(TokenType::LeftParen, start_pos));

    case ')':
        advance();
        return Ok<Token, ParseError>(make_token(TokenType::RightParen, start_pos));

    case '[':
        // Check for bracket argument [[ or bracket comment #[[
        if (position_ > 0 && buffer_.at(position_ - 1) == '#' && peek() == '[') {
            return lex_bracket_comment();
        } else if (peek() == '=' || peek() == '[') {
            return lex_bracket_argument();
        }
        advance();
        return Ok<Token, ParseError>(make_token(TokenType::LeftBracket, start_pos));

    case ']':
        advance();
        return Ok<Token, ParseError>(make_token(TokenType::RightBracket, start_pos));

    case ';':
        advance();
        return Ok<Token, ParseError>(make_token(TokenType::Semicolon, start_pos));

    case '\n':
        advance();
        return Ok<Token, ParseError>(make_token(TokenType::Newline, start_pos));

    case '#':
        // Line comment
        skip_line_comment();
        return next_token(); // Recursive call to get next real token

    case '"':
        return lex_string();

    case '$':
        if (peek() == '{') {
            return lex_variable();
        } else if (peek() == '<') {
            return lex_generator_expr();
        }
        // Fall through to unquoted argument
        break;

    case ' ':
    case '\t':
    case '\r':
        // Whitespace token
        while (!at_end() && (current() == ' ' || current() == '\t' || current() == '\r')) {
            advance();
        }
        return Ok<Token, ParseError>(make_token(TokenType::Whitespace, start_pos));
    }

    // Numbers
    if (std::isdigit(ch) || (ch == '.' && std::isdigit(peek()))) {
        return lex_number();
    }

    // Identifiers or unquoted arguments
    if (is_identifier_start(ch) || is_unquoted_elem(ch)) {
        return lex_unquoted_argument();
    }

    // Invalid character
    advance();
    ParseError error(fmt::format("Unexpected character: '{}'", ch));
    error.at(start_loc);
    return Result<Token, ParseError>(std::in_place_index<1>, std::move(error));
}

Result<Token, ParseError> Lexer::lex_string() {
    auto start_loc = current_location();
    auto start_pos = position_;

    advance(); // Skip opening quote

    std::string value;
    bool escaped = false;

    while (!at_end() && (escaped || current() != '"')) {
        if (escaped) {
            // Handle escape sequences
            switch (current()) {
            case 'n':
                value += '\n';
                break;
            case 't':
                value += '\t';
                break;
            case 'r':
                value += '\r';
                break;
            case '\\':
                value += '\\';
                break;
            case '"':
                value += '"';
                break;
            case '$':
                value += '$';
                break;
            case ';':
                value += ';';
                break;
            default:
                // Unknown escape - keep both characters
                value += '\\';
                value += current();
            }
            escaped = false;
        } else {
            if (current() == '\\') {
                escaped = true;
            } else {
                value += current();
            }
        }
        advance();
    }

    if (at_end()) {
        ParseError error(ParseError::Category::UnterminatedString, "Unterminated string");
        error.at(start_loc);
        return Result<Token, ParseError>(std::in_place_index<1>, std::move(error));
    }

    advance(); // Skip closing quote

    return Ok<Token, ParseError>(make_token(TokenType::String, start_pos, std::move(value)));
}

Result<Token, ParseError> Lexer::lex_variable() {
    auto start_loc = current_location();
    auto start_pos = position_;

    advance(); // Skip $
    advance(); // Skip {

    bool is_env = false;
    bool is_cache = false;

    // Check for ENV{ or CACHE{
    if (buffer_.slice(position_, position_ + 4) == "ENV{") {
        is_env = true;
        position_ += 4;
        column_ += 4;
    } else if (buffer_.slice(position_, position_ + 6) == "CACHE{") {
        is_cache = true;
        position_ += 6;
        column_ += 6;
    }

    std::string name;
    int brace_depth = 1;

    while (!at_end() && brace_depth > 0) {
        if (current() == '{') {
            ++brace_depth;
        } else if (current() == '}') {
            --brace_depth;
            if (brace_depth == 0)
                break;
        }
        name += current();
        advance();
    }

    if (at_end()) {
        ParseError error("Unterminated variable reference");
        error.at(start_loc);
        return Result<Token, ParseError>(std::in_place_index<1>, std::move(error));
    }

    advance(); // Skip closing }

    // Format the variable name with prefix if needed
    if (is_env) {
        name = "ENV{" + name + "}";
    } else if (is_cache) {
        name = "CACHE{" + name + "}";
    }

    return Ok<Token, ParseError>(make_token(TokenType::Variable, start_pos, std::move(name)));
}

Result<Token, ParseError> Lexer::lex_generator_expr() {
    auto start_loc = current_location();
    auto start_pos = position_;

    advance(); // Skip $
    advance(); // Skip <

    std::string expr;
    int angle_depth = 1;

    while (!at_end() && angle_depth > 0) {
        if (current() == '<') {
            ++angle_depth;
        } else if (current() == '>') {
            --angle_depth;
            if (angle_depth == 0)
                break;
        }
        expr += current();
        advance();
    }

    if (at_end()) {
        ParseError error("Unterminated generator expression");
        error.at(start_loc);
        return Result<Token, ParseError>(std::in_place_index<1>, std::move(error));
    }

    advance(); // Skip closing >

    return Ok<Token, ParseError>(make_token(TokenType::GeneratorExpr, start_pos, std::move(expr)));
}

Result<Token, ParseError> Lexer::lex_number() {
    auto start_pos = position_;

    // Collect digits before decimal
    while (!at_end() && std::isdigit(current())) {
        advance();
    }

    // Check for decimal point
    if (current() == '.' && std::isdigit(peek())) {
        advance(); // Skip decimal
        while (!at_end() && std::isdigit(current())) {
            advance();
        }
    }

    // Check for exponent
    if ((current() == 'e' || current() == 'E') &&
        (std::isdigit(peek()) || ((peek() == '+' || peek() == '-') && std::isdigit(peek(2))))) {
        advance(); // Skip e/E
        if (current() == '+' || current() == '-') {
            advance(); // Skip sign
        }
        while (!at_end() && std::isdigit(current())) {
            advance();
        }
    }

    // Parse the number
    std::string_view num_str = buffer_.slice(start_pos, position_);
    double value = 0.0;

    // Use stod for now since from_chars for float is not available on all platforms
    try {
        std::string num_string(num_str);
        value = std::stod(num_string);
    } catch (...) {
        ParseError error("Invalid number format");
        error.at(buffer_.location_at(start_pos));
        return Result<Token, ParseError>(std::in_place_index<1>, std::move(error));
    }

    return Ok<Token, ParseError>(make_token(TokenType::Number, start_pos, value));
}

Result<Token, ParseError> Lexer::lex_unquoted_argument() {
    auto start_pos = position_;
    std::string value;

    while (!at_end()) {
        char ch = current();

        // Stop at whitespace or special characters
        if (std::isspace(ch) || ch == '(' || ch == ')' || ch == '#') {
            break;
        }

        // Handle escapes in unquoted arguments
        if (ch == '\\') {
            char next = peek();
            if (next == ';' || next == ' ' || next == '(' || next == ')' || next == '$' ||
                next == '@' || next == '\\' || next == '#') {
                advance(); // Skip backslash
                value += current();
            } else if (next == '\n') {
                // Line continuation
                advance(); // Skip backslash
                advance(); // Skip newline
                continue;
            } else {
                value += ch;
            }
        } else {
            value += ch;
        }

        advance();
    }

    if (value.empty()) {
        ParseError error("Expected argument");
        error.at(buffer_.location_at(start_pos));
        return Result<Token, ParseError>(std::in_place_index<1>, std::move(error));
    }

    return Ok<Token, ParseError>(make_token(TokenType::Identifier, start_pos, std::move(value)));
}

Result<Token, ParseError> Lexer::lex_bracket_comment() {
    auto start_loc = current_location();
    auto start_pos = position_ - 1; // Include the # in the token

    // We're at the first [ after #
    advance(); // Skip first [
    advance(); // Skip second [

    // Count the number of = signs
    int equals_count = 0;
    while (current() == '=') {
        ++equals_count;
        advance();
    }

    // Now we should see [
    if (current() != '[') {
        ParseError error("Invalid bracket comment syntax");
        error.at(start_loc);
        return Result<Token, ParseError>(std::in_place_index<1>, std::move(error));
    }
    advance();

    // Find closing ]=...=]]
    std::string closing = "]";
    for (int i = 0; i < equals_count; ++i) {
        closing += '=';
    }
    closing += "]]";

    while (!at_end()) {
        if (buffer_.slice(position_, position_ + closing.length()) == closing) {
            position_ += closing.length();
            column_ += closing.length();
            break;
        }
        advance();
    }

    if (at_end()) {
        ParseError error("Unterminated bracket comment");
        error.at(start_loc);
        return Result<Token, ParseError>(std::in_place_index<1>, std::move(error));
    }

    return Ok<Token, ParseError>(make_token(TokenType::BracketComment, start_pos));
}

Result<Token, ParseError> Lexer::lex_bracket_argument() {
    auto start_loc = current_location();
    auto start_pos = position_;

    advance(); // Skip [

    // Count the number of = signs
    int equals_count = 0;
    while (current() == '=') {
        ++equals_count;
        advance();
    }

    // Now we should see [
    if (current() != '[') {
        ParseError error("Invalid bracket argument syntax");
        error.at(start_loc);
        return Result<Token, ParseError>(std::in_place_index<1>, std::move(error));
    }
    advance();

    // Find closing ]=...=]
    std::string closing = "]";
    for (int i = 0; i < equals_count; ++i) {
        closing += '=';
    }
    closing += "]";

    std::string content;
    while (!at_end()) {
        if (buffer_.slice(position_, position_ + closing.length()) == closing) {
            position_ += closing.length();
            column_ += closing.length();
            break;
        }
        content += current();
        advance();
    }

    if (at_end()) {
        ParseError error("Unterminated bracket argument");
        error.at(start_loc);
        return Result<Token, ParseError>(std::in_place_index<1>, std::move(error));
    }

    return Ok<Token, ParseError>(make_token(TokenType::String, start_pos, std::move(content)));
}

Result<Token, ParseError> Lexer::peek_token(size_t ahead) {
    // Ensure we have enough tokens in the buffer
    while (token_buffer_.size() <= buffer_pos_ + ahead) {
        // Save current state
        size_t saved_pos = position_;
        size_t saved_line = line_;
        size_t saved_col = column_;

        // Lex next token
        auto result = next_token();

        // Restore state
        position_ = saved_pos;
        line_ = saved_line;
        column_ = saved_col;

        if (result.has_error()) {
            return result;
        }

        token_buffer_.push_back(std::move(result.value()));
    }

    return Result<Token, ParseError>(token_buffer_[buffer_pos_ + ahead]);
}

} // namespace finch::lexer
