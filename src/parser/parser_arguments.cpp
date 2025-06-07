#include <finch/core/logging.hpp>
#include <finch/parser/ast/expressions.hpp>
#include <finch/parser/ast/literals.hpp>
#include <finch/parser/lexer/interpolation.hpp>
#include <finch/parser/parser.hpp>

namespace finch::parser {

using namespace ast;

Result<ASTNodeList, ParseError> Parser::parse_arguments() {
    ASTNodeList args;

    while (!check(lexer::TokenType::RightParen) && !check(lexer::TokenType::Eof)) {
        // Skip whitespace and newlines between arguments
        while (match(lexer::TokenType::Whitespace) || match(lexer::TokenType::Newline)) {
        }

        // Check if we're at the end
        if (check(lexer::TokenType::RightParen) || check(lexer::TokenType::Eof)) {
            break;
        }

        // Parse an argument
        auto arg_result = parse_argument();
        if (!arg_result.has_value()) {
            return Err<ParseError, ASTNodeList>(std::move(arg_result.error()));
        }

        args.push_back(std::move(arg_result.value()));

        // Skip trailing whitespace
        while (match(lexer::TokenType::Whitespace)) {
        }

        // Arguments can be separated by semicolons
        if (match(lexer::TokenType::Semicolon)) {
            // Semicolon separator consumed
        }
        // Otherwise just whitespace/newline separation is fine
    }

    return Ok<ASTNodeList, ParseError>(std::move(args));
}

Result<ASTNodePtr, ParseError> Parser::parse_argument() {
    // Check for different argument types
    if (check(lexer::TokenType::String)) {
        return parse_quoted_argument();
    } else if (check(lexer::TokenType::LeftBracket)) {
        // Check if it's a bracket argument [[ ]] or just a bracket expression
        if (peek(1).type == lexer::TokenType::LeftBracket) {
            return parse_bracket_argument();
        } else {
            // Regular bracket expression
            return parse_expression();
        }
    } else {
        return parse_unquoted_argument();
    }
}

Result<ASTNodePtr, ParseError> Parser::parse_quoted_argument() {
    if (!check(lexer::TokenType::String)) {
        return Err<ParseError, ASTNodePtr>(error("Expected quoted string"));
    }

    const auto& tok = current();
    advance();

    auto value = tok.get_value<std::string>();
    if (!value) {
        return Err<ParseError, ASTNodePtr>(error("Invalid string token"));
    }

    // Check for interpolations in the string
    if (lexer::InterpolationLexer::has_interpolations(*value)) {
        // Parse as compound argument with variables
        auto parts_result =
            lexer::InterpolationLexer::parse_interpolated_string(*value, tok.location);
        if (!parts_result.has_value()) {
            return Err<ParseError, ASTNodePtr>(std::move(parts_result.error()));
        }

        ASTNodeList nodes;
        for (const auto& part : parts_result.value()) {
            if (part.type == lexer::InterpolatedPart::Literal) {
                nodes.push_back(builder_.makeString(part.location, part.value, false));
            } else {
                // Variable reference
                nodes.push_back(builder_.makeVariable(part.location, part.value));
            }
        }

        // If only one part, return it directly
        if (nodes.size() == 1) {
            return Ok<ASTNodePtr, ParseError>(std::move(nodes[0]));
        }

        // Otherwise create a list expression
        return Ok<ASTNodePtr, ParseError>(builder_.makeList(tok.location, std::move(nodes)));
    }

    // Simple string literal
    return Ok<ASTNodePtr, ParseError>(builder_.makeString(tok.location, *value, true));
}

Result<ASTNodePtr, ParseError> Parser::parse_unquoted_argument() {
    auto start_loc = current().location;
    std::string value;

    // Collect tokens that form an unquoted argument
    bool has_content = false;
    while (!check(lexer::TokenType::Whitespace) && !check(lexer::TokenType::Newline) &&
           !check(lexer::TokenType::RightParen) && !check(lexer::TokenType::Semicolon) &&
           !check(lexer::TokenType::Eof)) {

        const auto& tok = current();

        if (tok.type == lexer::TokenType::Identifier || tok.type == lexer::TokenType::Number) {
            // Add the text representation
            value += std::string(tok.text);
            has_content = true;
            advance();
        } else if (tok.type == lexer::TokenType::Variable) {
            // Handle variable references in unquoted arguments
            if (!value.empty()) {
                // Create what we have so far as a string
                ASTNodeList parts;
                parts.push_back(builder_.makeString(start_loc, value, false));
                value.clear();

                // Add the variable
                auto var_value = tok.get_value<std::string>();
                if (var_value) {
                    parts.push_back(builder_.makeVariable(tok.location, *var_value));
                }
                advance();

                // Continue collecting the rest
                while (!check(lexer::TokenType::Whitespace) &&
                       !check(lexer::TokenType::RightParen) &&
                       !check(lexer::TokenType::Semicolon) && !check(lexer::TokenType::Eof)) {

                    const auto& next_tok = current();
                    if (next_tok.type == lexer::TokenType::Variable) {
                        if (!value.empty()) {
                            parts.push_back(builder_.makeString(current().location, value, false));
                            value.clear();
                        }
                        auto next_var = next_tok.get_value<std::string>();
                        if (next_var) {
                            parts.push_back(builder_.makeVariable(next_tok.location, *next_var));
                        }
                    } else {
                        value += std::string(next_tok.text);
                    }
                    advance();
                }

                if (!value.empty()) {
                    parts.push_back(builder_.makeString(current().location, value, false));
                }

                return Ok<ASTNodePtr, ParseError>(builder_.makeList(start_loc, std::move(parts)));
            } else {
                // Variable at the start
                auto var_value = tok.get_value<std::string>();
                if (!var_value) {
                    return Err<ParseError, ASTNodePtr>(error("Invalid variable token"));
                }
                advance();
                return Ok<ASTNodePtr, ParseError>(builder_.makeVariable(tok.location, *var_value));
            }
        } else {
            // Other token types become part of the unquoted string
            value += std::string(tok.text);
            has_content = true;
            advance();
        }
    }

    if (!has_content) {
        return Err<ParseError, ASTNodePtr>(error("Expected argument"));
    }

    // Check if it's a boolean literal
    if (value == "TRUE" || value == "ON" || value == "YES" || value == "Y") {
        return Ok<ASTNodePtr, ParseError>(builder_.makeBoolean(start_loc, true, value));
    } else if (value == "FALSE" || value == "OFF" || value == "NO" || value == "N") {
        return Ok<ASTNodePtr, ParseError>(builder_.makeBoolean(start_loc, false, value));
    }

    // Check if it's a number
    char* end;
    long long_val = std::strtol(value.c_str(), &end, 0);
    if (end == value.c_str() + value.length()) {
        return Ok<ASTNodePtr, ParseError>(
            builder_.makeNumber(start_loc, value, static_cast<int64_t>(long_val)));
    }

    double double_val = std::strtod(value.c_str(), &end);
    if (end == value.c_str() + value.length()) {
        return Ok<ASTNodePtr, ParseError>(builder_.makeNumber(start_loc, value, double_val));
    }

    // Otherwise it's an unquoted string
    return Ok<ASTNodePtr, ParseError>(builder_.makeString(start_loc, value, false));
}

Result<ASTNodePtr, ParseError> Parser::parse_bracket_argument() {
    auto start_loc = current().location;

    // Consume opening [[
    auto lb1_result = consume(lexer::TokenType::LeftBracket, "Expected '['");
    if (!lb1_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(lb1_result.error()));
    }

    auto lb2_result = consume(lexer::TokenType::LeftBracket, "Expected second '['");
    if (!lb2_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(lb2_result.error()));
    }

    // Look for optional = signs to determine bracket depth
    int equals_count = 0;
    while (check(lexer::TokenType::Identifier)) {
        auto tok_value = current().get_value<std::string>();
        if (tok_value && *tok_value == "=") {
            equals_count++;
            advance();
        } else {
            break;
        }
    }

    // Collect content until we find the matching closing bracket
    std::string content;
    int bracket_depth = 0;

    while (!check(lexer::TokenType::Eof)) {
        if (check(lexer::TokenType::RightBracket)) {
            // Check if this is our closing bracket
            size_t pos = current_;
            advance();

            // Count equals signs
            int closing_equals = 0;
            while (check(lexer::TokenType::Identifier)) {
                auto tok_value = current().get_value<std::string>();
                if (tok_value && *tok_value == "=") {
                    closing_equals++;
                    advance();
                } else {
                    break;
                }
            }

            if (closing_equals == equals_count && check(lexer::TokenType::RightBracket)) {
                // This is our closing bracket
                advance();
                break;
            } else {
                // Not our closing bracket, rewind and add to content
                current_ = pos;
                content += std::string(current().text);
                advance();
            }
        } else {
            content += std::string(current().text);
            advance();
        }
    }

    // Create bracket expression node
    auto content_node = builder_.makeString(start_loc, content, false);
    return Ok<ASTNodePtr, ParseError>(
        builder_.makeBracketExpr(start_loc, std::move(content_node), true));
}

} // namespace finch::parser
