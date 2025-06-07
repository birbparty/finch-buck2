#include <finch/parser/ast/commands.hpp>
#include <finch/parser/ast/control_flow.hpp>
#include <finch/parser/ast/cpm_nodes.hpp>
#include <finch/parser/ast/expressions.hpp>
#include <finch/parser/ast/literals.hpp>
#include <finch/parser/ast/structure.hpp>

namespace finch::ast {

// Control Flow
std::unique_ptr<ASTNode> IfStatement::clone() const {
    auto cloned_condition = condition_->clone();
    ASTNodeList cloned_then;
    for (const auto& stmt : then_branch_) {
        cloned_then.push_back(stmt->clone());
    }
    auto result = std::make_unique<IfStatement>(location(), std::move(cloned_condition),
                                                std::move(cloned_then));

    // Clone elseif branches
    for (const auto& elseif : elseif_branches_) {
        result->elseif_branches_.push_back(elseif->clone());
    }

    // Clone else branch
    for (const auto& stmt : else_branch_) {
        result->else_branch_.push_back(stmt->clone());
    }

    return result;
}

std::unique_ptr<ASTNode> ElseIfStatement::clone() const {
    return std::make_unique<ElseIfStatement>(location(), condition_->clone());
}

std::unique_ptr<ASTNode> ElseStatement::clone() const {
    return std::make_unique<ElseStatement>(location());
}

std::unique_ptr<ASTNode> WhileStatement::clone() const {
    auto cloned_condition = condition_->clone();
    ASTNodeList cloned_body;
    for (const auto& stmt : body_) {
        cloned_body.push_back(stmt->clone());
    }
    return std::make_unique<WhileStatement>(location(), std::move(cloned_condition),
                                            std::move(cloned_body));
}

std::unique_ptr<ASTNode> ForEachStatement::clone() const {
    ASTNodeList cloned_items;
    for (const auto& item : items_) {
        cloned_items.push_back(item->clone());
    }
    ASTNodeList cloned_body;
    for (const auto& stmt : body_) {
        cloned_body.push_back(stmt->clone());
    }
    return std::make_unique<ForEachStatement>(location(), variables_, loop_type_,
                                              std::move(cloned_items), std::move(cloned_body));
}

// CPM Nodes - clone() is implemented in header files

// Expressions
std::unique_ptr<ASTNode> ListExpression::clone() const {
    ASTNodeList cloned_elements;
    for (const auto& elem : elements_) {
        cloned_elements.push_back(elem->clone());
    }
    return std::make_unique<ListExpression>(location(), std::move(cloned_elements), separator_);
}

std::unique_ptr<ASTNode> GeneratorExpression::clone() const {
    return std::make_unique<GeneratorExpression>(location(), expression_);
}

std::unique_ptr<ASTNode> BracketExpression::clone() const {
    return std::make_unique<BracketExpression>(location(), content_->clone(), is_quoted_);
}

std::unique_ptr<ASTNode> BinaryOp::clone() const {
    return std::make_unique<BinaryOp>(location(), left_->clone(), op_, right_->clone());
}

std::unique_ptr<ASTNode> UnaryOp::clone() const {
    return std::make_unique<UnaryOp>(location(), op_, operand_->clone());
}

std::unique_ptr<ASTNode> FunctionCall::clone() const {
    ASTNodeList cloned_args;
    for (const auto& arg : arguments_) {
        cloned_args.push_back(arg->clone());
    }
    return std::make_unique<FunctionCall>(location(), name_, std::move(cloned_args));
}

// Literals - clone() is implemented in header files

// Structure
std::unique_ptr<ASTNode> Block::clone() const {
    ASTNodeList cloned_stmts;
    for (const auto& stmt : statements_) {
        cloned_stmts.push_back(stmt->clone());
    }
    return std::make_unique<Block>(location(), std::move(cloned_stmts));
}

std::unique_ptr<ASTNode> File::clone() const {
    ASTNodeList cloned_stmts;
    for (const auto& stmt : statements_) {
        cloned_stmts.push_back(stmt->clone());
    }
    auto cloned = std::make_unique<File>(location(), path_, std::move(cloned_stmts));
    if (content_hash_.has_value()) {
        cloned->set_content_hash(*content_hash_);
    }
    return cloned;
}

std::unique_ptr<ASTNode> Include::clone() const {
    auto cloned = std::make_unique<Include>(location(), path_, is_optional_);
    if (is_no_policy_scope_) {
        cloned->set_no_policy_scope(true);
    }
    if (result_var_.has_value()) {
        cloned->set_result_variable(*result_var_);
    }
    return cloned;
}

} // namespace finch::ast
