#pragma once

#include <finch/analyzer/project_analysis.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace finch::analyzer {

// Value types that can be stored in CMake
using Value = std::variant<std::string, bool, double, std::vector<std::string>>;

// Confidence in evaluation result
enum class Confidence {
    Certain,   // Known value
    Likely,    // High confidence guess
    Uncertain, // Low confidence
    Unknown    // Cannot evaluate
};

// Evaluated value with confidence
struct EvaluatedValue {
    Value value;
    Confidence confidence;

    bool is_certain() const {
        return confidence == Confidence::Certain;
    }
    bool is_known() const {
        return confidence != Confidence::Unknown;
    }
};

// CMake evaluation context
class EvaluationContext {
  private:
    // Variable storage
    std::unordered_map<std::string, EvaluatedValue> variables_;

    // Cache variables
    std::unordered_map<std::string, EvaluatedValue> cache_variables_;

    // Platform detection results
    std::unordered_map<std::string, bool> platform_checks_;

    // Targets discovered during evaluation
    std::vector<Target> targets_;

    // Parent context for scoping
    EvaluationContext* parent_ = nullptr;

  public:
    EvaluationContext() = default;
    explicit EvaluationContext(EvaluationContext* parent) : parent_(parent) {}

    // Variable operations
    void set_variable(const std::string& name, Value value,
                      Confidence confidence = Confidence::Certain);

    std::optional<EvaluatedValue> get_variable(const std::string& name) const;

    // Cache variable operations
    void set_cache_variable(const std::string& name, Value value,
                            Confidence confidence = Confidence::Certain);

    std::optional<EvaluatedValue> get_cache_variable(const std::string& name) const;

    // Platform checks
    void set_platform_check(const std::string& check, bool result);
    std::optional<bool> get_platform_check(const std::string& check) const;

    // Target management
    void add_target(const Target& target);
    const std::vector<Target>& get_targets() const;

    // Scope management
    std::unique_ptr<EvaluationContext> create_child_scope();

    // Built-in variables
    void initialize_builtin_variables();

    // Utility functions
    bool has_variable(const std::string& name) const;
    bool has_cache_variable(const std::string& name) const;

    // Debug/inspection
    std::vector<std::string> list_variables() const;
    std::vector<std::string> list_cache_variables() const;
};

// Helper functions for working with values
namespace value_helpers {

// Convert value to string representation
std::string to_string(const Value& value);

// Check if value is truthy (for conditions)
bool is_truthy(const Value& value);

// Convert value to bool if possible
std::optional<bool> to_bool(const Value& value);

// Convert value to double if possible
std::optional<double> to_double(const Value& value);

// Convert value to list (single string becomes one-element list)
std::vector<std::string> to_list(const Value& value);

} // namespace value_helpers

} // namespace finch::analyzer
