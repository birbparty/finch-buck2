#include <finch/core/logging.hpp>
#include <finch/core/try.hpp>
#include <finch/parser/ast/commands.hpp>
#include <finch/parser/ast/control_flow.hpp>
#include <finch/parser/ast/expressions.hpp>
#include <finch/parser/ast/literals.hpp>
#include <finch/parser/ast/structure.hpp>
#include <finch/parser/cpm_parser.hpp>
#include <finch/parser/parser.hpp>
#include <fmt/format.h>

namespace finch::parser {

using namespace ast;

Parser::Parser(std::string_view source, std::string filename)
    : lexer_(source, filename), filename_(std::move(filename)) {
    LOG_DEBUG("Creating parser for file: {}", filename_);
}

Parser::Parser(lexer::SourceBuffer buffer) : lexer_(buffer), filename_(buffer.filename()) {
    LOG_DEBUG("Creating parser from buffer for file: {}", filename_);
}

Result<std::unique_ptr<File>, std::vector<ParseError>> Parser::parse_file() {
    LOG_DEBUG("Starting parse of file: {}", filename_);

    // Parse all file elements
    auto elements_result = parse_file_elements();

    // Check if we collected any errors during parsing
    if (!errors_.empty()) {
        LOG_ERROR("Parse completed with {} errors", errors_.size());
        return Err<std::vector<ParseError>, std::unique_ptr<File>>(std::move(errors_));
    }

    // Check if the main parsing had an error
    if (!elements_result.has_value()) {
        errors_.push_back(std::move(elements_result.error()));
        return Err<std::vector<ParseError>, std::unique_ptr<File>>(std::move(errors_));
    }

    // Create the file node
    auto file = std::make_unique<File>(SourceLocation{filename_, 1, 1},
                                       builder_.interner().intern(filename_),
                                       std::move(elements_result.value()));

    LOG_DEBUG("Parse completed successfully with {} top-level elements", file->statements().size());
    LOG_DEBUG("String interner stats: {} unique strings", builder_.interner().unique_strings());

    return Ok<std::unique_ptr<File>, std::vector<ParseError>>(std::move(file));
}

Result<ASTNodeList, ParseError> Parser::parse_file_elements() {
    ASTNodeList elements;

    while (!check(lexer::TokenType::Eof)) {
        // Skip any leading trivia
        skip_trivia();

        if (check(lexer::TokenType::Eof)) {
            break;
        }

        // Parse next statement
        auto stmt_result = parse_statement();
        if (!stmt_result.has_value()) {
            report_error(std::move(stmt_result.error()));
            synchronize();
        } else {
            elements.push_back(std::move(stmt_result.value()));
        }

        // Skip trailing trivia
        skip_trivia();
    }

    return Ok<ASTNodeList, ParseError>(std::move(elements));
}

Result<ASTNodePtr, ParseError> Parser::parse_statement() {
    // Skip leading whitespace (but not newlines)
    while (match(lexer::TokenType::Whitespace)) {
    }

    // Check for commands and control flow
    if (check(lexer::TokenType::Identifier)) {
        const auto& tok = current();
        auto name_value = tok.get_value<std::string>();
        if (!name_value) {
            return Err<ParseError, ASTNodePtr>(error("Invalid identifier token"));
        }

        const auto& name = *name_value;

        // Check for control flow keywords
        if (name == "if") {
            return parse_if_statement();
        } else if (name == "foreach") {
            return parse_foreach_statement();
        } else if (name == "while") {
            return parse_while_statement();
        } else if (name == "function") {
            return parse_function_def();
        } else if (name == "macro") {
            return parse_macro_def();
        } else {
            // Regular command invocation
            return parse_command_invocation();
        }
    }

    // Handle comments as no-op statements
    if (match(lexer::TokenType::Comment) || match(lexer::TokenType::BracketComment)) {
        // Return a block node as a placeholder for comments
        return Ok<ASTNodePtr, ParseError>(builder_.makeBlock(current().location));
    }

    return Err<ParseError, ASTNodePtr>(error("Expected command or control flow statement"));
}

Result<ASTNodePtr, ParseError> Parser::parse_command_invocation() {
    auto start_loc = current().location;

    // Command name
    auto name_result = consume(lexer::TokenType::Identifier, "Expected command name");
    if (!name_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(name_result.error()));
    }

    const auto& name_token = token_buffer_[current_ - 1];
    auto name_value = name_token.get_value<std::string>();
    if (!name_value) {
        return Err<ParseError, ASTNodePtr>(error("Invalid command name"));
    }

    const auto& name = *name_value;

    // Skip whitespace
    while (match(lexer::TokenType::Whitespace)) {
    }

    // Opening parenthesis
    auto lparen_result = consume(lexer::TokenType::LeftParen, "Expected '(' after command name");
    if (!lparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(lparen_result.error()));
    }

    // Parse arguments
    auto args_result = parse_arguments();
    if (!args_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(args_result.error()));
    }

    // Closing parenthesis
    auto rparen_result = consume(lexer::TokenType::RightParen, "Expected ')' after arguments");
    if (!rparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(rparen_result.error()));
    }

    // Check if this is a CPM command
    if (name.starts_with("CPM")) {
        CPMParser cpm_parser(*this);
        auto cpm_result = cpm_parser.parse_cpm_command(name, args_result.value());
        if (cpm_result.has_value()) {
            return cpm_result;
        }
        // Fall through to regular command if not recognized
        LOG_DEBUG("CPM command {} not recognized, treating as regular command", name);
    }

    return Ok<ASTNodePtr, ParseError>(
        builder_.makeCommand(start_loc, name, std::move(args_result.value())));
}

// Token management methods
const lexer::Token& Parser::current() {
    if (token_buffer_.empty() || current_ >= token_buffer_.size()) {
        // Fill token buffer
        auto token_result = lexer_.next_token();
        if (token_result.has_value()) {
            token_buffer_.push_back(std::move(token_result.value()));
        } else {
            // Return EOF token on lexer error
            static const lexer::Token eof_token{lexer::TokenType::Eof, lexer::TokenValue{},
                                                SourceLocation{filename_, 1, 1}, ""};
            return eof_token;
        }
    }
    return token_buffer_[current_];
}

const lexer::Token& Parser::peek(size_t ahead) {
    // Ensure we have enough tokens buffered
    while (token_buffer_.size() <= current_ + ahead) {
        auto token_result = lexer_.next_token();
        if (token_result.has_value()) {
            token_buffer_.push_back(std::move(token_result.value()));
        } else {
            // Return EOF token on lexer error
            static const lexer::Token eof_token{lexer::TokenType::Eof, lexer::TokenValue{},
                                                SourceLocation{filename_, 1, 1}, ""};
            return eof_token;
        }
    }
    return token_buffer_[current_ + ahead];
}

Result<lexer::Token, ParseError> Parser::advance() {
    auto token = current();
    if (!check(lexer::TokenType::Eof)) {
        current_++;
    }
    return finch::Result<lexer::Token, ParseError>{token};
}

bool Parser::check(lexer::TokenType type) const {
    if (token_buffer_.empty() || current_ >= token_buffer_.size()) {
        return type == lexer::TokenType::Eof;
    }
    return token_buffer_[current_].type == type;
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
        return finch::Result<void, ParseError>{};
    }
    return Err<ParseError, void>(error(message));
}

void Parser::skip_trivia() {
    while (match(lexer::TokenType::Whitespace) || match(lexer::TokenType::Comment) ||
           match(lexer::TokenType::BracketComment)) {
        // Continue skipping
    }
}

// Error handling
void Parser::synchronize() {
    panic_mode_ = false;

    // Skip tokens until we find a likely statement boundary
    while (!check(lexer::TokenType::Eof)) {
        // Look for statement-ending tokens
        if (match(lexer::TokenType::Newline)) {
            return;
        }

        // Look for command names
        if (check(lexer::TokenType::Identifier)) {
            auto token = current();
            auto name_value = token.get_value<std::string>();
            if (name_value) {
                const auto& name = *name_value;
                if (name == "if" || name == "foreach" || name == "while" || name == "function" ||
                    name == "macro" || name == "endif" || name == "endforeach" ||
                    name == "endwhile" || name == "endfunction" || name == "endmacro") {
                    return;
                }
            }
        }

        advance();
    }
}

ParseError Parser::error(const std::string& message) {
    auto location = current().location;
    ParseError err{ParseError::Category::InvalidSyntax, message};
    err.at(location);
    return err;
}

void Parser::report_error(ParseError err) {
    errors_.push_back(std::move(err));
    panic_mode_ = true;
}

// Statement parsing implementations
Result<ASTNodePtr, ParseError> Parser::parse_if_statement() {
    auto start_loc = current().location;

    // 'if' keyword
    auto if_result = consume(lexer::TokenType::Identifier, "Expected 'if'");
    if (!if_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(if_result.error()));
    }

    skip_trivia();

    // Opening parenthesis
    auto lparen_result = consume(lexer::TokenType::LeftParen, "Expected '(' after 'if'");
    if (!lparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(lparen_result.error()));
    }

    // Condition expression
    auto condition_result = parse_expression();
    if (!condition_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(condition_result.error()));
    }

    // Closing parenthesis
    auto rparen_result = consume(lexer::TokenType::RightParen, "Expected ')' after condition");
    if (!rparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(rparen_result.error()));
    }

    skip_trivia();

    // Parse then block
    std::vector<std::string_view> terminators = {"else", "elseif", "endif"};
    auto then_block_result = parse_block_until(terminators);
    if (!then_block_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(then_block_result.error()));
    }

    // Handle optional else/elseif
    ASTNodeList else_block;
    while (check(lexer::TokenType::Identifier)) {
        auto token = current();
        auto name_value = token.get_value<std::string>();
        if (!name_value)
            break;

        const auto& name = *name_value;
        if (name == "else") {
            advance(); // consume 'else'
            skip_trivia();

            std::vector<std::string_view> end_terminators = {"endif"};
            auto else_result = parse_block_until(end_terminators);
            if (!else_result.has_value()) {
                return Err<ParseError, ASTNodePtr>(std::move(else_result.error()));
            }
            else_block = std::move(else_result.value());
            break;
        } else if (name == "elseif") {
            // TODO: Handle elseif properly
            advance();
            skip_trivia();
        } else if (name == "endif") {
            break;
        } else {
            break;
        }
    }

    // 'endif'
    auto endif_result = consume(lexer::TokenType::Identifier, "Expected 'endif'");
    if (!endif_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(endif_result.error()));
    }

    return Ok<ASTNodePtr, ParseError>(builder_.makeIf(
        start_loc, std::move(condition_result.value()), std::move(then_block_result.value())));
}

Result<ASTNodePtr, ParseError> Parser::parse_foreach_statement() {
    auto start_loc = current().location;

    // 'foreach' keyword
    auto foreach_result = consume(lexer::TokenType::Identifier, "Expected 'foreach'");
    if (!foreach_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(foreach_result.error()));
    }

    skip_trivia();

    // Opening parenthesis
    auto lparen_result = consume(lexer::TokenType::LeftParen, "Expected '(' after 'foreach'");
    if (!lparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(lparen_result.error()));
    }

    // Parse arguments (loop variable and list)
    auto args_result = parse_arguments();
    if (!args_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(args_result.error()));
    }

    // Closing parenthesis
    auto rparen_result =
        consume(lexer::TokenType::RightParen, "Expected ')' after foreach arguments");
    if (!rparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(rparen_result.error()));
    }

    skip_trivia();

    // Parse body
    std::vector<std::string_view> terminators = {"endforeach"};
    auto body_result = parse_block_until(terminators);
    if (!body_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(body_result.error()));
    }

    // 'endforeach'
    auto endforeach_result = consume(lexer::TokenType::Identifier, "Expected 'endforeach'");
    if (!endforeach_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(endforeach_result.error()));
    }

    // For a basic foreach, we need to extract the variables and items from args
    std::vector<std::string_view> variables = {"var"}; // placeholder
    ASTNodeList items;
    return Ok<ASTNodePtr, ParseError>(builder_.makeForEach(
        start_loc, std::move(variables), ast::ForEachStatement::LoopType::IN_ITEMS,
        std::move(items), std::move(body_result.value())));
}

Result<ASTNodePtr, ParseError> Parser::parse_while_statement() {
    return Err<ParseError, ASTNodePtr>(error("While statements not yet implemented"));
}

Result<ASTNodePtr, ParseError> Parser::parse_function_def() {
    return Err<ParseError, ASTNodePtr>(error("Function definitions not yet implemented"));
}

Result<ASTNodePtr, ParseError> Parser::parse_macro_def() {
    return Err<ParseError, ASTNodePtr>(error("Macro definitions not yet implemented"));
}

// Expression parsing
Result<ASTNodePtr, ParseError> Parser::parse_expression() {
    // For now, parse a simple primary expression
    return parse_primary_expression();
}

Result<ASTNodePtr, ParseError> Parser::parse_primary_expression() {
    // Parse arguments as expressions for now
    auto args_result = parse_arguments();
    if (!args_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(args_result.error()));
    }

    // Return first argument as expression, or create a list expression
    auto& args = args_result.value();
    if (args.empty()) {
        return Err<ParseError, ASTNodePtr>(error("Expected expression"));
    }

    // If single argument, return it
    if (args.size() == 1) {
        return Ok<ASTNodePtr, ParseError>(std::move(args[0]));
    }

    // Multiple arguments - create expression list
    return Ok<ASTNodePtr, ParseError>(builder_.makeList(current().location, std::move(args)));
}

// Control flow helpers
Result<ASTNodeList, ParseError>
Parser::parse_block_until(const std::vector<std::string_view>& terminators) {
    ASTNodeList statements;

    while (!check(lexer::TokenType::Eof) && !is_at_block_terminator(terminators)) {
        skip_trivia();

        if (check(lexer::TokenType::Eof) || is_at_block_terminator(terminators)) {
            break;
        }

        auto stmt_result = parse_statement();
        if (!stmt_result.has_value()) {
            report_error(std::move(stmt_result.error()));
            synchronize();
        } else {
            statements.push_back(std::move(stmt_result.value()));
        }

        skip_trivia();
    }

    return Ok<ASTNodeList, ParseError>(std::move(statements));
}

bool Parser::is_at_block_terminator(const std::vector<std::string_view>& terminators) {
    if (!check(lexer::TokenType::Identifier)) {
        return false;
    }

    auto token = current();
    auto name_value = token.get_value<std::string>();
    if (!name_value) {
        return false;
    }

    const auto& name = *name_value;
    for (const auto& terminator : terminators) {
        if (name == terminator) {
            return true;
        }
    }

    return false;
}

Result<ast::ForEachStatement::LoopType, ParseError> Parser::parse_foreach_loop_type() {
    // Default to items loop for now
    return Ok<ast::ForEachStatement::LoopType, ParseError>(
        ast::ForEachStatement::LoopType::IN_ITEMS);
}

} // namespace finch::parser
