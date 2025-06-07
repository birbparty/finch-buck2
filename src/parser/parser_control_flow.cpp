#include <finch/core/logging.hpp>
#include <finch/parser/ast/control_flow.hpp>
#include <finch/parser/ast/expressions.hpp>
#include <finch/parser/ast/literals.hpp>
#include <finch/parser/parser.hpp>

namespace finch::parser {

using namespace ast;

Result<ASTNodePtr, ParseError> Parser::parse_if_statement() {
    auto start_loc = current().location;

    // Consume 'if'
    auto if_result = consume(lexer::TokenType::Identifier, "Expected 'if'");
    if (!if_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(if_result.error()));
    }

    // Skip whitespace
    while (match(lexer::TokenType::Whitespace)) {
    }

    // Opening parenthesis
    auto lparen_result = consume(lexer::TokenType::LeftParen, "Expected '(' after 'if'");
    if (!lparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(lparen_result.error()));
    }

    // Parse condition
    auto condition_result = parse_expression();
    if (!condition_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(condition_result.error()));
    }

    // Closing parenthesis
    auto rparen_result = consume(lexer::TokenType::RightParen, "Expected ')' after condition");
    if (!rparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(rparen_result.error()));
    }

    // Skip to next line
    while (match(lexer::TokenType::Whitespace) || match(lexer::TokenType::Newline)) {
    }

    // Parse then branch
    std::vector<std::string_view> terminators = {"else", "elseif", "endif"};
    auto then_result = parse_block_until(terminators);
    if (!then_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(then_result.error()));
    }

    // Create if statement
    auto if_stmt = std::make_unique<IfStatement>(start_loc, std::move(condition_result.value()),
                                                 std::move(then_result.value()));

    // Handle elseif/else branches
    while (check(lexer::TokenType::Identifier)) {
        auto name_value = current().get_value<std::string>();
        if (!name_value) {
            break;
        }

        if (*name_value == "elseif") {
            // Parse elseif
            advance();
            while (match(lexer::TokenType::Whitespace)) {
            }

            auto elseif_lparen =
                consume(lexer::TokenType::LeftParen, "Expected '(' after 'elseif'");
            if (!elseif_lparen.has_value()) {
                return Err<ParseError, ASTNodePtr>(std::move(elseif_lparen.error()));
            }

            auto elseif_condition = parse_expression();
            if (!elseif_condition.has_value()) {
                return Err<ParseError, ASTNodePtr>(std::move(elseif_condition.error()));
            }

            auto elseif_rparen =
                consume(lexer::TokenType::RightParen, "Expected ')' after elseif condition");
            if (!elseif_rparen.has_value()) {
                return Err<ParseError, ASTNodePtr>(std::move(elseif_rparen.error()));
            }

            while (match(lexer::TokenType::Whitespace) || match(lexer::TokenType::Newline)) {
            }

            auto elseif_body = parse_block_until(terminators);
            if (!elseif_body.has_value()) {
                return Err<ParseError, ASTNodePtr>(std::move(elseif_body.error()));
            }

            if_stmt->add_elseif(std::move(elseif_condition.value()),
                                std::move(elseif_body.value()));
        } else if (*name_value == "else") {
            // Parse else
            advance();
            while (match(lexer::TokenType::Whitespace)) {
            }

            auto else_lparen = consume(lexer::TokenType::LeftParen, "Expected '(' after 'else'");
            if (!else_lparen.has_value()) {
                return Err<ParseError, ASTNodePtr>(std::move(else_lparen.error()));
            }

            auto else_rparen = consume(lexer::TokenType::RightParen, "Expected ')' after 'else('");
            if (!else_rparen.has_value()) {
                return Err<ParseError, ASTNodePtr>(std::move(else_rparen.error()));
            }

            while (match(lexer::TokenType::Whitespace) || match(lexer::TokenType::Newline)) {
            }

            std::vector<std::string_view> else_terminators = {"endif"};
            auto else_body = parse_block_until(else_terminators);
            if (!else_body.has_value()) {
                return Err<ParseError, ASTNodePtr>(std::move(else_body.error()));
            }

            if_stmt->set_else_branch(std::move(else_body.value()));
            break; // No more branches after else
        } else {
            break;
        }
    }

    // Consume endif
    auto endif_result = consume(lexer::TokenType::Identifier, "Expected 'endif'");
    if (!endif_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(endif_result.error()));
    }

    while (match(lexer::TokenType::Whitespace)) {
    }

    auto endif_lparen = consume(lexer::TokenType::LeftParen, "Expected '(' after 'endif'");
    if (!endif_lparen.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(endif_lparen.error()));
    }

    auto endif_rparen = consume(lexer::TokenType::RightParen, "Expected ')' after 'endif('");
    if (!endif_rparen.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(endif_rparen.error()));
    }

    return Ok<ASTNodePtr, ParseError>(std::move(if_stmt));
}

Result<ASTNodePtr, ParseError> Parser::parse_foreach_statement() {
    auto start_loc = current().location;

    // Consume 'foreach'
    auto foreach_result = consume(lexer::TokenType::Identifier, "Expected 'foreach'");
    if (!foreach_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(foreach_result.error()));
    }

    while (match(lexer::TokenType::Whitespace)) {
    }

    auto lparen_result = consume(lexer::TokenType::LeftParen, "Expected '(' after 'foreach'");
    if (!lparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(lparen_result.error()));
    }

    // Parse loop variables
    std::vector<std::string_view> variables;
    while (check(lexer::TokenType::Identifier)) {
        auto var_tok = current();
        auto var_value = var_tok.get_value<std::string>();
        if (!var_value) {
            return Err<ParseError, ASTNodePtr>(error("Invalid variable name"));
        }

        // Check if this is actually a loop type keyword
        if (*var_value == "IN" || *var_value == "RANGE") {
            break;
        }

        variables.push_back(builder_.interner().intern(*var_value));
        advance();

        while (match(lexer::TokenType::Whitespace)) {
        }
    }

    if (variables.empty()) {
        return Err<ParseError, ASTNodePtr>(error("Expected loop variable(s) in foreach"));
    }

    // Parse loop type
    auto loop_type_result = parse_foreach_loop_type();
    if (!loop_type_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(loop_type_result.error()));
    }

    // Parse items/lists/range parameters
    auto items_result = parse_arguments();
    if (!items_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(items_result.error()));
    }

    auto rparen_result =
        consume(lexer::TokenType::RightParen, "Expected ')' after foreach arguments");
    if (!rparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(rparen_result.error()));
    }

    while (match(lexer::TokenType::Whitespace) || match(lexer::TokenType::Newline)) {
    }

    // Parse body
    std::vector<std::string_view> terminators = {"endforeach"};
    auto body_result = parse_block_until(terminators);
    if (!body_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(body_result.error()));
    }

    // Consume endforeach
    auto end_result = consume(lexer::TokenType::Identifier, "Expected 'endforeach'");
    if (!end_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(end_result.error()));
    }

    while (match(lexer::TokenType::Whitespace)) {
    }

    auto end_lparen = consume(lexer::TokenType::LeftParen, "Expected '(' after 'endforeach'");
    if (!end_lparen.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(end_lparen.error()));
    }

    auto end_rparen = consume(lexer::TokenType::RightParen, "Expected ')' after 'endforeach('");
    if (!end_rparen.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(end_rparen.error()));
    }

    return Ok<ASTNodePtr, ParseError>(
        builder_.makeForEach(start_loc, std::move(variables), loop_type_result.value(),
                             std::move(items_result.value()), std::move(body_result.value())));
}

Result<ForEachStatement::LoopType, ParseError> Parser::parse_foreach_loop_type() {
    if (!check(lexer::TokenType::Identifier)) {
        return Err<ParseError, ForEachStatement::LoopType>(
            error("Expected loop type (IN, RANGE, etc.)"));
    }

    auto type_value = current().get_value<std::string>();
    if (!type_value) {
        return Err<ParseError, ForEachStatement::LoopType>(error("Invalid loop type token"));
    }

    if (*type_value == "RANGE") {
        advance();
        return Ok<ForEachStatement::LoopType, ParseError>(ForEachStatement::LoopType::RANGE);
    }

    if (*type_value != "IN") {
        return Err<ParseError, ForEachStatement::LoopType>(
            error("Expected 'IN' or 'RANGE' in foreach"));
    }

    advance();
    while (match(lexer::TokenType::Whitespace)) {
    }

    // Check for IN modifiers (LISTS, ITEMS, ZIP_LISTS)
    if (check(lexer::TokenType::Identifier)) {
        auto modifier_value = current().get_value<std::string>();
        if (modifier_value) {
            if (*modifier_value == "LISTS") {
                advance();
                return Ok<ForEachStatement::LoopType, ParseError>(
                    ForEachStatement::LoopType::IN_LISTS);
            } else if (*modifier_value == "ITEMS") {
                advance();
                return Ok<ForEachStatement::LoopType, ParseError>(
                    ForEachStatement::LoopType::IN_ITEMS);
            } else if (*modifier_value == "ZIP_LISTS") {
                advance();
                return Ok<ForEachStatement::LoopType, ParseError>(
                    ForEachStatement::LoopType::IN_ZIP_LISTS);
            }
        }
    }

    // Default IN
    return Ok<ForEachStatement::LoopType, ParseError>(ForEachStatement::LoopType::IN);
}

Result<ASTNodePtr, ParseError> Parser::parse_while_statement() {
    auto start_loc = current().location;

    // Consume 'while'
    auto while_result = consume(lexer::TokenType::Identifier, "Expected 'while'");
    if (!while_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(while_result.error()));
    }

    while (match(lexer::TokenType::Whitespace)) {
    }

    auto lparen_result = consume(lexer::TokenType::LeftParen, "Expected '(' after 'while'");
    if (!lparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(lparen_result.error()));
    }

    // Parse condition
    auto condition_result = parse_expression();
    if (!condition_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(condition_result.error()));
    }

    auto rparen_result = consume(lexer::TokenType::RightParen, "Expected ')' after condition");
    if (!rparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(rparen_result.error()));
    }

    while (match(lexer::TokenType::Whitespace) || match(lexer::TokenType::Newline)) {
    }

    // Parse body
    std::vector<std::string_view> terminators = {"endwhile"};
    auto body_result = parse_block_until(terminators);
    if (!body_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(body_result.error()));
    }

    // Consume endwhile
    auto end_result = consume(lexer::TokenType::Identifier, "Expected 'endwhile'");
    if (!end_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(end_result.error()));
    }

    while (match(lexer::TokenType::Whitespace)) {
    }

    auto end_lparen = consume(lexer::TokenType::LeftParen, "Expected '(' after 'endwhile'");
    if (!end_lparen.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(end_lparen.error()));
    }

    auto end_rparen = consume(lexer::TokenType::RightParen, "Expected ')' after 'endwhile('");
    if (!end_rparen.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(end_rparen.error()));
    }

    return Ok<ASTNodePtr, ParseError>(builder_.makeWhile(
        start_loc, std::move(condition_result.value()), std::move(body_result.value())));
}

Result<ASTNodePtr, ParseError> Parser::parse_function_def() {
    auto start_loc = current().location;

    // Consume 'function'
    auto func_result = consume(lexer::TokenType::Identifier, "Expected 'function'");
    if (!func_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(func_result.error()));
    }

    while (match(lexer::TokenType::Whitespace)) {
    }

    auto lparen_result = consume(lexer::TokenType::LeftParen, "Expected '(' after 'function'");
    if (!lparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(lparen_result.error()));
    }

    // Parse function name
    if (!check(lexer::TokenType::Identifier)) {
        return Err<ParseError, ASTNodePtr>(error("Expected function name"));
    }

    auto name_value = current().get_value<std::string>();
    if (!name_value) {
        return Err<ParseError, ASTNodePtr>(error("Invalid function name"));
    }

    std::string_view name = builder_.interner().intern(*name_value);
    advance();

    while (match(lexer::TokenType::Whitespace)) {
    }

    // Parse parameters
    std::vector<std::string_view> parameters;
    while (!check(lexer::TokenType::RightParen) && !check(lexer::TokenType::Eof)) {
        if (check(lexer::TokenType::Identifier)) {
            auto param_value = current().get_value<std::string>();
            if (param_value) {
                parameters.push_back(builder_.interner().intern(*param_value));
                advance();
            }
        }

        while (match(lexer::TokenType::Whitespace)) {
        }
    }

    auto rparen_result =
        consume(lexer::TokenType::RightParen, "Expected ')' after function parameters");
    if (!rparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(rparen_result.error()));
    }

    while (match(lexer::TokenType::Whitespace) || match(lexer::TokenType::Newline)) {
    }

    // Parse body
    std::vector<std::string_view> terminators = {"endfunction"};
    auto body_result = parse_block_until(terminators);
    if (!body_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(body_result.error()));
    }

    // Consume endfunction
    auto end_result = consume(lexer::TokenType::Identifier, "Expected 'endfunction'");
    if (!end_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(end_result.error()));
    }

    while (match(lexer::TokenType::Whitespace)) {
    }

    auto end_lparen = consume(lexer::TokenType::LeftParen, "Expected '(' after 'endfunction'");
    if (!end_lparen.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(end_lparen.error()));
    }

    auto end_rparen = consume(lexer::TokenType::RightParen, "Expected ')' after 'endfunction('");
    if (!end_rparen.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(end_rparen.error()));
    }

    return Ok<ASTNodePtr, ParseError>(builder_.makeFunction(start_loc, name, std::move(parameters),
                                                            std::move(body_result.value())));
}

Result<ASTNodePtr, ParseError> Parser::parse_macro_def() {
    auto start_loc = current().location;

    // Consume 'macro'
    auto macro_result = consume(lexer::TokenType::Identifier, "Expected 'macro'");
    if (!macro_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(macro_result.error()));
    }

    while (match(lexer::TokenType::Whitespace)) {
    }

    auto lparen_result = consume(lexer::TokenType::LeftParen, "Expected '(' after 'macro'");
    if (!lparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(lparen_result.error()));
    }

    // Parse macro name
    if (!check(lexer::TokenType::Identifier)) {
        return Err<ParseError, ASTNodePtr>(error("Expected macro name"));
    }

    auto name_value = current().get_value<std::string>();
    if (!name_value) {
        return Err<ParseError, ASTNodePtr>(error("Invalid macro name"));
    }

    std::string_view name = builder_.interner().intern(*name_value);
    advance();

    while (match(lexer::TokenType::Whitespace)) {
    }

    // Parse parameters
    std::vector<std::string_view> parameters;
    while (!check(lexer::TokenType::RightParen) && !check(lexer::TokenType::Eof)) {
        if (check(lexer::TokenType::Identifier)) {
            auto param_value = current().get_value<std::string>();
            if (param_value) {
                parameters.push_back(builder_.interner().intern(*param_value));
                advance();
            }
        }

        while (match(lexer::TokenType::Whitespace)) {
        }
    }

    auto rparen_result =
        consume(lexer::TokenType::RightParen, "Expected ')' after macro parameters");
    if (!rparen_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(rparen_result.error()));
    }

    while (match(lexer::TokenType::Whitespace) || match(lexer::TokenType::Newline)) {
    }

    // Parse body
    std::vector<std::string_view> terminators = {"endmacro"};
    auto body_result = parse_block_until(terminators);
    if (!body_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(body_result.error()));
    }

    // Consume endmacro
    auto end_result = consume(lexer::TokenType::Identifier, "Expected 'endmacro'");
    if (!end_result.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(end_result.error()));
    }

    while (match(lexer::TokenType::Whitespace)) {
    }

    auto end_lparen = consume(lexer::TokenType::LeftParen, "Expected '(' after 'endmacro'");
    if (!end_lparen.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(end_lparen.error()));
    }

    auto end_rparen = consume(lexer::TokenType::RightParen, "Expected ')' after 'endmacro('");
    if (!end_rparen.has_value()) {
        return Err<ParseError, ASTNodePtr>(std::move(end_rparen.error()));
    }

    return Ok<ASTNodePtr, ParseError>(
        builder_.makeMacro(start_loc, name, std::move(parameters), std::move(body_result.value())));
}

Result<ASTNodeList, ParseError>
Parser::parse_block_until(const std::vector<std::string_view>& terminators) {
    ASTNodeList statements;

    while (!check(lexer::TokenType::Eof)) {
        skip_trivia();

        if (is_at_block_terminator(terminators)) {
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

    auto name_value = current().get_value<std::string>();
    if (!name_value) {
        return false;
    }

    for (const auto& terminator : terminators) {
        if (*name_value == terminator) {
            return true;
        }
    }

    return false;
}

Result<ASTNodePtr, ParseError> Parser::parse_expression() {
    // For now, just parse a primary expression
    // TODO: Add support for binary operators (AND, OR, NOT, etc.)
    return parse_primary_expression();
}

Result<ASTNodePtr, ParseError> Parser::parse_primary_expression() {
    skip_trivia();

    // Variable reference
    if (check(lexer::TokenType::Variable)) {
        auto var_tok = current();
        auto var_value = var_tok.get_value<std::string>();
        if (!var_value) {
            return Err<ParseError, ASTNodePtr>(error("Invalid variable token"));
        }
        advance();
        return Ok<ASTNodePtr, ParseError>(builder_.makeVariable(var_tok.location, *var_value));
    }

    // String literal
    if (check(lexer::TokenType::String)) {
        return parse_quoted_argument();
    }

    // Identifier (unquoted string or boolean)
    if (check(lexer::TokenType::Identifier)) {
        return parse_unquoted_argument();
    }

    // Number
    if (check(lexer::TokenType::Number)) {
        auto num_tok = current();
        auto num_value = num_tok.get_value<double>();
        if (!num_value) {
            return Err<ParseError, ASTNodePtr>(error("Invalid number token"));
        }
        advance();

        // Check if it's an integer
        if (*num_value == static_cast<int64_t>(*num_value)) {
            return Ok<ASTNodePtr, ParseError>(builder_.makeNumber(
                num_tok.location, num_tok.text, static_cast<int64_t>(*num_value)));
        } else {
            return Ok<ASTNodePtr, ParseError>(
                builder_.makeNumber(num_tok.location, num_tok.text, *num_value));
        }
    }

    // Generator expression
    if (check(lexer::TokenType::GeneratorExpr)) {
        auto gen_tok = current();
        auto gen_value = gen_tok.get_value<std::string>();
        if (!gen_value) {
            return Err<ParseError, ASTNodePtr>(error("Invalid generator expression token"));
        }
        advance();
        return Ok<ASTNodePtr, ParseError>(builder_.makeGeneratorExpr(gen_tok.location, *gen_value));
    }

    return Err<ParseError, ASTNodePtr>(error("Expected expression"));
}

} // namespace finch::parser
