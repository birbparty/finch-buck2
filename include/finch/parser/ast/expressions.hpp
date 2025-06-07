#pragma once

#include "node.hpp"
#include <fmt/format.h>
#include <string>
#include <string_view>
#include <vector>

namespace finch::ast {

/// List expression (space or semicolon separated)
class ListExpression : public ASTNode {
  private:
    ASTNodeList elements_;
    char separator_; // ' ' or ';'

  public:
    ListExpression(SourceLocation loc, ASTNodeList elements, char separator = ' ')
        : ASTNode(std::move(loc)), elements_(std::move(elements)), separator_(separator) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::ListExpression;
    }

    [[nodiscard]] const ASTNodeList& elements() const {
        return elements_;
    }

    [[nodiscard]] char separator() const {
        return separator_;
    }

    [[nodiscard]] size_t size() const {
        return elements_.size();
    }

    [[nodiscard]] bool empty() const {
        return elements_.empty();
    }

    [[nodiscard]] std::string to_string() const override {
        std::string result;
        for (size_t i = 0; i < elements_.size(); ++i) {
            if (i > 0)
                result += separator_;
            result += elements_[i]->to_string();
        }
        return result;
    }
};

/// Generator expression $<...> (kept as opaque string for performance)
class GeneratorExpression : public ASTNode {
  private:
    std::string_view expression_; // Interned expression content

  public:
    GeneratorExpression(SourceLocation loc, std::string_view expr)
        : ASTNode(std::move(loc)), expression_(expr) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::GeneratorExpression;
    }

    [[nodiscard]] std::string_view expression() const {
        return expression_;
    }

    [[nodiscard]] std::string to_string() const override {
        return fmt::format("$<{}>", expression_);
    }
};

/// Bracket expression [[...]] for bracket arguments
class BracketExpression : public ASTNode {
  private:
    ASTNodePtr content_; // Content inside brackets
    bool is_quoted_;     // Track if it's in a quoted context

  public:
    BracketExpression(SourceLocation loc, ASTNodePtr content, bool is_quoted = false)
        : ASTNode(std::move(loc)), content_(std::move(content)), is_quoted_(is_quoted) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::BracketExpression;
    }

    [[nodiscard]] const ASTNode* content() const {
        return content_.get();
    }

    [[nodiscard]] bool is_quoted() const {
        return is_quoted_;
    }

    [[nodiscard]] std::string to_string() const override {
        return fmt::format("[[{}]]", content_->to_string());
    }
};

/// Binary operator for expressions
class BinaryOp : public ASTNode {
  public:
    enum class Operator {
        // Logical operators
        AND,
        OR,
        NOT,
        // Comparison operators
        EQUAL,
        NOT_EQUAL,
        LESS,
        LESS_EQUAL,
        GREATER,
        GREATER_EQUAL,
        // String operators
        MATCHES,
        STREQUAL,
        STRLESS,
        STRGREATER,
        // Version comparison
        VERSION_EQUAL,
        VERSION_LESS,
        VERSION_GREATER,
        // Math operators (for math() command)
        ADD,
        SUBTRACT,
        MULTIPLY,
        DIVIDE,
        MOD
    };

  private:
    ASTNodePtr left_;
    ASTNodePtr right_;
    Operator op_;

  public:
    BinaryOp(SourceLocation loc, ASTNodePtr left, Operator op, ASTNodePtr right)
        : ASTNode(std::move(loc)), left_(std::move(left)), right_(std::move(right)), op_(op) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::BinaryOp;
    }

    [[nodiscard]] const ASTNode* left() const {
        return left_.get();
    }

    [[nodiscard]] const ASTNode* right() const {
        return right_.get();
    }

    [[nodiscard]] Operator op() const {
        return op_;
    }

    [[nodiscard]] std::string operator_string() const {
        switch (op_) {
        case Operator::AND:
            return "AND";
        case Operator::OR:
            return "OR";
        case Operator::NOT:
            return "NOT";
        case Operator::EQUAL:
            return "EQUAL";
        case Operator::NOT_EQUAL:
            return "NOT EQUAL";
        case Operator::LESS:
            return "LESS";
        case Operator::LESS_EQUAL:
            return "LESS_EQUAL";
        case Operator::GREATER:
            return "GREATER";
        case Operator::GREATER_EQUAL:
            return "GREATER_EQUAL";
        case Operator::MATCHES:
            return "MATCHES";
        case Operator::STREQUAL:
            return "STREQUAL";
        case Operator::STRLESS:
            return "STRLESS";
        case Operator::STRGREATER:
            return "STRGREATER";
        case Operator::VERSION_EQUAL:
            return "VERSION_EQUAL";
        case Operator::VERSION_LESS:
            return "VERSION_LESS";
        case Operator::VERSION_GREATER:
            return "VERSION_GREATER";
        case Operator::ADD:
            return "+";
        case Operator::SUBTRACT:
            return "-";
        case Operator::MULTIPLY:
            return "*";
        case Operator::DIVIDE:
            return "/";
        case Operator::MOD:
            return "%";
        }
        return "";
    }

    [[nodiscard]] std::string to_string() const override {
        return fmt::format("({} {} {})", left_->to_string(), operator_string(),
                           right_->to_string());
    }
};

/// Unary operator for expressions
class UnaryOp : public ASTNode {
  public:
    enum class Operator {
        NOT,     // Logical NOT
        EXISTS,  // File/directory existence
        DEFINED, // Variable defined
        IS_DIRECTORY,
        IS_ABSOLUTE,
        COMMAND // Command exists
    };

  private:
    ASTNodePtr operand_;
    Operator op_;

  public:
    UnaryOp(SourceLocation loc, Operator op, ASTNodePtr operand)
        : ASTNode(std::move(loc)), operand_(std::move(operand)), op_(op) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::UnaryOp;
    }

    [[nodiscard]] const ASTNode* operand() const {
        return operand_.get();
    }

    [[nodiscard]] Operator op() const {
        return op_;
    }

    [[nodiscard]] std::string operator_string() const {
        switch (op_) {
        case Operator::NOT:
            return "NOT";
        case Operator::EXISTS:
            return "EXISTS";
        case Operator::DEFINED:
            return "DEFINED";
        case Operator::IS_DIRECTORY:
            return "IS_DIRECTORY";
        case Operator::IS_ABSOLUTE:
            return "IS_ABSOLUTE";
        case Operator::COMMAND:
            return "COMMAND";
        }
        return "";
    }

    [[nodiscard]] std::string to_string() const override {
        return fmt::format("({} {})", operator_string(), operand_->to_string());
    }
};

/// Function call expression (for string(), list(), etc.)
class FunctionCall : public ASTNode {
  private:
    std::string_view name_; // Interned function name
    ASTNodeList arguments_;

  public:
    FunctionCall(SourceLocation loc, std::string_view name, ASTNodeList args)
        : ASTNode(std::move(loc)), name_(name), arguments_(std::move(args)) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::FunctionCall;
    }

    [[nodiscard]] std::string_view name() const {
        return name_;
    }

    [[nodiscard]] const ASTNodeList& arguments() const {
        return arguments_;
    }

    [[nodiscard]] size_t argument_count() const {
        return arguments_.size();
    }

    [[nodiscard]] std::string to_string() const override {
        std::string result = fmt::format("{}(", name_);
        for (size_t i = 0; i < arguments_.size(); ++i) {
            if (i > 0)
                result += ", ";
            result += arguments_[i]->to_string();
        }
        result += ")";
        return result;
    }
};

} // namespace finch::ast
