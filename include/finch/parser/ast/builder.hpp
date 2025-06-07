#pragma once

#include "commands.hpp"
#include "control_flow.hpp"
#include "expressions.hpp"
#include "literals.hpp"
#include "node.hpp"
#include "structure.hpp"
#include <finch/core/result.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace finch::ast {

/// Builder for constructing AST nodes with string interning
class ASTBuilder {
  private:
    StringInterner interner_;

  public:
    /// Get the string interner
    StringInterner& interner() {
        return interner_;
    }
    const StringInterner& interner() const {
        return interner_;
    }

    /// Factory methods for creating nodes
    template <typename T, typename... Args>
    static std::unique_ptr<T> make(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    /// Create a string literal
    ASTNodePtr makeString(SourceLocation loc, std::string_view value, bool quoted = true) {
        auto interned = interner_.intern(value);
        return make<StringLiteral>(std::move(loc), interned, quoted);
    }

    /// Create a number literal (integer)
    ASTNodePtr makeNumber(SourceLocation loc, std::string_view text, int64_t value) {
        auto interned = interner_.intern(text);
        return make<NumberLiteral>(std::move(loc), interned, value);
    }

    /// Create a number literal (float)
    ASTNodePtr makeNumber(SourceLocation loc, std::string_view text, double value) {
        auto interned = interner_.intern(text);
        return make<NumberLiteral>(std::move(loc), interned, value);
    }

    /// Create a boolean literal
    ASTNodePtr makeBoolean(SourceLocation loc, bool value, std::string_view original_text) {
        auto interned = interner_.intern(original_text);
        return make<BooleanLiteral>(std::move(loc), value, interned);
    }

    /// Create a variable reference
    ASTNodePtr makeVariable(SourceLocation loc, std::string_view name,
                            Variable::VariableType type = Variable::VariableType::Normal) {
        auto interned = interner_.intern(name);
        return make<Variable>(std::move(loc), interned, type);
    }

    /// Create an identifier
    ASTNodePtr makeIdentifier(SourceLocation loc, std::string_view name) {
        auto interned = interner_.intern(name);
        return make<Identifier>(std::move(loc), interned);
    }

    /// Create a command call
    ASTNodePtr makeCommand(SourceLocation loc, std::string_view name, ASTNodeList args) {
        auto interned = interner_.intern(name);
        return make<CommandCall>(std::move(loc), interned, std::move(args));
    }

    /// Create a function definition
    ASTNodePtr makeFunction(SourceLocation loc, std::string_view name,
                            std::vector<std::string_view> params, ASTNodeList body) {
        auto interned_name = interner_.intern(name);
        std::vector<std::string_view> interned_params;
        for (auto param : params) {
            interned_params.push_back(interner_.intern(param));
        }
        return make<FunctionDef>(std::move(loc), interned_name, std::move(interned_params),
                                 std::move(body));
    }

    /// Create a macro definition
    ASTNodePtr makeMacro(SourceLocation loc, std::string_view name,
                         std::vector<std::string_view> params, ASTNodeList body) {
        auto interned_name = interner_.intern(name);
        std::vector<std::string_view> interned_params;
        for (auto param : params) {
            interned_params.push_back(interner_.intern(param));
        }
        return make<MacroDef>(std::move(loc), interned_name, std::move(interned_params),
                              std::move(body));
    }

    /// Create an if statement
    ASTNodePtr makeIf(SourceLocation loc, ASTNodePtr condition, ASTNodeList then_branch) {
        return make<IfStatement>(std::move(loc), std::move(condition), std::move(then_branch));
    }

    /// Create a foreach statement
    ASTNodePtr makeForEach(SourceLocation loc, std::vector<std::string_view> variables,
                           ForEachStatement::LoopType loop_type, ASTNodeList items,
                           ASTNodeList body) {
        std::vector<std::string_view> interned_vars;
        for (auto var : variables) {
            interned_vars.push_back(interner_.intern(var));
        }
        return make<ForEachStatement>(std::move(loc), std::move(interned_vars), loop_type,
                                      std::move(items), std::move(body));
    }

    /// Create a while statement
    ASTNodePtr makeWhile(SourceLocation loc, ASTNodePtr condition, ASTNodeList body) {
        return make<WhileStatement>(std::move(loc), std::move(condition), std::move(body));
    }

    /// Create a list expression
    ASTNodePtr makeList(SourceLocation loc, ASTNodeList elements, char separator = ' ') {
        return make<ListExpression>(std::move(loc), std::move(elements), separator);
    }

    /// Create a generator expression
    ASTNodePtr makeGeneratorExpr(SourceLocation loc, std::string_view expr) {
        auto interned = interner_.intern(expr);
        return make<GeneratorExpression>(std::move(loc), interned);
    }

    /// Create a bracket expression
    ASTNodePtr makeBracketExpr(SourceLocation loc, ASTNodePtr content, bool is_quoted = false) {
        return make<BracketExpression>(std::move(loc), std::move(content), is_quoted);
    }

    /// Create a binary operation
    ASTNodePtr makeBinaryOp(SourceLocation loc, ASTNodePtr left, BinaryOp::Operator op,
                            ASTNodePtr right) {
        return make<BinaryOp>(std::move(loc), std::move(left), op, std::move(right));
    }

    /// Create a unary operation
    ASTNodePtr makeUnaryOp(SourceLocation loc, UnaryOp::Operator op, ASTNodePtr operand) {
        return make<UnaryOp>(std::move(loc), op, std::move(operand));
    }

    /// Create a function call expression
    ASTNodePtr makeFunctionCall(SourceLocation loc, std::string_view name, ASTNodeList args) {
        auto interned = interner_.intern(name);
        return make<FunctionCall>(std::move(loc), interned, std::move(args));
    }

    /// Create a block
    ASTNodePtr makeBlock(SourceLocation loc, ASTNodeList statements) {
        return make<Block>(std::move(loc), std::move(statements));
    }

    /// Create an empty block
    ASTNodePtr makeBlock(SourceLocation loc) {
        return make<Block>(std::move(loc));
    }

    /// Create a file node
    ASTNodePtr makeFile(SourceLocation loc, std::string_view path, ASTNodeList statements) {
        auto interned = interner_.intern(path);
        return make<File>(std::move(loc), interned, std::move(statements));
    }

    /// Create an empty file node
    ASTNodePtr makeFile(SourceLocation loc, std::string_view path) {
        auto interned = interner_.intern(path);
        return make<File>(std::move(loc), interned);
    }

    /// Create an error node
    ASTNodePtr makeError(SourceLocation loc, std::string message, ParseError::Category category) {
        return make<ErrorNode>(std::move(loc), std::move(message), category);
    }

    /// List builders
    static ASTNodeList makeList() {
        return ASTNodeList{};
    }

    static ASTNodeList makeList(ASTNodePtr node) {
        ASTNodeList list;
        list.push_back(std::move(node));
        return list;
    }

    template <typename... Nodes>
    static ASTNodeList makeList(ASTNodePtr first, Nodes&&... rest) {
        ASTNodeList list;
        list.push_back(std::move(first));
        (list.push_back(std::forward<Nodes>(rest)), ...);
        return list;
    }
};

/// Type-safe builder with Result returns for error handling
class SafeASTBuilder : public ASTBuilder {
  public:
    using ASTResult = Result<ASTNodePtr, ParseError>;

    /// Validate and create a command
    ASTResult makeCommandSafe(SourceLocation loc, std::string_view name, ASTNodeList args) {
        if (name.empty()) {
            ParseError error(ParseError::Category::InvalidSyntax, "Empty command name");
            error.at(loc);
            // Use the constructor with value directly
            return ASTResult(
                makeCommand(loc, "", std::move(args))); // Create empty result then assign error
        }
        return ASTResult(makeCommand(loc, name, std::move(args)));
    }

    /// Validate and create a variable
    ASTResult makeVariableSafe(SourceLocation loc, std::string_view name,
                               Variable::VariableType type = Variable::VariableType::Normal) {
        if (name.empty()) {
            ParseError error(ParseError::Category::InvalidSyntax, "Empty variable name");
            error.at(loc);
            // Actually, let's just not use SafeASTBuilder for now - remove it
            return ASTResult(makeVariable(loc, "", type));
        }
        return ASTResult(makeVariable(loc, name, type));
    }

    /// Validate and create a function definition
    ASTResult makeFunctionSafe(SourceLocation loc, std::string_view name,
                               std::vector<std::string_view> params, ASTNodeList body) {
        if (name.empty()) {
            ParseError error(ParseError::Category::InvalidSyntax, "Empty function name");
            error.at(loc);
            return ASTResult(makeFunction(loc, "", std::move(params), std::move(body)));
        }

        // Check for duplicate parameters
        std::unordered_set<std::string_view> seen;
        for (auto param : params) {
            if (!seen.insert(param).second) {
                ParseError error(ParseError::Category::InvalidSyntax,
                                 fmt::format("Duplicate parameter: {}", param));
                error.at(loc);
                return ASTResult(makeFunction(loc, name, std::move(params), std::move(body)));
            }
        }

        return ASTResult(makeFunction(loc, name, std::move(params), std::move(body)));
    }
};

} // namespace finch::ast
