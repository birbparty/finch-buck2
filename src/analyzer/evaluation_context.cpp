#include <algorithm>
#include <finch/analyzer/evaluation_context.hpp>
#include <finch/core/logging.hpp>
#include <sstream>

namespace finch::analyzer {

void EvaluationContext::set_variable(const std::string& name, Value value, Confidence confidence) {
    variables_[name] = EvaluatedValue{std::move(value), confidence};
    LOG_TRACE("Set variable '{}' with confidence {}", name, static_cast<int>(confidence));
}

std::optional<EvaluatedValue> EvaluationContext::get_variable(const std::string& name) const {
    // Check current scope first
    if (auto it = variables_.find(name); it != variables_.end()) {
        return it->second;
    }

    // Check parent scope
    if (parent_) {
        return parent_->get_variable(name);
    }

    return std::nullopt;
}

void EvaluationContext::set_cache_variable(const std::string& name, Value value,
                                           Confidence confidence) {
    cache_variables_[name] = EvaluatedValue{std::move(value), confidence};
    LOG_TRACE("Set cache variable '{}' with confidence {}", name, static_cast<int>(confidence));
}

std::optional<EvaluatedValue> EvaluationContext::get_cache_variable(const std::string& name) const {
    // Cache variables don't inherit from parent scope
    if (auto it = cache_variables_.find(name); it != cache_variables_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void EvaluationContext::set_platform_check(const std::string& check, bool result) {
    platform_checks_[check] = result;
    LOG_TRACE("Set platform check '{}' = {}", check, result);
}

std::optional<bool> EvaluationContext::get_platform_check(const std::string& check) const {
    if (auto it = platform_checks_.find(check); it != platform_checks_.end()) {
        return it->second;
    }

    // Check parent scope
    if (parent_) {
        return parent_->get_platform_check(check);
    }

    return std::nullopt;
}

void EvaluationContext::add_target(const Target& target) {
    targets_.push_back(target);
    LOG_TRACE("Added target '{}' of type {}", target.name, static_cast<int>(target.type));
}

const std::vector<Target>& EvaluationContext::get_targets() const {
    return targets_;
}

std::unique_ptr<EvaluationContext> EvaluationContext::create_child_scope() {
    return std::make_unique<EvaluationContext>(this);
}

void EvaluationContext::initialize_builtin_variables() {
    // Common CMake variables
    set_variable("CMAKE_SOURCE_DIR", "/source", Confidence::Uncertain);
    set_variable("CMAKE_BINARY_DIR", "/build", Confidence::Uncertain);
    set_variable("CMAKE_CURRENT_SOURCE_DIR", "/source", Confidence::Uncertain);
    set_variable("CMAKE_CURRENT_BINARY_DIR", "/build", Confidence::Uncertain);

// Platform variables (will be resolved to Buck2 select())
#ifdef _WIN32
    set_variable("WIN32", "1", Confidence::Certain);
    set_variable("WINDOWS", "1", Confidence::Certain);
    set_variable("UNIX", "", Confidence::Certain);
    set_variable("APPLE", "", Confidence::Certain);
    set_variable("LINUX", "", Confidence::Certain);
#elif defined(__APPLE__)
    set_variable("APPLE", "1", Confidence::Certain);
    set_variable("UNIX", "1", Confidence::Certain);
    set_variable("DARWIN", "1", Confidence::Certain);
    set_variable("WIN32", "", Confidence::Certain);
    set_variable("WINDOWS", "", Confidence::Certain);
    set_variable("LINUX", "", Confidence::Certain);
#else
    set_variable("UNIX", "1", Confidence::Certain);
    set_variable("LINUX", "1", Confidence::Certain);
    set_variable("WIN32", "", Confidence::Certain);
    set_variable("WINDOWS", "", Confidence::Certain);
    set_variable("APPLE", "", Confidence::Certain);
#endif

    // C++ compiler info (generic)
    set_variable("CMAKE_CXX_COMPILER_ID", "Generic", Confidence::Uncertain);
    set_variable("CMAKE_CXX_STANDARD", "17", Confidence::Likely);
    set_variable("CMAKE_C_COMPILER_ID", "Generic", Confidence::Uncertain);
    set_variable("CMAKE_C_STANDARD", "11", Confidence::Likely);

    // Build type (common default)
    set_variable("CMAKE_BUILD_TYPE", "Release", Confidence::Uncertain);

    // Common boolean values
    set_variable("TRUE", "1", Confidence::Certain);
    set_variable("FALSE", "", Confidence::Certain);
    set_variable("ON", "ON", Confidence::Certain);
    set_variable("OFF", "OFF", Confidence::Certain);
    set_variable("YES", "1", Confidence::Certain);
    set_variable("NO", "", Confidence::Certain);

    LOG_DEBUG("Initialized built-in CMake variables");
}

bool EvaluationContext::has_variable(const std::string& name) const {
    return get_variable(name).has_value();
}

bool EvaluationContext::has_cache_variable(const std::string& name) const {
    return get_cache_variable(name).has_value();
}

std::vector<std::string> EvaluationContext::list_variables() const {
    std::vector<std::string> result;
    for (const auto& [name, _] : variables_) {
        result.push_back(name);
    }

    // Add parent variables if any
    if (parent_) {
        auto parent_vars = parent_->list_variables();
        result.insert(result.end(), parent_vars.begin(), parent_vars.end());
    }

    // Remove duplicates and sort
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

std::vector<std::string> EvaluationContext::list_cache_variables() const {
    std::vector<std::string> result;
    for (const auto& [name, _] : cache_variables_) {
        result.push_back(name);
    }
    std::sort(result.begin(), result.end());
    return result;
}

namespace value_helpers {

std::string to_string(const Value& value) {
    return std::visit(
        [](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return v;
            } else if constexpr (std::is_same_v<T, bool>) {
                return v ? "TRUE" : "FALSE";
            } else if constexpr (std::is_same_v<T, double>) {
                // Format double without trailing zeros
                std::ostringstream oss;
                oss << v;
                return oss.str();
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                // Join list with semicolons (CMake list format)
                if (v.empty()) {
                    return "";
                }
                std::string result;
                for (size_t i = 0; i < v.size(); ++i) {
                    if (i > 0) {
                        result += ";";
                    }
                    result += v[i];
                }
                return result;
            }
        },
        value);
}

bool is_truthy(const Value& value) {
    return std::visit(
        [](const auto& v) -> bool {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                // CMake considers these false
                return !v.empty() && v != "0" && v != "OFF" && v != "NO" && v != "FALSE" &&
                       v != "N" && v != "IGNORE" && v != "NOTFOUND" && !v.ends_with("-NOTFOUND");
            } else if constexpr (std::is_same_v<T, bool>) {
                return v;
            } else if constexpr (std::is_same_v<T, double>) {
                return v != 0.0;
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                return !v.empty();
            }
        },
        value);
}

std::optional<bool> to_bool(const Value& value) {
    return std::visit(
        [](const auto& v) -> std::optional<bool> {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                if (v == "1" || v == "ON" || v == "YES" || v == "TRUE" || v == "Y") {
                    return true;
                }
                if (v == "0" || v == "OFF" || v == "NO" || v == "FALSE" || v == "N" ||
                    v == "IGNORE" || v == "NOTFOUND" || v.ends_with("-NOTFOUND") || v.empty()) {
                    return false;
                }
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, bool>) {
                return v;
            } else if constexpr (std::is_same_v<T, double>) {
                return v != 0.0;
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                return !v.empty();
            }
        },
        value);
}

std::optional<double> to_double(const Value& value) {
    return std::visit(
        [](const auto& v) -> std::optional<double> {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                try {
                    size_t idx;
                    double result = std::stod(v, &idx);
                    if (idx == v.length()) {
                        return result;
                    }
                } catch (...) {
                    // Not a valid double
                }
                return std::nullopt;
            } else if constexpr (std::is_same_v<T, bool>) {
                return v ? 1.0 : 0.0;
            } else if constexpr (std::is_same_v<T, double>) {
                return v;
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                // Can't convert list to double
                return std::nullopt;
            }
        },
        value);
}

std::vector<std::string> to_list(const Value& value) {
    return std::visit(
        [](const auto& v) -> std::vector<std::string> {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                if (v.empty()) {
                    return {};
                }
                // Check if it's already a list (contains semicolons)
                if (v.find(';') != std::string::npos) {
                    std::vector<std::string> result;
                    std::istringstream iss(v);
                    std::string item;
                    while (std::getline(iss, item, ';')) {
                        result.push_back(item);
                    }
                    return result;
                }
                // Single value
                return {v};
            } else if constexpr (std::is_same_v<T, bool>) {
                return {v ? "TRUE" : "FALSE"};
            } else if constexpr (std::is_same_v<T, double>) {
                std::ostringstream oss;
                oss << v;
                return {oss.str()};
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                return v;
            }
        },
        value);
}

} // namespace value_helpers

} // namespace finch::analyzer
