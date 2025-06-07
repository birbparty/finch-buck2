#pragma once

#include "node.hpp"
#include <fmt/format.h>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace finch::ast {

/// If statement
class IfStatement : public ASTNode {
  private:
    ASTNodePtr condition_;
    ASTNodeList then_branch_;
    ASTNodeList elseif_branches_; // Pairs of condition and body
    ASTNodeList else_branch_;     // May be empty

  public:
    IfStatement(SourceLocation loc, ASTNodePtr condition, ASTNodeList then_branch)
        : ASTNode(std::move(loc)), condition_(std::move(condition)),
          then_branch_(std::move(then_branch)) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::IfStatement;
    }

    [[nodiscard]] const ASTNode* condition() const {
        return condition_.get();
    }

    [[nodiscard]] const ASTNodeList& then_branch() const {
        return then_branch_;
    }

    [[nodiscard]] const ASTNodeList& elseif_branches() const {
        return elseif_branches_;
    }

    [[nodiscard]] const ASTNodeList& else_branch() const {
        return else_branch_;
    }

    void add_elseif(ASTNodePtr condition, ASTNodeList body) {
        elseif_branches_.push_back(std::move(condition));
        // Store body elements after condition
        for (auto& stmt : body) {
            elseif_branches_.push_back(std::move(stmt));
        }
    }

    void set_else_branch(ASTNodeList else_body) {
        else_branch_ = std::move(else_body);
    }

    [[nodiscard]] std::string to_string() const override {
        return fmt::format("if({})", condition_->to_string());
    }

    [[nodiscard]] std::string pretty_print(size_t indent) const override {
        std::string result =
            std::string(indent, ' ') + fmt::format("if({})\n", condition_->to_string());

        for (const auto& stmt : then_branch_) {
            result += stmt->pretty_print(indent + 2) + "\n";
        }

        // Handle elseif branches (stored as condition, body, condition, body...)
        for (size_t i = 0; i < elseif_branches_.size();) {
            if (i + 1 < elseif_branches_.size()) {
                result += std::string(indent, ' ') +
                          fmt::format("elseif({})\n", elseif_branches_[i]->to_string());
                i++; // Skip condition

                // Print body until next condition or end
                while (i < elseif_branches_.size() &&
                       elseif_branches_[i]->type() != NodeType::ElseIfStatement) {
                    result += elseif_branches_[i]->pretty_print(indent + 2) + "\n";
                    i++;
                }
            } else {
                break;
            }
        }

        if (!else_branch_.empty()) {
            result += std::string(indent, ' ') + "else()\n";
            for (const auto& stmt : else_branch_) {
                result += stmt->pretty_print(indent + 2) + "\n";
            }
        }

        result += std::string(indent, ' ') + "endif()";
        return result;
    }
};

/// ForEach loop
class ForEachStatement : public ASTNode {
  public:
    enum class LoopType {
        IN_LISTS,    // foreach(var IN LISTS list1 list2 ...)
        IN_ITEMS,    // foreach(var IN ITEMS item1 item2 ...)
        IN,          // foreach(var IN item1 item2 ...)
        RANGE,       // foreach(var RANGE start stop [step])
        IN_ZIP_LISTS // foreach(var1 var2 IN ZIP_LISTS list1 list2)
    };

  private:
    std::vector<std::string_view> variables_; // Loop variables (interned)
    LoopType loop_type_;
    ASTNodeList items_; // Lists, items, or range parameters
    ASTNodeList body_;

  public:
    ForEachStatement(SourceLocation loc, std::vector<std::string_view> variables,
                     LoopType loop_type, ASTNodeList items, ASTNodeList body)
        : ASTNode(std::move(loc)), variables_(std::move(variables)), loop_type_(loop_type),
          items_(std::move(items)), body_(std::move(body)) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::ForEachStatement;
    }

    [[nodiscard]] const std::vector<std::string_view>& variables() const {
        return variables_;
    }

    [[nodiscard]] LoopType loop_type() const {
        return loop_type_;
    }

    [[nodiscard]] const ASTNodeList& items() const {
        return items_;
    }

    [[nodiscard]] const ASTNodeList& body() const {
        return body_;
    }

    [[nodiscard]] std::string loop_type_string() const {
        switch (loop_type_) {
        case LoopType::IN_LISTS:
            return "IN LISTS";
        case LoopType::IN_ITEMS:
            return "IN ITEMS";
        case LoopType::IN:
            return "IN";
        case LoopType::RANGE:
            return "RANGE";
        case LoopType::IN_ZIP_LISTS:
            return "IN ZIP_LISTS";
        }
        return "";
    }

    [[nodiscard]] std::string to_string() const override {
        std::string result = "foreach(";
        for (size_t i = 0; i < variables_.size(); ++i) {
            if (i > 0)
                result += " ";
            result += variables_[i];
        }
        result += fmt::format(" {})", loop_type_string());
        return result;
    }

    [[nodiscard]] std::string pretty_print(size_t indent) const override {
        std::string result = std::string(indent, ' ') + "foreach(";
        for (size_t i = 0; i < variables_.size(); ++i) {
            if (i > 0)
                result += " ";
            result += variables_[i];
        }
        result += fmt::format(" {}", loop_type_string());

        for (const auto& item : items_) {
            result += " " + item->to_string();
        }
        result += ")\n";

        for (const auto& stmt : body_) {
            result += stmt->pretty_print(indent + 2) + "\n";
        }

        result += std::string(indent, ' ') + "endforeach()";
        return result;
    }
};

/// While loop
class WhileStatement : public ASTNode {
  private:
    ASTNodePtr condition_;
    ASTNodeList body_;

  public:
    WhileStatement(SourceLocation loc, ASTNodePtr condition, ASTNodeList body)
        : ASTNode(std::move(loc)), condition_(std::move(condition)), body_(std::move(body)) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::WhileStatement;
    }

    [[nodiscard]] const ASTNode* condition() const {
        return condition_.get();
    }

    [[nodiscard]] const ASTNodeList& body() const {
        return body_;
    }

    [[nodiscard]] std::string to_string() const override {
        return fmt::format("while({})", condition_->to_string());
    }

    [[nodiscard]] std::string pretty_print(size_t indent) const override {
        std::string result =
            std::string(indent, ' ') + fmt::format("while({})\n", condition_->to_string());

        for (const auto& stmt : body_) {
            result += stmt->pretty_print(indent + 2) + "\n";
        }

        result += std::string(indent, ' ') + "endwhile()";
        return result;
    }
};

/// ElseIf statement (for better AST structure)
class ElseIfStatement : public ASTNode {
  private:
    ASTNodePtr condition_;

  public:
    ElseIfStatement(SourceLocation loc, ASTNodePtr condition)
        : ASTNode(std::move(loc)), condition_(std::move(condition)) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::ElseIfStatement;
    }

    [[nodiscard]] const ASTNode* condition() const {
        return condition_.get();
    }

    [[nodiscard]] std::string to_string() const override {
        return fmt::format("elseif({})", condition_->to_string());
    }
};

/// Else statement (for better AST structure)
class ElseStatement : public ASTNode {
  public:
    explicit ElseStatement(SourceLocation loc) : ASTNode(std::move(loc)) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::ElseStatement;
    }

    [[nodiscard]] std::string to_string() const override {
        return "else()";
    }
};

} // namespace finch::ast
