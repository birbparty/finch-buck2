#pragma once

namespace finch::ast {

// Forward declaration of base node type
class ASTNode;

// Forward declarations for all node types
class StringLiteral;
class NumberLiteral;
class BooleanLiteral;
class Variable;
class Identifier;
class CommandCall;
class FunctionDef;
class MacroDef;
class IfStatement;
class ElseIfStatement;
class ElseStatement;
class WhileStatement;
class ForEachStatement;
class ListExpression;
class GeneratorExpression;
class BracketExpression;
class BinaryOp;
class UnaryOp;
class FunctionCall;
class Block;
class File;
class ErrorNode;
class CPMAddPackage;
class CPMFindPackage;
class CPMUsePackageLock;
class CPMDeclarePackage;

/// Visitor interface for AST traversal
class ASTVisitor {
  public:
    virtual ~ASTVisitor() = default;

    // Visit methods for all node types
    virtual void visit(const StringLiteral& node) = 0;
    virtual void visit(const NumberLiteral& node) = 0;
    virtual void visit(const BooleanLiteral& node) = 0;
    virtual void visit(const Variable& node) = 0;
    virtual void visit(const Identifier& node) = 0;
    virtual void visit(const CommandCall& node) = 0;
    virtual void visit(const FunctionDef& node) = 0;
    virtual void visit(const MacroDef& node) = 0;
    virtual void visit(const IfStatement& node) = 0;
    virtual void visit(const ElseIfStatement& node) = 0;
    virtual void visit(const ElseStatement& node) = 0;
    virtual void visit(const WhileStatement& node) = 0;
    virtual void visit(const ForEachStatement& node) = 0;
    virtual void visit(const ListExpression& node) = 0;
    virtual void visit(const GeneratorExpression& node) = 0;
    virtual void visit(const BracketExpression& node) = 0;
    virtual void visit(const BinaryOp& node) = 0;
    virtual void visit(const UnaryOp& node) = 0;
    virtual void visit(const FunctionCall& node) = 0;
    virtual void visit(const Block& node) = 0;
    virtual void visit(const File& node) = 0;
    virtual void visit(const ErrorNode& node) = 0;
    virtual void visit(const CPMAddPackage& node) = 0;
    virtual void visit(const CPMFindPackage& node) = 0;
    virtual void visit(const CPMUsePackageLock& node) = 0;
    virtual void visit(const CPMDeclarePackage& node) = 0;
};

/// Base visitor with default (empty) implementations
class ASTVisitorBase : public ASTVisitor {
  public:
    void visit(const StringLiteral&) override {}
    void visit(const NumberLiteral&) override {}
    void visit(const BooleanLiteral&) override {}
    void visit(const Variable&) override {}
    void visit(const Identifier&) override {}
    void visit(const CommandCall&) override {}
    void visit(const FunctionDef&) override {}
    void visit(const MacroDef&) override {}
    void visit(const IfStatement&) override {}
    void visit(const ElseIfStatement&) override {}
    void visit(const ElseStatement&) override {}
    void visit(const WhileStatement&) override {}
    void visit(const ForEachStatement&) override {}
    void visit(const ListExpression&) override {}
    void visit(const GeneratorExpression&) override {}
    void visit(const BracketExpression&) override {}
    void visit(const BinaryOp&) override {}
    void visit(const UnaryOp&) override {}
    void visit(const FunctionCall&) override {}
    void visit(const Block&) override {}
    void visit(const File&) override {}
    void visit(const ErrorNode&) override {}
    void visit(const CPMAddPackage&) override {}
    void visit(const CPMFindPackage&) override {}
    void visit(const CPMUsePackageLock&) override {}
    void visit(const CPMDeclarePackage&) override {}
};

/// Recursive visitor that automatically visits children
class RecursiveASTVisitor : public ASTVisitorBase {
  public:
    // Override these to add custom behavior
    virtual void visitPre(const ASTNode& node) {
        (void)node;
    }
    virtual void visitPost(const ASTNode& node) {
        (void)node;
    }

    // Implementations that visit children
    void visit(const CommandCall& node) override;
    void visit(const FunctionDef& node) override;
    void visit(const MacroDef& node) override;
    void visit(const IfStatement& node) override;
    void visit(const WhileStatement& node) override;
    void visit(const ForEachStatement& node) override;
    void visit(const ListExpression& node) override;
    void visit(const BracketExpression& node) override;
    void visit(const BinaryOp& node) override;
    void visit(const UnaryOp& node) override;
    void visit(const FunctionCall& node) override;
    void visit(const Block& node) override;
    void visit(const File& node) override;
};

} // namespace finch::ast

// Now we need to define the accept methods for all nodes
// This needs to be done after the visitor is declared

#include "commands.hpp"
#include "control_flow.hpp"
#include "cpm_nodes.hpp"
#include "expressions.hpp"
#include "literals.hpp"
#include "structure.hpp"

namespace finch::ast {

// Define accept methods inline
inline void StringLiteral::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void NumberLiteral::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void BooleanLiteral::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void Variable::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void Identifier::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void CommandCall::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void FunctionDef::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void MacroDef::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void IfStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void ElseIfStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void ElseStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void WhileStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void ForEachStatement::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void ListExpression::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void GeneratorExpression::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void BracketExpression::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void BinaryOp::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void UnaryOp::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void FunctionCall::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void Block::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void File::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void ErrorNode::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void CPMAddPackage::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void CPMFindPackage::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void CPMUsePackageLock::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

inline void CPMDeclarePackage::accept(ASTVisitor& visitor) const {
    visitor.visit(*this);
}

// RecursiveASTVisitor implementations
inline void RecursiveASTVisitor::visit(const CommandCall& node) {
    visitPre(node);
    for (const auto& arg : node.arguments()) {
        arg->accept(*this);
    }
    visitPost(node);
}

inline void RecursiveASTVisitor::visit(const FunctionDef& node) {
    visitPre(node);
    for (const auto& stmt : node.body()) {
        stmt->accept(*this);
    }
    visitPost(node);
}

inline void RecursiveASTVisitor::visit(const MacroDef& node) {
    visitPre(node);
    for (const auto& stmt : node.body()) {
        stmt->accept(*this);
    }
    visitPost(node);
}

inline void RecursiveASTVisitor::visit(const IfStatement& node) {
    visitPre(node);
    node.condition()->accept(*this);
    for (const auto& stmt : node.then_branch()) {
        stmt->accept(*this);
    }
    for (const auto& stmt : node.elseif_branches()) {
        stmt->accept(*this);
    }
    for (const auto& stmt : node.else_branch()) {
        stmt->accept(*this);
    }
    visitPost(node);
}

inline void RecursiveASTVisitor::visit(const WhileStatement& node) {
    visitPre(node);
    node.condition()->accept(*this);
    for (const auto& stmt : node.body()) {
        stmt->accept(*this);
    }
    visitPost(node);
}

inline void RecursiveASTVisitor::visit(const ForEachStatement& node) {
    visitPre(node);
    for (const auto& item : node.items()) {
        item->accept(*this);
    }
    for (const auto& stmt : node.body()) {
        stmt->accept(*this);
    }
    visitPost(node);
}

inline void RecursiveASTVisitor::visit(const ListExpression& node) {
    visitPre(node);
    for (const auto& elem : node.elements()) {
        elem->accept(*this);
    }
    visitPost(node);
}

inline void RecursiveASTVisitor::visit(const BracketExpression& node) {
    visitPre(node);
    node.content()->accept(*this);
    visitPost(node);
}

inline void RecursiveASTVisitor::visit(const BinaryOp& node) {
    visitPre(node);
    node.left()->accept(*this);
    node.right()->accept(*this);
    visitPost(node);
}

inline void RecursiveASTVisitor::visit(const UnaryOp& node) {
    visitPre(node);
    node.operand()->accept(*this);
    visitPost(node);
}

inline void RecursiveASTVisitor::visit(const FunctionCall& node) {
    visitPre(node);
    for (const auto& arg : node.arguments()) {
        arg->accept(*this);
    }
    visitPost(node);
}

inline void RecursiveASTVisitor::visit(const Block& node) {
    visitPre(node);
    for (const auto& stmt : node.statements()) {
        stmt->accept(*this);
    }
    visitPost(node);
}

inline void RecursiveASTVisitor::visit(const File& node) {
    visitPre(node);
    for (const auto& stmt : node.statements()) {
        stmt->accept(*this);
    }
    visitPost(node);
}

} // namespace finch::ast
