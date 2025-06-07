#pragma once

#include "error.hpp"
#include "logging.hpp"
#include "result.hpp"
#include <string>

namespace finch {

/// Log error with full context
inline void log_error(const Error& error) {
    LOG_ERROR("{}", error.message());

    if (error.location()) {
        LOG_ERROR("  at {}", error.location()->to_string());
    }

    // Log context at debug level
    for (const auto& ctx : error.context()) {
        LOG_DEBUG("  context: {}", ctx);
    }

    // Log help text if available
    if (error.help()) {
        LOG_INFO("  help: {}", *error.help());
    }
}

/// Log error with module context
inline void log_error(const std::string& module, const Error& error) {
    LOG_ERROR("[{}] {}", module, error.message());

    if (error.location()) {
        LOG_ERROR("[{}]   at {}", module, error.location()->to_string());
    }

    for (const auto& ctx : error.context()) {
        LOG_DEBUG("[{}]   context: {}", module, ctx);
    }

    if (error.help()) {
        LOG_INFO("[{}]   help: {}", module, *error.help());
    }
}

/// Log result of operation
template <typename T, typename E>
void log_result(const Result<T, E>& result, const std::string& operation) {
    if (result.has_value()) {
        LOG_DEBUG("{} succeeded", operation);
    } else {
        LOG_ERROR("{} failed: {}", operation, result.error().message());

        // Log additional error context
        const auto& error = result.error();
        if (error.location()) {
            LOG_ERROR("  at {}", error.location()->to_string());
        }

        for (const auto& ctx : error.context()) {
            LOG_DEBUG("  context: {}", ctx);
        }

        if (error.help()) {
            LOG_INFO("  help: {}", *error.help());
        }
    }
}

/// Log result with module context
template <typename T, typename E>
void log_result(const std::string& module, const Result<T, E>& result,
                const std::string& operation) {
    if (result.has_value()) {
        LOG_DEBUG("[{}] {} succeeded", module, operation);
    } else {
        LOG_ERROR("[{}] {} failed: {}", module, operation, result.error().message());

        const auto& error = result.error();
        if (error.location()) {
            LOG_ERROR("[{}]   at {}", module, error.location()->to_string());
        }

        for (const auto& ctx : error.context()) {
            LOG_DEBUG("[{}]   context: {}", module, ctx);
        }

        if (error.help()) {
            LOG_INFO("[{}]   help: {}", module, *error.help());
        }
    }
}

/// Log error with structured data for JSON output
template <typename ErrorType>
void log_error_structured(const ErrorType& error) {
    // Create structured error data
    std::string error_data = fmt::format(
        "{{\"error_type\":\"{}\",\"message\":\"{}\",\"location\":\"{}\",\"context_count\":{}}}",
        error.error_type(), error.message(),
        error.location() ? error.location()->to_string() : "unknown", error.context().size());

    LOG_ERROR("STRUCTURED_ERROR: {}", error_data);

    // Still log human-readable format
    log_error(error);
}

/// RAII error scope for tracking operation context
class ErrorScope {
  private:
    std::string operation_;
    bool has_error_ = false;

  public:
    explicit ErrorScope(std::string operation) : operation_(std::move(operation)) {
        LOG_TRACE("Entering error scope: {}", operation_);
    }

    ~ErrorScope() {
        if (has_error_) {
            LOG_WARN("Exiting error scope: {} (with errors)", operation_);
        } else {
            LOG_TRACE("Exiting error scope: {} (success)", operation_);
        }
    }

    /// Mark that an error occurred in this scope
    void mark_error() {
        has_error_ = true;
    }

    /// Log an error within this scope
    template <typename ErrorType>
    void log_error(const ErrorType& error) {
        mark_error();
        LOG_ERROR("Error in {}: {}", operation_, error.message());
        finch::log_error(error);
    }

    /// Log a result within this scope
    template <typename T, typename E>
    void log_result(const Result<T, E>& result, const std::string& sub_operation = "") {
        const std::string full_operation =
            sub_operation.empty() ? operation_ : fmt::format("{}.{}", operation_, sub_operation);

        if (result.has_error()) {
            mark_error();
        }

        finch::log_result(result, full_operation);
    }
};

/// Exception-safe error logging wrapper
template <typename Func>
auto with_error_logging(const std::string& operation, Func&& func) -> decltype(func()) {
    try {
        ErrorScope scope(operation);
        auto result = func();

        // If result is a Result type, log it
        if constexpr (std::is_same_v<decltype(result),
                                     Result<typename decltype(result)::value_type,
                                            typename decltype(result)::error_type>>) {
            scope.log_result(result);
        }

        return result;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in {}: {}", operation, e.what());
        throw;
    }
}

} // namespace finch

// Convenience macros for error logging
#define LOG_ERROR_SCOPE(name) finch::ErrorScope _error_scope(name)
#define LOG_RESULT(result, operation) finch::log_result(result, operation)
#define LOG_MODULE_RESULT(module, result, operation) finch::log_result(module, result, operation)
#define LOG_ERROR_STRUCTURED(error) finch::log_error_structured(error)
