#pragma once

#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>
#include <shared_mutex>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace finch {

/// Logging configuration structure
struct LogConfig {
    /// Log level for console output
    spdlog::level::level_enum console_level = spdlog::level::info;

    /// Log file path (empty = no file logging)
    std::string log_file;

    /// Use colored output
    bool use_color = true;

    /// Logging mode
    enum class Mode { Synchronous, Asynchronous } mode = Mode::Synchronous;

    /// Async queue size (only used for async mode)
    size_t async_queue_size = 8192;

    /// Log format
    enum class Format {
        Text,
        JSON,
        Both // Log to separate sinks
    } format = Format::Text;

    /// File rotation settings
    size_t max_file_size_mb = 10;
    size_t max_files = 5;

    /// OpenTelemetry configuration
    struct OtelConfig {
        bool enabled = false;
        std::string endpoint = "http://localhost:4318";
        bool use_grpc = false; // false = HTTP, true = gRPC
        std::string service_name = "finch-buck2";
        std::string service_version = "0.1.0";
        std::chrono::seconds export_interval{5};
        std::unordered_map<std::string, std::string> resource_attributes;
    } otel;
};

/// Core logging class
class Logger {
  private:
    static std::shared_ptr<spdlog::logger> logger_;
    static LogConfig config_;
    static std::shared_mutex config_mutex_;
    static bool initialized_;

  public:
    /// Initialize logging system
    static void initialize(const LogConfig& config = LogConfig{});

    /// Shutdown logging system
    static void shutdown();

    /// Check if logger is initialized
    static bool is_initialized() noexcept;

    /// Get logger instance
    static std::shared_ptr<spdlog::logger> get();

    /// Set log level at runtime
    static void set_level(spdlog::level::level_enum level);

    /// Get current log level
    static spdlog::level::level_enum get_level();

    /// Set pattern
    static void set_pattern(const std::string& pattern);

    /// Get current configuration
    static LogConfig get_config();

    /// Update configuration at runtime
    static void update_config(const LogConfig& config);

    /// Force flush all sinks
    static void flush();

  private:
    /// Create text formatter pattern
    static std::string create_text_pattern();

    /// Create JSON formatter pattern
    static std::string create_json_pattern();

    /// Setup sinks based on configuration
    static std::vector<spdlog::sink_ptr> create_sinks(const LogConfig& config);
};

/// Module-specific logger registry for runtime level control
class ModuleLoggerRegistry {
  private:
    static std::unordered_map<std::string, spdlog::level::level_enum> module_levels_;
    static std::shared_mutex mutex_;
    static spdlog::level::level_enum default_level_;

  public:
    /// Set log level for specific module
    static void set_module_level(const std::string& module, spdlog::level::level_enum level);

    /// Get log level for specific module
    static spdlog::level::level_enum get_module_level(const std::string& module);

    /// Set default level for unregistered modules
    static void set_default_level(spdlog::level::level_enum level);

    /// Set all modules to the same level
    static void set_all_modules_level(spdlog::level::level_enum level);

    /// Get all module levels as JSON
    static nlohmann::json get_module_levels_json();

    /// Load module levels from JSON
    static void load_module_levels_json(const nlohmann::json& config);

    /// Clear all module-specific levels
    static void clear_module_levels();
};

} // namespace finch

// Convenience macros
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(finch::Logger::get(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(finch::Logger::get(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(finch::Logger::get(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(finch::Logger::get(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(finch::Logger::get(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(finch::Logger::get(), __VA_ARGS__)
