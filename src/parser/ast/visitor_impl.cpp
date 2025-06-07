#include <finch/parser/ast/commands.hpp>
#include <finch/parser/ast/control_flow.hpp>
#include <finch/parser/ast/cpm_nodes.hpp>
#include <finch/parser/ast/expressions.hpp>
#include <finch/parser/ast/literals.hpp>
#include <finch/parser/ast/structure.hpp>
#include <finch/parser/ast/visitor.hpp>

namespace finch::ast {

// Commands
void CommandCall::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void FunctionDef::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void MacroDef::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

// Control Flow
void IfStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void ElseIfStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void ElseStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void WhileStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void ForEachStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

// CPM Nodes
void CPMAddPackage::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void CPMFindPackage::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void CPMUsePackageLock::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void CPMDeclarePackage::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

// Expressions
void ListExpression::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void GeneratorExpression::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void BracketExpression::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void BinaryOp::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void UnaryOp::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void FunctionCall::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

// Literals
void StringLiteral::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void NumberLiteral::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void BooleanLiteral::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void Variable::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void Identifier::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

// Structure
void Block::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void File::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

void ErrorNode::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

} // namespace finch::ast
