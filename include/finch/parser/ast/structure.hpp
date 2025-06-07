#pragma once

#include "node.hpp"
#include <fmt/format.h>
#include <string>
#include <string_view>
#include <vector>

namespace finch::ast {

/// Block node - represents a sequence of statements
class Block : public ASTNode {
  private:
    ASTNodeList statements_;

  public:
    Block(SourceLocation loc, ASTNodeList statements)
        : ASTNode(std::move(loc)), statements_(std::move(statements)) {}

    explicit Block(SourceLocation loc) : ASTNode(std::move(loc)) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::Block;
    }

    [[nodiscard]] const ASTNodeList& statements() const {
        return statements_;
    }

    void add_statement(ASTNodePtr stmt) {
        statements_.push_back(std::move(stmt));
    }

    [[nodiscard]] size_t size() const {
        return statements_.size();
    }

    [[nodiscard]] bool empty() const {
        return statements_.empty();
    }

    [[nodiscard]] std::string to_string() const override {
        return fmt::format("<Block[{}]>", statements_.size());
    }

    [[nodiscard]] std::string pretty_print(size_t indent) const override {
        std::string result;
        for (const auto& stmt : statements_) {
            result += stmt->pretty_print(indent) + "\n";
        }
        return result;
    }
};

/// File node - represents an entire CMake file
class File : public ASTNode {
  private:
    std::string_view path_; // Interned file path
    ASTNodeList statements_;
    std::optional<std::string_view> content_hash_; // For caching

  public:
    File(SourceLocation loc, std::string_view path, ASTNodeList statements)
        : ASTNode(std::move(loc)), path_(path), statements_(std::move(statements)) {}

    File(SourceLocation loc, std::string_view path) : ASTNode(std::move(loc)), path_(path) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::File;
    }

    [[nodiscard]] std::string_view path() const {
        return path_;
    }

    [[nodiscard]] const ASTNodeList& statements() const {
        return statements_;
    }

    void add_statement(ASTNodePtr stmt) {
        statements_.push_back(std::move(stmt));
    }

    void set_content_hash(std::string_view hash) {
        content_hash_ = hash;
    }

    [[nodiscard]] std::optional<std::string_view> content_hash() const {
        return content_hash_;
    }

    [[nodiscard]] size_t size() const {
        return statements_.size();
    }

    [[nodiscard]] bool empty() const {
        return statements_.empty();
    }

    [[nodiscard]] std::string to_string() const override {
        return fmt::format("<File: {}>", path_);
    }

    [[nodiscard]] std::string pretty_print(size_t indent) const override {
        std::string result = std::string(indent, ' ') + fmt::format("# File: {}\n", path_);
        for (const auto& stmt : statements_) {
            result += stmt->pretty_print(indent) + "\n";
        }
        return result;
    }
};

/// Include representation for single-file AST (not resolved)
class Include : public ASTNode {
  private:
    std::string_view path_;   // Path to include (may contain variables)
    bool is_optional_;        // OPTIONAL keyword
    bool is_no_policy_scope_; // NO_POLICY_SCOPE keyword
    bool is_result_variable_; // RESULT_VARIABLE <var>
    std::optional<std::string_view> result_var_;

  public:
    Include(SourceLocation loc, std::string_view path, bool is_optional = false)
        : ASTNode(std::move(loc)), path_(path), is_optional_(is_optional),
          is_no_policy_scope_(false), is_result_variable_(false) {}

    void accept(ASTVisitor& visitor) const override;
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::ErrorNode;
    }

    [[nodiscard]] std::string_view path() const {
        return path_;
    }

    [[nodiscard]] bool is_optional() const {
        return is_optional_;
    }

    [[nodiscard]] bool is_no_policy_scope() const {
        return is_no_policy_scope_;
    }

    void set_no_policy_scope(bool value) {
        is_no_policy_scope_ = value;
    }

    void set_result_variable(std::string_view var) {
        is_result_variable_ = true;
        result_var_ = var;
    }

    [[nodiscard]] std::optional<std::string_view> result_variable() const {
        return result_var_;
    }

    [[nodiscard]] std::string to_string() const override {
        std::string result = "include(";
        if (is_optional_)
            result += "OPTIONAL ";
        if (is_no_policy_scope_)
            result += "NO_POLICY_SCOPE ";
        result += path_;
        if (result_var_) {
            result += fmt::format(" RESULT_VARIABLE {}", *result_var_);
        }
        result += ")";
        return result;
    }
};

} // namespace finch::ast
