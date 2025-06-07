#pragma once

#include "logging.hpp"
#include "logging_helpers.hpp"
#include <fmt/format.h>
#include <string>

namespace finch {

/// Module-specific logger that prefixes messages with module name
class ModuleLogger {
  private:
    std::string module_;

  public:
    explicit ModuleLogger(std::string module) : module_(std::move(module)) {}

    /// Get the module name
    [[nodiscard]] const std::string& module_name() const noexcept {
        return module_;
    }

    /// Check if logging is enabled for this module at the given level
    [[nodiscard]] bool should_log(spdlog::level::level_enum level) const {
        auto module_level = ModuleLoggerRegistry::get_module_level(module_);
        return level >= module_level;
    }

    template <typename... Args>
    void trace(fmt::format_string<Args...> fmt, Args&&... args) {
        if (should_log(spdlog::level::trace)) {
            LOG_TRACE("[{}] {}", module_, fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    void debug(fmt::format_string<Args...> fmt, Args&&... args) {
        if (should_log(spdlog::level::debug)) {
            LOG_DEBUG("[{}] {}", module_, fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    void info(fmt::format_string<Args...> fmt, Args&&... args) {
        if (should_log(spdlog::level::info)) {
            LOG_INFO("[{}] {}", module_, fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    void warn(fmt::format_string<Args...> fmt, Args&&... args) {
        if (should_log(spdlog::level::warn)) {
            LOG_WARN("[{}] {}", module_, fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    void error(fmt::format_string<Args...> fmt, Args&&... args) {
        if (should_log(spdlog::level::err)) {
            LOG_ERROR("[{}] {}", module_, fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    void critical(fmt::format_string<Args...> fmt, Args&&... args) {
        if (should_log(spdlog::level::critical)) {
            LOG_CRITICAL("[{}] {}", module_, fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    /// Trace with additional structured data
    template <typename... Args>
    void trace_with_data(const std::string& key, const std::string& value,
                         fmt::format_string<Args...> fmt, Args&&... args) {
        if (should_log(spdlog::level::trace)) {
            LOG_TRACE("[{}] {} [{}={}]", module_, fmt::format(fmt, std::forward<Args>(args)...),
                      key, value);
        }
    }

    /// Debug with additional structured data
    template <typename... Args>
    void debug_with_data(const std::string& key, const std::string& value,
                         fmt::format_string<Args...> fmt, Args&&... args) {
        if (should_log(spdlog::level::debug)) {
            LOG_DEBUG("[{}] {} [{}={}]", module_, fmt::format(fmt, std::forward<Args>(args)...),
                      key, value);
        }
    }

    /// Info with additional structured data
    template <typename... Args>
    void info_with_data(const std::string& key, const std::string& value,
                        fmt::format_string<Args...> fmt, Args&&... args) {
        if (should_log(spdlog::level::info)) {
            LOG_INFO("[{}] {} [{}={}]", module_, fmt::format(fmt, std::forward<Args>(args)...), key,
                     value);
        }
    }

    /// Create a timer with module context
    [[nodiscard]] LogTimer
    create_timer(const std::string& operation,
                 spdlog::level::level_enum level = spdlog::level::debug) const {
        return LogTimer(fmt::format("[{}] {}", module_, operation), level);
    }

    /// Create a progress logger with module context
    [[nodiscard]] ProgressLogger create_progress(const std::string& task, size_t total,
                                                 size_t report_interval = 10) const {
        return ProgressLogger(fmt::format("[{}] {}", module_, task), total, report_interval);
    }

    /// Create a scoped logger with module context
    [[nodiscard]] ScopedLogger
    create_scoped(const std::string& operation,
                  spdlog::level::level_enum level = spdlog::level::info) const {
        return ScopedLogger(fmt::format("[{}] {}", module_, operation), level);
    }
};

} // namespace finch

// Convenience macros for module logging
#define MODULE_LOG(module) finch::ModuleLogger _module_logger(module)
#define MODULE_TRACE(module, ...) finch::ModuleLogger(module).trace(__VA_ARGS__)
#define MODULE_DEBUG(module, ...) finch::ModuleLogger(module).debug(__VA_ARGS__)
#define MODULE_INFO(module, ...) finch::ModuleLogger(module).info(__VA_ARGS__)
#define MODULE_WARN(module, ...) finch::ModuleLogger(module).warn(__VA_ARGS__)
#define MODULE_ERROR(module, ...) finch::ModuleLogger(module).error(__VA_ARGS__)
#define MODULE_CRITICAL(module, ...) finch::ModuleLogger(module).critical(__VA_ARGS__)
