#pragma once

#include <finch/core/error.hpp>
#include <fmt/format.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace finch::ast {

// Forward declarations
class ASTVisitor;

/// Node type enumeration for runtime type identification
enum class NodeType {
    // Literals
    StringLiteral,
    NumberLiteral,
    BooleanLiteral,

    // Identifiers
    Identifier,
    Variable,

    // Commands
    CommandCall,
    FunctionDef,
    MacroDef,

    // Control flow
    IfStatement,
    ElseIfStatement,
    ElseStatement,
    WhileStatement,
    ForEachStatement,

    // Expressions
    BinaryOp,
    UnaryOp,
    FunctionCall,
    ListExpression,
    GeneratorExpression,
    BracketExpression,

    // Structure
    Block,
    File,

    // CPM commands
    CPMAddPackage,
    CPMFindPackage,
    CPMUsePackageLock,
    CPMDeclarePackage,

    // Error recovery
    ErrorNode
};

/// Base AST node interface
class ASTNode {
  protected:
    SourceLocation location_;
    bool is_error_ = false; // For error recovery

  public:
    explicit ASTNode(SourceLocation location) : location_(std::move(location)) {}

    virtual ~ASTNode() = default;

    // Disable copy to ensure immutability
    ASTNode(const ASTNode&) = delete;
    ASTNode& operator=(const ASTNode&) = delete;

    // Allow move for construction
    ASTNode(ASTNode&&) = default;
    ASTNode& operator=(ASTNode&&) = default;

    /// Visitor pattern
    virtual void accept(ASTVisitor& visitor) const = 0;

    /// Node type query
    [[nodiscard]] virtual NodeType type() const = 0;

    /// Location access
    [[nodiscard]] const SourceLocation& location() const {
        return location_;
    }

    /// Check if this node represents a parse error
    [[nodiscard]] bool is_error() const {
        return is_error_;
    }

    /// Mark node as error (for error recovery)
    void mark_as_error() {
        is_error_ = true;
    }

    /// Debugging
    [[nodiscard]] virtual std::string to_string() const = 0;

    /// Pretty printing with indentation
    [[nodiscard]] virtual std::string pretty_print(size_t indent = 0) const {
        return std::string(indent, ' ') + to_string();
    }

    /// Clone the node (deep copy)
    [[nodiscard]] virtual std::unique_ptr<ASTNode> clone() const = 0;
};

using ASTNodePtr = std::unique_ptr<ASTNode>;
using ASTNodeList = std::vector<ASTNodePtr>;

/// String interning for performance
class StringInterner {
  private:
    std::vector<std::unique_ptr<std::string>> strings_;
    std::unordered_map<std::string_view, size_t> index_;

  public:
    /// Intern a string and return a view to it
    [[nodiscard]] std::string_view intern(std::string_view str) {
        auto it = index_.find(str);
        if (it != index_.end()) {
            return *strings_[it->second];
        }

        auto owned = std::make_unique<std::string>(str);
        std::string_view view = *owned;
        size_t idx = strings_.size();
        strings_.push_back(std::move(owned));
        index_[view] = idx;
        return view;
    }

    /// Get statistics
    [[nodiscard]] size_t unique_strings() const {
        return strings_.size();
    }
    [[nodiscard]] size_t total_lookups() const {
        return index_.size();
    }
};

/// Error node for error recovery
class ErrorNode : public ASTNode {
  private:
    std::string message_;
    ParseError::Category category_;

  public:
    ErrorNode(SourceLocation loc, std::string message, ParseError::Category category)
        : ASTNode(std::move(loc)), message_(std::move(message)), category_(category) {
        mark_as_error();
    }

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::ErrorNode;
    }

    [[nodiscard]] const std::string& message() const {
        return message_;
    }
    [[nodiscard]] ParseError::Category category() const {
        return category_;
    }

    [[nodiscard]] std::string to_string() const override {
        return fmt::format("<Error: {}>", message_);
    }

    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<ErrorNode>(location_, message_, category_);
    }
};

} // namespace finch::ast
