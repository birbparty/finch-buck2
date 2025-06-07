#pragma once

#include <finch/analyzer/evaluation_context.hpp>
#include <finch/analyzer/project_analysis.hpp>
#include <finch/core/error.hpp>
#include <finch/core/result.hpp>
#include <finch/parser/ast/visitor.hpp>
#include <regex>
#include <string>

namespace finch::analyzer {

class CMakeEvaluator : public ast::ASTVisitor {
  private:
    EvaluationContext& context_;
    Result<EvaluatedValue, AnalysisError> result_;

    // Stack for tracking recursive evaluations
    std::vector<std::string> evaluation_stack_;
    const size_t max_recursion_depth_ = 100;

  public:
    explicit CMakeEvaluator(EvaluationContext& context) : context_(context) {}

    // Evaluate an AST node
    Result<EvaluatedValue, AnalysisError> evaluate(const ast::ASTNode& node);

    // Visitor methods for literals
    void visit(const ast::StringLiteral& node) override;
    void visit(const ast::NumberLiteral& node) override;
    void visit(const ast::BooleanLiteral& node) override;
    void visit(const ast::Variable& node) override;
    void visit(const ast::Identifier& node) override;

    // Visitor methods for commands
    void visit(const ast::CommandCall& node) override;
    void visit(const ast::FunctionDef& node) override;
    void visit(const ast::MacroDef& node) override;

    // Visitor methods for control flow
    void visit(const ast::IfStatement& node) override;
    void visit(const ast::ElseIfStatement& node) override;
    void visit(const ast::ElseStatement& node) override;
    void visit(const ast::WhileStatement& node) override;
    void visit(const ast::ForEachStatement& node) override;

    // Visitor methods for expressions
    void visit(const ast::ListExpression& node) override;
    void visit(const ast::GeneratorExpression& node) override;
    void visit(const ast::BracketExpression& node) override;
    void visit(const ast::BinaryOp& node) override;
    void visit(const ast::UnaryOp& node) override;
    void visit(const ast::FunctionCall& node) override;

    // Visitor methods for structure
    void visit(const ast::Block& node) override;
    void visit(const ast::File& node) override;
    void visit(const ast::ErrorNode& node) override;

    // Visitor methods for CPM nodes
    void visit(const ast::CPMAddPackage& node) override;
    void visit(const ast::CPMFindPackage& node) override;
    void visit(const ast::CPMUsePackageLock& node) override;
    void visit(const ast::CPMDeclarePackage& node) override;

  private:
    // Command evaluators
    Result<EvaluatedValue, AnalysisError> evaluate_set_command(const ast::CommandCall& cmd);

    Result<EvaluatedValue, AnalysisError> evaluate_if_command(const ast::CommandCall& cmd);

    Result<EvaluatedValue, AnalysisError>
    evaluate_cmake_minimum_required(const ast::CommandCall& cmd);

    Result<EvaluatedValue, AnalysisError> evaluate_option_command(const ast::CommandCall& cmd);

    Result<EvaluatedValue, AnalysisError> evaluate_project_command(const ast::CommandCall& cmd);

    Result<EvaluatedValue, AnalysisError> evaluate_message_command(const ast::CommandCall& cmd);

    Result<EvaluatedValue, AnalysisError> evaluate_add_library_command(const ast::CommandCall& cmd);

    Result<EvaluatedValue, AnalysisError>
    evaluate_add_executable_command(const ast::CommandCall& cmd);

    Result<EvaluatedValue, AnalysisError>
    evaluate_target_include_directories_command(const ast::CommandCall& cmd);

    Result<EvaluatedValue, AnalysisError>
    evaluate_target_link_libraries_command(const ast::CommandCall& cmd);

    Result<EvaluatedValue, AnalysisError>
    evaluate_target_compile_definitions_command(const ast::CommandCall& cmd);

    // Condition evaluation
    Result<bool, AnalysisError> evaluate_condition(const ast::ASTNode& condition);

    Result<bool, AnalysisError> evaluate_condition_function(const ast::CommandCall& cmd);

    Result<bool, AnalysisError> evaluate_string_comparison(const std::string& op,
                                                           const std::string& left,
                                                           const std::string& right);

    Result<bool, AnalysisError> evaluate_version_comparison(const std::string& op,
                                                            const std::string& left,
                                                            const std::string& right);

    // String interpolation
    Result<std::string, AnalysisError> interpolate_string(const std::string& str);

    Result<std::string, AnalysisError> expand_variable_reference(const std::string& var_name);

    // Platform detection
    Result<bool, AnalysisError> evaluate_platform_check(const std::string& platform);

    // Helper methods
    bool is_truthy_string(const std::string& str) const;
    bool is_list_empty(const std::vector<std::string>& list) const;

    // Error helpers
    AnalysisError make_evaluation_error(const std::string& message, const ast::ASTNode& node) const;

    // Recursion protection
    bool push_evaluation_stack(const std::string& name);
    void pop_evaluation_stack();
};

// Utility class for evaluating CMake files
class CMakeFileEvaluator {
  private:
    EvaluationContext context_;

  public:
    CMakeFileEvaluator();

    // Evaluate a parsed CMake file
    Result<void, AnalysisError> evaluate_file(const ast::File& file);

    // Analyze a parsed CMake file and return ProjectAnalysis
    Result<ProjectAnalysis, AnalysisError> analyze(const ast::File& file);

    // Get the evaluation context
    EvaluationContext& context() {
        return context_;
    }
    const EvaluationContext& context() const {
        return context_;
    }

    // Get variable value
    std::optional<EvaluatedValue> get_variable(const std::string& name) const {
        return context_.get_variable(name);
    }

    // Get all defined variables
    std::vector<std::string> list_variables() const {
        return context_.list_variables();
    }
};

} // namespace finch::analyzer
