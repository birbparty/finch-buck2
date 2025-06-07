#include <algorithm>
#include <finch/analyzer/cmake_evaluator.hpp>
#include <finch/core/logging.hpp>
#include <finch/parser/ast/commands.hpp>
#include <finch/parser/ast/control_flow.hpp>
#include <finch/parser/ast/cpm_nodes.hpp>
#include <finch/parser/ast/expressions.hpp>
#include <finch/parser/ast/literals.hpp>
#include <finch/parser/ast/node.hpp>
#include <finch/parser/ast/structure.hpp>
#include <fmt/format.h>
#include <regex>

namespace finch::analyzer {

using namespace finch::ast;

Result<EvaluatedValue, AnalysisError> CMakeEvaluator::evaluate(const ast::ASTNode& node) {
    result_ = Result<EvaluatedValue, AnalysisError>(std::in_place_index<1>,
                                                    AnalysisError("Not evaluated"));
    node.accept(*this);
    return result_;
}

void CMakeEvaluator::visit(const ast::StringLiteral& node) {
    // Try to interpolate variables in string
    std::string str_value(node.value());
    auto interpolated = interpolate_string(str_value);
    if (interpolated.has_value()) {
        result_ = Result<EvaluatedValue, AnalysisError>(
            EvaluatedValue{interpolated.value(), Confidence::Certain});
    } else {
        // Return original string with low confidence if interpolation failed
        result_ = Result<EvaluatedValue, AnalysisError>(
            EvaluatedValue{std::string(node.value()), Confidence::Uncertain});
    }
}

void CMakeEvaluator::visit(const ast::NumberLiteral& node) {
    // Numbers are certain
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{node.as_float(), Confidence::Certain});
}

void CMakeEvaluator::visit(const ast::BooleanLiteral& node) {
    // Booleans are certain
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{node.value(), Confidence::Certain});
}

void CMakeEvaluator::visit(const ast::Variable& node) {
    std::string var_name(node.name()); // Convert string_view to string
    auto value = context_.get_variable(var_name);
    if (value) {
        result_ = Result<EvaluatedValue, AnalysisError>(*value);
    } else {
        LOG_DEBUG("Unknown variable: {}", var_name);
        result_ = Result<EvaluatedValue, AnalysisError>(
            EvaluatedValue{fmt::format("${{{}}}", var_name), // Keep as variable reference
                           Confidence::Unknown});
    }
}

void CMakeEvaluator::visit(const ast::Identifier& node) {
    // Identifiers are treated as strings
    result_ = Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{std::string(node.name()), Confidence::Certain});
}

void CMakeEvaluator::visit(const ast::CommandCall& node) {
    const auto& name = node.name();

    if (name == "set") {
        result_ = evaluate_set_command(node);
    } else if (name == "if") {
        result_ = evaluate_if_command(node);
    } else if (name == "cmake_minimum_required") {
        result_ = evaluate_cmake_minimum_required(node);
    } else if (name == "option") {
        result_ = evaluate_option_command(node);
    } else if (name == "project") {
        result_ = evaluate_project_command(node);
    } else if (name == "message") {
        result_ = evaluate_message_command(node);
    } else if (name == "add_library") {
        result_ = evaluate_add_library_command(node);
    } else if (name == "add_executable") {
        result_ = evaluate_add_executable_command(node);
    } else if (name == "target_include_directories") {
        result_ = evaluate_target_include_directories_command(node);
    } else if (name == "target_link_libraries") {
        result_ = evaluate_target_link_libraries_command(node);
    } else if (name == "target_compile_definitions") {
        result_ = evaluate_target_compile_definitions_command(node);
    } else {
        // Unknown command - don't evaluate
        LOG_TRACE("Unknown command for evaluation: {}", name);
        result_ = Result<EvaluatedValue, AnalysisError>(
            EvaluatedValue{std::string(""), Confidence::Unknown});
    }
}

Result<EvaluatedValue, AnalysisError>
CMakeEvaluator::evaluate_set_command(const ast::CommandCall& cmd) {
    const auto& args = cmd.arguments();

    if (args.size() < 2) {
        return Result<EvaluatedValue, AnalysisError>(
            std::in_place_index<1>, AnalysisError("set() requires at least 2 arguments"));
    }

    // Get variable name
    auto name_result = evaluate(*args[0]);
    if (name_result.has_error() || !name_result.value().is_certain()) {
        return Result<EvaluatedValue, AnalysisError>(
            std::in_place_index<1>, AnalysisError("Cannot determine variable name"));
    }

    auto var_name = std::get<std::string>(name_result.value().value);

    // Handle special set() forms
    if (args.size() == 2) {
        // Single value
        auto value_result = evaluate(*args[1]);
        if (value_result.has_value()) {
            context_.set_variable(var_name, value_result.value().value,
                                  value_result.value().confidence);
        }
    } else {
        // Multiple values - create a list
        std::vector<std::string> list_values;
        Confidence min_confidence = Confidence::Certain;

        for (size_t i = 1; i < args.size(); ++i) {
            auto value_result = evaluate(*args[i]);
            if (value_result.has_error()) {
                min_confidence = Confidence::Unknown;
                break;
            }

            auto str_val = value_helpers::to_string(value_result.value().value);
            list_values.push_back(str_val);
            min_confidence = std::min(min_confidence, value_result.value().confidence);
        }

        context_.set_variable(var_name, list_values, min_confidence);
    }

    return Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{std::string(""), Confidence::Certain});
}

Result<EvaluatedValue, AnalysisError>
CMakeEvaluator::evaluate_cmake_minimum_required(const ast::CommandCall& cmd) {
    const auto& args = cmd.arguments();

    if (args.size() < 2) {
        return Result<EvaluatedValue, AnalysisError>(
            std::in_place_index<1>,
            AnalysisError("cmake_minimum_required() requires VERSION argument"));
    }

    // Look for VERSION keyword
    size_t version_idx = 0;
    for (size_t i = 0; i < args.size() - 1; ++i) {
        auto keyword_result = evaluate(*args[i]);
        if (keyword_result.has_value()) {
            auto keyword = value_helpers::to_string(keyword_result.value().value);
            if (keyword == "VERSION") {
                version_idx = i + 1;
                break;
            }
        }
    }

    if (version_idx > 0 && version_idx < args.size()) {
        auto version_result = evaluate(*args[version_idx]);
        if (version_result.has_value()) {
            auto version = value_helpers::to_string(version_result.value().value);
            context_.set_variable("CMAKE_MINIMUM_REQUIRED_VERSION", version, Confidence::Certain);
        }
    }

    return Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{std::string(""), Confidence::Certain});
}

Result<EvaluatedValue, AnalysisError>
CMakeEvaluator::evaluate_option_command(const ast::CommandCall& cmd) {
    const auto& args = cmd.arguments();

    if (args.size() < 2) {
        return Result<EvaluatedValue, AnalysisError>(
            std::in_place_index<1>, AnalysisError("option() requires at least 2 arguments"));
    }

    // Get option name
    auto name_result = evaluate(*args[0]);
    if (name_result.has_error()) {
        return Result<EvaluatedValue, AnalysisError>(std::in_place_index<1>, name_result.error());
    }

    auto option_name = value_helpers::to_string(name_result.value().value);

    // Get default value (ON/OFF)
    bool default_value = false;
    if (args.size() >= 3) {
        // Skip description string, look for ON/OFF at the end
        auto value_result = evaluate(*args[args.size() - 1]);
        if (value_result.has_value()) {
            auto value_str = value_helpers::to_string(value_result.value().value);
            default_value = (value_str == "ON" || value_str == "TRUE" || value_str == "YES" ||
                             value_str == "1");
        }
    }

    // Set as cache variable with uncertain confidence (can be overridden)
    context_.set_cache_variable(option_name, default_value ? "ON" : "OFF", Confidence::Uncertain);

    return Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{std::string(""), Confidence::Certain});
}

Result<std::string, AnalysisError> CMakeEvaluator::interpolate_string(const std::string& str) {
    // Regular expression to find ${VAR} patterns
    static const std::regex var_pattern(R"(\$\{([^}]+)\})");

    std::string result = str;
    std::smatch match;
    std::string::const_iterator search_start(str.cbegin());

    while (std::regex_search(search_start, str.cend(), match, var_pattern)) {
        std::string var_name = match[1];

        // Expand the variable
        auto expanded = expand_variable_reference(var_name);
        if (expanded.has_value()) {
            // Replace in result
            size_t pos = match.position() + (search_start - str.cbegin());
            result.replace(pos, match.length(), expanded.value());
            search_start = str.cbegin() + pos + expanded.value().length();
        } else {
            // Skip this variable reference if we can't expand it
            search_start = match.suffix().first;
        }
    }

    return Result<std::string, AnalysisError>(result);
}

Result<std::string, AnalysisError>
CMakeEvaluator::expand_variable_reference(const std::string& var_name) {
    // Check for environment variables
    if (var_name.starts_with("ENV{") && var_name.ends_with("}")) {
        // Environment variables are uncertain
        return Result<std::string, AnalysisError>(fmt::format("${{{}}}", var_name));
    }

    // Look up variable
    auto value = context_.get_variable(var_name);
    if (value) {
        return Result<std::string, AnalysisError>(value_helpers::to_string(value->value));
    }

    // Check cache variables
    auto cache_value = context_.get_cache_variable(var_name);
    if (cache_value) {
        return Result<std::string, AnalysisError>(value_helpers::to_string(cache_value->value));
    }

    // Unknown variable
    return Result<std::string, AnalysisError>(
        std::in_place_index<1>, AnalysisError(fmt::format("Unknown variable: {}", var_name)));
}

Result<bool, AnalysisError> CMakeEvaluator::evaluate_condition(const ast::ASTNode& condition) {
    // Evaluate the condition node
    auto result = evaluate(condition);
    if (result.has_error()) {
        return Result<bool, AnalysisError>(std::in_place_index<1>, result.error());
    }

    // Convert to boolean
    return Result<bool, AnalysisError>(value_helpers::is_truthy(result.value().value));
}

Result<bool, AnalysisError> CMakeEvaluator::evaluate_platform_check(const std::string& platform) {
    // Check cached results first
    if (auto cached = context_.get_platform_check(platform)) {
        return Result<bool, AnalysisError>(*cached);
    }

    // Common platform checks
    static const std::unordered_map<std::string, std::string> platform_vars = {
        {"WIN32", "WIN32"}, {"WINDOWS", "WINDOWS"}, {"UNIX", "UNIX"},
        {"LINUX", "LINUX"}, {"APPLE", "APPLE"},     {"DARWIN", "DARWIN"},
        {"MSVC", "MSVC"},   {"MINGW", "MINGW"},     {"CYGWIN", "CYGWIN"}};

    auto it = platform_vars.find(platform);
    if (it != platform_vars.end()) {
        auto var = context_.get_variable(it->second);
        if (var && var->is_certain()) {
            bool result = value_helpers::is_truthy(var->value);
            context_.set_platform_check(platform, result);
            return Result<bool, AnalysisError>(result);
        }
    }

    // Unknown platform - preserve for Buck2 select()
    LOG_WARN("Unknown platform check: {}", platform);
    return Result<bool, AnalysisError>(
        std::in_place_index<1>,
        AnalysisError(fmt::format("Cannot evaluate platform: {}", platform)));
}

// Implement remaining visitor methods with default behavior
void CMakeEvaluator::visit(const ast::FunctionDef& node) {
    // Don't evaluate function definitions
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::MacroDef& node) {
    // Don't evaluate macro definitions
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::IfStatement& node) {
    // Evaluate condition
    auto cond_result = evaluate_condition(*node.condition());
    if (cond_result.has_value() && cond_result.value()) {
        // Condition is true - evaluate then branch
        for (const auto& stmt : node.then_branch()) {
            evaluate(*stmt);
        }
    } else if (cond_result.has_value() && !cond_result.value()) {
        // Condition is false - check elseif/else branches
        bool evaluated = false;
        // The elseif_branches_ contains conditions and body statements interleaved
        for (size_t i = 0; i < node.elseif_branches().size();) {
            // Check if this is an ElseIfStatement (condition)
            if (auto* elseif_stmt =
                    dynamic_cast<const ast::ElseIfStatement*>(node.elseif_branches()[i].get())) {
                auto elseif_cond = evaluate_condition(*elseif_stmt->condition());
                if (elseif_cond.has_value() && elseif_cond.value()) {
                    // Execute the body statements that follow this condition
                    i++; // Skip the condition
                    while (i < node.elseif_branches().size() &&
                           node.elseif_branches()[i]->type() != NodeType::ElseIfStatement) {
                        evaluate(*node.elseif_branches()[i]);
                        i++;
                    }
                    evaluated = true;
                    break;
                } else {
                    // Skip this elseif block
                    i++; // Skip the condition
                    while (i < node.elseif_branches().size() &&
                           node.elseif_branches()[i]->type() != NodeType::ElseIfStatement) {
                        i++;
                    }
                }
            } else {
                i++;
            }
        }

        if (!evaluated && !node.else_branch().empty()) {
            for (const auto& stmt : node.else_branch()) {
                evaluate(*stmt);
            }
        }
    }

    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Certain});
}

void CMakeEvaluator::visit(const ast::ListExpression& node) {
    std::vector<std::string> list_values;
    Confidence min_confidence = Confidence::Certain;

    for (const auto& elem : node.elements()) {
        auto elem_result = evaluate(*elem);
        if (elem_result.has_error()) {
            min_confidence = Confidence::Unknown;
            break;
        }

        auto str_val = value_helpers::to_string(elem_result.value().value);
        list_values.push_back(str_val);
        min_confidence = std::min(min_confidence, elem_result.value().confidence);
    }

    result_ = Result<EvaluatedValue, AnalysisError>(EvaluatedValue{list_values, min_confidence});
}

// Stub implementations for remaining visitor methods
void CMakeEvaluator::visit(const ast::ElseIfStatement& node) {
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::ElseStatement& node) {
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::WhileStatement& node) {
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::ForEachStatement& node) {
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::GeneratorExpression& node) {
    // Generator expressions are not evaluated - preserve for Buck2
    result_ = Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{fmt::format("${{{}}}", node.expression()), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::BracketExpression& node) {
    // Evaluate the content
    auto content_result = evaluate(*node.content());
    result_ = content_result;
}

void CMakeEvaluator::visit(const ast::BinaryOp& node) {
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::UnaryOp& node) {
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::FunctionCall& node) {
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::Block& node) {
    // Evaluate all statements in the block
    for (const auto& stmt : node.statements()) {
        evaluate(*stmt);
    }
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Certain});
}

void CMakeEvaluator::visit(const ast::File& node) {
    // Evaluate all statements in the file
    for (const auto& stmt : node.statements()) {
        evaluate(*stmt);
    }
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Certain});
}

void CMakeEvaluator::visit(const ast::ErrorNode& node) {
    result_ = Result<EvaluatedValue, AnalysisError>(std::in_place_index<1>,
                                                    AnalysisError(node.message()));
}

// CPM-specific nodes
void CMakeEvaluator::visit(const ast::CPMAddPackage& node) {
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::CPMFindPackage& node) {
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::CPMUsePackageLock& node) {
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

void CMakeEvaluator::visit(const ast::CPMDeclarePackage& node) {
    result_ =
        Result<EvaluatedValue, AnalysisError>(EvaluatedValue{std::string(""), Confidence::Unknown});
}

// Implement helper methods
Result<EvaluatedValue, AnalysisError>
CMakeEvaluator::evaluate_if_command(const ast::CommandCall& cmd) {
    // The if() command itself doesn't need evaluation - it's handled by IfStatement
    return Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{std::string(""), Confidence::Certain});
}

Result<EvaluatedValue, AnalysisError>
CMakeEvaluator::evaluate_project_command(const ast::CommandCall& cmd) {
    const auto& args = cmd.arguments();

    if (!args.empty()) {
        auto name_result = evaluate(*args[0]);
        if (name_result.has_value()) {
            auto project_name = value_helpers::to_string(name_result.value().value);
            context_.set_variable("PROJECT_NAME", project_name, Confidence::Certain);
            context_.set_variable("CMAKE_PROJECT_NAME", project_name, Confidence::Certain);
        }
    }

    return Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{std::string(""), Confidence::Certain});
}

Result<EvaluatedValue, AnalysisError>
CMakeEvaluator::evaluate_message_command(const ast::CommandCall& cmd) {
    // Messages don't affect evaluation
    return Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{std::string(""), Confidence::Certain});
}

Result<EvaluatedValue, AnalysisError>
CMakeEvaluator::evaluate_add_library_command(const ast::CommandCall& cmd) {
    const auto& args = cmd.arguments();

    if (args.empty()) {
        return Result<EvaluatedValue, AnalysisError>(
            std::in_place_index<1>, AnalysisError("add_library() requires target name"));
    }

    // Get target name
    auto name_result = evaluate(*args[0]);
    if (name_result.has_error()) {
        return Result<EvaluatedValue, AnalysisError>(std::in_place_index<1>, name_result.error());
    }

    auto target_name = value_helpers::to_string(name_result.value().value);

    // Create target
    Target target;
    target.name = target_name;
    target.type = Target::Type::StaticLibrary; // Default

    // Parse library type and sources
    size_t source_start = 1;
    if (args.size() > 1) {
        auto type_result = evaluate(*args[1]);
        if (type_result.has_value()) {
            auto type_str = value_helpers::to_string(type_result.value().value);
            if (type_str == "SHARED") {
                target.type = Target::Type::SharedLibrary;
                source_start = 2;
            } else if (type_str == "STATIC") {
                target.type = Target::Type::StaticLibrary;
                source_start = 2;
            } else if (type_str == "INTERFACE") {
                target.type = Target::Type::InterfaceLibrary;
                source_start = 2;
            }
            // If not a type keyword, treat as source file
        }
    }

    // Extract source files
    for (size_t i = source_start; i < args.size(); ++i) {
        auto source_result = evaluate(*args[i]);
        if (source_result.has_value()) {
            auto source_path = value_helpers::to_string(source_result.value().value);
            target.sources.push_back(source_path);
        }
    }

    // Set source directory to current directory (could be improved)
    target.source_directory = std::filesystem::current_path();

    // Add target to context
    context_.add_target(target);
    LOG_DEBUG("Added library target: {}", target_name);

    return Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{std::string(""), Confidence::Certain});
}

Result<EvaluatedValue, AnalysisError>
CMakeEvaluator::evaluate_add_executable_command(const ast::CommandCall& cmd) {
    const auto& args = cmd.arguments();

    if (args.empty()) {
        return Result<EvaluatedValue, AnalysisError>(
            std::in_place_index<1>, AnalysisError("add_executable() requires target name"));
    }

    // Get target name
    auto name_result = evaluate(*args[0]);
    if (name_result.has_error()) {
        return Result<EvaluatedValue, AnalysisError>(std::in_place_index<1>, name_result.error());
    }

    auto target_name = value_helpers::to_string(name_result.value().value);

    // Create target
    Target target;
    target.name = target_name;
    target.type = Target::Type::ExecutableTarget;

    // Extract source files (all args after target name)
    for (size_t i = 1; i < args.size(); ++i) {
        auto source_result = evaluate(*args[i]);
        if (source_result.has_value()) {
            auto source_path = value_helpers::to_string(source_result.value().value);
            target.sources.push_back(source_path);
        }
    }

    // Set source directory to current directory
    target.source_directory = std::filesystem::current_path();

    // Add target to context
    context_.add_target(target);
    LOG_DEBUG("Added executable target: {}", target_name);

    return Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{std::string(""), Confidence::Certain});
}

Result<EvaluatedValue, AnalysisError>
CMakeEvaluator::evaluate_target_include_directories_command(const ast::CommandCall& cmd) {
    const auto& args = cmd.arguments();

    if (args.size() < 2) {
        return Result<EvaluatedValue, AnalysisError>(
            std::in_place_index<1>,
            AnalysisError("target_include_directories() requires target and directories"));
    }

    // Get target name
    auto name_result = evaluate(*args[0]);
    if (name_result.has_error()) {
        return Result<EvaluatedValue, AnalysisError>(std::in_place_index<1>, name_result.error());
    }

    auto target_name = value_helpers::to_string(name_result.value().value);

    // Find target in context and update it
    auto& targets = const_cast<std::vector<Target>&>(context_.get_targets());
    for (auto& target : targets) {
        if (target.name == target_name) {
            // Extract include directories (skip visibility specifiers for now)
            for (size_t i = 1; i < args.size(); ++i) {
                auto dir_result = evaluate(*args[i]);
                if (dir_result.has_value()) {
                    auto dir_path = value_helpers::to_string(dir_result.value().value);
                    // Skip visibility keywords
                    if (dir_path != "PUBLIC" && dir_path != "PRIVATE" && dir_path != "INTERFACE") {
                        target.include_directories.push_back(dir_path);
                    }
                }
            }
            LOG_DEBUG("Updated include directories for target: {}", target_name);
            break;
        }
    }

    return Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{std::string(""), Confidence::Certain});
}

Result<EvaluatedValue, AnalysisError>
CMakeEvaluator::evaluate_target_link_libraries_command(const ast::CommandCall& cmd) {
    const auto& args = cmd.arguments();

    if (args.size() < 2) {
        return Result<EvaluatedValue, AnalysisError>(
            std::in_place_index<1>,
            AnalysisError("target_link_libraries() requires target and libraries"));
    }

    // Get target name
    auto name_result = evaluate(*args[0]);
    if (name_result.has_error()) {
        return Result<EvaluatedValue, AnalysisError>(std::in_place_index<1>, name_result.error());
    }

    auto target_name = value_helpers::to_string(name_result.value().value);

    // Find target in context and update it
    auto& targets = const_cast<std::vector<Target>&>(context_.get_targets());
    for (auto& target : targets) {
        if (target.name == target_name) {
            // Extract link libraries (skip visibility specifiers for now)
            for (size_t i = 1; i < args.size(); ++i) {
                auto lib_result = evaluate(*args[i]);
                if (lib_result.has_value()) {
                    auto lib_name = value_helpers::to_string(lib_result.value().value);
                    // Skip visibility keywords
                    if (lib_name != "PUBLIC" && lib_name != "PRIVATE" && lib_name != "INTERFACE") {
                        target.link_libraries.push_back(lib_name);
                    }
                }
            }
            LOG_DEBUG("Updated link libraries for target: {}", target_name);
            break;
        }
    }

    return Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{std::string(""), Confidence::Certain});
}

Result<EvaluatedValue, AnalysisError>
CMakeEvaluator::evaluate_target_compile_definitions_command(const ast::CommandCall& cmd) {
    const auto& args = cmd.arguments();

    if (args.size() < 2) {
        return Result<EvaluatedValue, AnalysisError>(
            std::in_place_index<1>,
            AnalysisError("target_compile_definitions() requires target and definitions"));
    }

    // Get target name
    auto name_result = evaluate(*args[0]);
    if (name_result.has_error()) {
        return Result<EvaluatedValue, AnalysisError>(std::in_place_index<1>, name_result.error());
    }

    auto target_name = value_helpers::to_string(name_result.value().value);

    // Find target in context and update it
    auto& targets = const_cast<std::vector<Target>&>(context_.get_targets());
    for (auto& target : targets) {
        if (target.name == target_name) {
            // Extract compile definitions (skip visibility specifiers for now)
            for (size_t i = 1; i < args.size(); ++i) {
                auto def_result = evaluate(*args[i]);
                if (def_result.has_value()) {
                    auto definition = value_helpers::to_string(def_result.value().value);
                    // Skip visibility keywords
                    if (definition != "PUBLIC" && definition != "PRIVATE" &&
                        definition != "INTERFACE") {
                        target.compile_definitions.push_back(definition);
                    }
                }
            }
            LOG_DEBUG("Updated compile definitions for target: {}", target_name);
            break;
        }
    }

    return Result<EvaluatedValue, AnalysisError>(
        EvaluatedValue{std::string(""), Confidence::Certain});
}

// CMakeFileEvaluator implementation
CMakeFileEvaluator::CMakeFileEvaluator() {
    context_.initialize_builtin_variables();
}

Result<void, AnalysisError> CMakeFileEvaluator::evaluate_file(const ast::File& file) {
    CMakeEvaluator evaluator(context_);
    auto result = evaluator.evaluate(file);

    if (result.has_error()) {
        return Result<void, AnalysisError>::error(result.error());
    }

    return Ok<AnalysisError>();
}

Result<ProjectAnalysis, AnalysisError> CMakeFileEvaluator::analyze(const ast::File& file) {
    // First evaluate the file to populate context
    auto eval_result = evaluate_file(file);
    if (eval_result.has_error()) {
        return Result<ProjectAnalysis, AnalysisError>(std::in_place_index<1>, eval_result.error());
    }

    // Create project analysis from the evaluation context
    ProjectAnalysis analysis;

    // Extract project name and version from context
    if (auto project_name = context_.get_variable("PROJECT_NAME")) {
        analysis.project_name = value_helpers::to_string(project_name->value);
    }
    if (auto project_version = context_.get_variable("PROJECT_VERSION")) {
        analysis.project_version = value_helpers::to_string(project_version->value);
    }

    // Extract targets from the evaluation context
    auto targets = context_.get_targets();
    for (const auto& target : targets) {
        analysis.targets.push_back(target);
    }

    // Extract global variables
    for (const auto& var_name : context_.list_variables()) {
        if (auto var_value = context_.get_variable(var_name)) {
            analysis.global_variables[var_name] = value_helpers::to_string(var_value->value);
        }
    }

    // Extract cache variables
    for (const auto& var_name : context_.list_cache_variables()) {
        if (auto var_value = context_.get_cache_variable(var_name)) {
            analysis.cache_variables[var_name] = value_helpers::to_string(var_value->value);
        }
    }

    return Result<ProjectAnalysis, AnalysisError>(analysis);
}

} // namespace finch::analyzer
