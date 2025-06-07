#pragma once

#include "node.hpp"
#include <fmt/format.h>
#include <optional>
#include <string>
#include <string_view>

namespace finch::ast {

/// String literal node
class StringLiteral : public ASTNode {
  private:
    std::string_view value_; // Interned string
    bool quoted_;            // Track if originally quoted

  public:
    StringLiteral(SourceLocation loc, std::string_view value, bool quoted)
        : ASTNode(std::move(loc)), value_(value), quoted_(quoted) {}

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::StringLiteral;
    }

    [[nodiscard]] std::string_view value() const {
        return value_;
    }

    [[nodiscard]] bool is_quoted() const {
        return quoted_;
    }

    [[nodiscard]] std::string to_string() const override {
        return quoted_ ? fmt::format("\"{}\"", value_) : std::string(value_);
    }

    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<StringLiteral>(location_, value_, quoted_);
    }
};

/// Number literal node (handles both int and float)
class NumberLiteral : public ASTNode {
  public:
    enum class NumberType { Integer, Float };

  private:
    std::string_view text_; // Original text representation (interned)
    union {
        int64_t int_value_;
        double float_value_;
    };
    NumberType number_type_;

  public:
    NumberLiteral(SourceLocation loc, std::string_view text, int64_t value)
        : ASTNode(std::move(loc)), text_(text), int_value_(value),
          number_type_(NumberType::Integer) {}

    NumberLiteral(SourceLocation loc, std::string_view text, double value)
        : ASTNode(std::move(loc)), text_(text), float_value_(value),
          number_type_(NumberType::Float) {}

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::NumberLiteral;
    }

    [[nodiscard]] NumberType number_type() const {
        return number_type_;
    }

    [[nodiscard]] int64_t as_int() const {
        return number_type_ == NumberType::Integer ? int_value_
                                                   : static_cast<int64_t>(float_value_);
    }

    [[nodiscard]] double as_float() const {
        return number_type_ == NumberType::Float ? float_value_ : static_cast<double>(int_value_);
    }

    [[nodiscard]] std::string_view text() const {
        return text_;
    }

    [[nodiscard]] std::string to_string() const override {
        return std::string(text_);
    }

    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        if (number_type_ == NumberType::Integer) {
            return std::make_unique<NumberLiteral>(location_, text_, int_value_);
        } else {
            return std::make_unique<NumberLiteral>(location_, text_, float_value_);
        }
    }
};

/// Boolean literal node
class BooleanLiteral : public ASTNode {
  private:
    bool value_;
    std::string_view original_text_; // TRUE/true/ON/on/YES/yes or FALSE/false/OFF/off/NO/no

  public:
    BooleanLiteral(SourceLocation loc, bool value, std::string_view original_text)
        : ASTNode(std::move(loc)), value_(value), original_text_(original_text) {}

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::BooleanLiteral;
    }

    [[nodiscard]] bool value() const {
        return value_;
    }

    [[nodiscard]] std::string_view original_text() const {
        return original_text_;
    }

    [[nodiscard]] std::string to_string() const override {
        return std::string(original_text_);
    }

    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<BooleanLiteral>(location_, value_, original_text_);
    }
};

/// Variable reference ${VAR} or $ENV{VAR} or $CACHE{VAR}
class Variable : public ASTNode {
  public:
    enum class VariableType { Normal, Environment, Cache };

  private:
    std::string_view name_; // Interned variable name
    VariableType var_type_;

  public:
    Variable(SourceLocation loc, std::string_view name,
             VariableType var_type = VariableType::Normal)
        : ASTNode(std::move(loc)), name_(name), var_type_(var_type) {}

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::Variable;
    }

    [[nodiscard]] std::string_view name() const {
        return name_;
    }

    [[nodiscard]] VariableType variable_type() const {
        return var_type_;
    }

    [[nodiscard]] bool is_env() const {
        return var_type_ == VariableType::Environment;
    }

    [[nodiscard]] bool is_cache() const {
        return var_type_ == VariableType::Cache;
    }

    [[nodiscard]] std::string to_string() const override {
        switch (var_type_) {
        case VariableType::Normal:
            return fmt::format("${{{}}}", name_);
        case VariableType::Environment:
            return fmt::format("$ENV{{{}}}", name_);
        case VariableType::Cache:
            return fmt::format("$CACHE{{{}}}", name_);
        }
        return ""; // Unreachable
    }

    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<Variable>(location_, name_, var_type_);
    }
};

/// Identifier node (command names, function names, etc.)
class Identifier : public ASTNode {
  private:
    std::string_view name_; // Interned identifier

  public:
    Identifier(SourceLocation loc, std::string_view name) : ASTNode(std::move(loc)), name_(name) {}

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::Identifier;
    }

    [[nodiscard]] std::string_view name() const {
        return name_;
    }

    [[nodiscard]] std::string to_string() const override {
        return std::string(name_);
    }

    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<Identifier>(location_, name_);
    }
};

} // namespace finch::ast
