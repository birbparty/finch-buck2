#include <finch/core/logging.hpp>
#include <finch/parser/parser.hpp>

namespace finch::parser {

void Parser::synchronize() {
    panic_mode_ = false;

    // Skip tokens until we find a likely statement boundary
    while (!check(lexer::TokenType::Eof)) {
        // Newline often indicates statement boundary in CMake
        if (current_ > 0 && token_buffer_[current_ - 1].type == lexer::TokenType::Newline) {
            return;
        }

        // Common command starts that indicate a new statement
        if (check(lexer::TokenType::Identifier)) {
            auto name_value = current().get_value<std::string>();
            if (name_value) {
                const auto& name = *name_value;
                // Common CMake commands that start statements
                if (name == "if" || name == "foreach" || name == "while" || name == "function" ||
                    name == "macro" || name == "set" || name == "add_library" ||
                    name == "add_executable" || name == "target_" || name == "find_" ||
                    name == "include" || name == "project" || name == "cmake_minimum_required") {
                    return;
                }
            }
        }

        advance();
    }
}

ParseError Parser::error(const std::string& message) {
    panic_mode_ = true;

    // Create error with current location
    ParseError err(ParseError::Category::InvalidSyntax, message);
    err.at(current().location);

    // Add context about what we were parsing
    if (current().type == lexer::TokenType::Identifier) {
        auto value = current().get_value<std::string>();
        if (value) {
            err.with_context(fmt::format("near identifier '{}'", *value));
        }
    } else if (current().type == lexer::TokenType::String) {
        auto value = current().get_value<std::string>();
        if (value) {
            err.with_context(fmt::format("near string \"{}\"", *value));
        }
    } else {
        err.with_context(
            fmt::format("near token type {}", lexer::Token::type_name(current().type)));
    }

    return err;
}

void Parser::report_error(ParseError err) {
    // Don't cascade errors when in panic mode
    if (panic_mode_) {
        return;
    }

    if (err.location().has_value()) {
        LOG_ERROR("Parse error at {}:{}: {}", err.location()->line, err.location()->column,
                  err.message());
    } else {
        LOG_ERROR("Parse error: {}", err.message());
    }

    errors_.push_back(std::move(err));
}

} // namespace finch::parser
