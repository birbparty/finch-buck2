#pragma once

#include "node.hpp"
#include <fmt/format.h>
#include <string>
#include <string_view>
#include <vector>

namespace finch::ast {

/// Generic command invocation (e.g., add_library, set, etc.)
class CommandCall : public ASTNode {
  private:
    std::string_view name_; // Interned command name
    ASTNodeList arguments_;

  public:
    CommandCall(SourceLocation loc, std::string_view name, ASTNodeList args)
        : ASTNode(std::move(loc)), name_(name), arguments_(std::move(args)) {}

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::CommandCall;
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

    [[nodiscard]] const ASTNode* argument(size_t index) const {
        return index < arguments_.size() ? arguments_[index].get() : nullptr;
    }

    [[nodiscard]] std::string to_string() const override {
        std::string result = fmt::format("{}(", name_);
        for (size_t i = 0; i < arguments_.size(); ++i) {
            if (i > 0)
                result += " ";
            result += arguments_[i]->to_string();
        }
        result += ")";
        return result;
    }

    [[nodiscard]] std::string pretty_print(size_t indent) const override {
        std::string result = std::string(indent, ' ') + fmt::format("{}(\n", name_);
        for (const auto& arg : arguments_) {
            result += arg->pretty_print(indent + 2) + "\n";
        }
        result += std::string(indent, ' ') + ")";
        return result;
    }

    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        ASTNodeList cloned_args;
        cloned_args.reserve(arguments_.size());
        for (const auto& arg : arguments_) {
            cloned_args.push_back(arg->clone());
        }
        return std::make_unique<CommandCall>(location(), name_, std::move(cloned_args));
    }
};

/// Function definition
class FunctionDef : public ASTNode {
  private:
    std::string_view name_;                    // Interned function name
    std::vector<std::string_view> parameters_; // Interned parameter names
    ASTNodeList body_;

  public:
    FunctionDef(SourceLocation loc, std::string_view name, std::vector<std::string_view> params,
                ASTNodeList body)
        : ASTNode(std::move(loc)), name_(name), parameters_(std::move(params)),
          body_(std::move(body)) {}

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::FunctionDef;
    }

    [[nodiscard]] std::string_view name() const {
        return name_;
    }

    [[nodiscard]] const std::vector<std::string_view>& parameters() const {
        return parameters_;
    }

    [[nodiscard]] const ASTNodeList& body() const {
        return body_;
    }

    [[nodiscard]] size_t parameter_count() const {
        return parameters_.size();
    }

    [[nodiscard]] std::string to_string() const override {
        std::string result = fmt::format("function {}(", name_);
        for (size_t i = 0; i < parameters_.size(); ++i) {
            if (i > 0)
                result += " ";
            result += parameters_[i];
        }
        result += ")";
        return result;
    }

    [[nodiscard]] std::string pretty_print(size_t indent) const override {
        std::string result = std::string(indent, ' ') + fmt::format("function {}(", name_);
        for (size_t i = 0; i < parameters_.size(); ++i) {
            if (i > 0)
                result += " ";
            result += parameters_[i];
        }
        result += ")\n";

        for (const auto& stmt : body_) {
            result += stmt->pretty_print(indent + 2) + "\n";
        }

        result += std::string(indent, ' ') + "endfunction()";
        return result;
    }

    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        ASTNodeList cloned_body;
        cloned_body.reserve(body_.size());
        for (const auto& stmt : body_) {
            cloned_body.push_back(stmt->clone());
        }
        return std::make_unique<FunctionDef>(location(), name_, parameters_,
                                             std::move(cloned_body));
    }
};

/// Macro definition (kept unexpanded for performance)
class MacroDef : public ASTNode {
  private:
    std::string_view name_;                    // Interned macro name
    std::vector<std::string_view> parameters_; // Interned parameter names
    ASTNodeList body_;

  public:
    MacroDef(SourceLocation loc, std::string_view name, std::vector<std::string_view> params,
             ASTNodeList body)
        : ASTNode(std::move(loc)), name_(name), parameters_(std::move(params)),
          body_(std::move(body)) {}

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::MacroDef;
    }

    [[nodiscard]] std::string_view name() const {
        return name_;
    }

    [[nodiscard]] const std::vector<std::string_view>& parameters() const {
        return parameters_;
    }

    [[nodiscard]] const ASTNodeList& body() const {
        return body_;
    }

    [[nodiscard]] size_t parameter_count() const {
        return parameters_.size();
    }

    [[nodiscard]] std::string to_string() const override {
        std::string result = fmt::format("macro {}(", name_);
        for (size_t i = 0; i < parameters_.size(); ++i) {
            if (i > 0)
                result += " ";
            result += parameters_[i];
        }
        result += ")";
        return result;
    }

    [[nodiscard]] std::string pretty_print(size_t indent) const override {
        std::string result = std::string(indent, ' ') + fmt::format("macro {}(", name_);
        for (size_t i = 0; i < parameters_.size(); ++i) {
            if (i > 0)
                result += " ";
            result += parameters_[i];
        }
        result += ")\n";

        for (const auto& stmt : body_) {
            result += stmt->pretty_print(indent + 2) + "\n";
        }

        result += std::string(indent, ' ') + "endmacro()";
        return result;
    }

    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        ASTNodeList cloned_body;
        cloned_body.reserve(body_.size());
        for (const auto& stmt : body_) {
            cloned_body.push_back(stmt->clone());
        }
        return std::make_unique<MacroDef>(location(), name_, parameters_, std::move(cloned_body));
    }
};

} // namespace finch::ast
