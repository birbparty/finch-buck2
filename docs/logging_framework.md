# Finch Logging Framework

The Finch logging framework provides a comprehensive, production-ready logging solution built on top of spdlog with additional features for structured logging, error integration, and OpenTelemetry support.

## Table of Contents

- [Features](#features)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [Basic Logging](#basic-logging)
- [Module-Specific Logging](#module-specific-logging)
- [Logging Helpers](#logging-helpers)
- [Error Integration](#error-integration)
- [OpenTelemetry Integration](#opentelemetry-integration)
- [Performance Considerations](#performance-considerations)
- [Best Practices](#best-practices)
- [API Reference](#api-reference)

## Features

- **High Performance**: Built on spdlog with both synchronous and asynchronous modes
- **Multiple Formats**: Support for both text and JSON output formats
- **Module-Specific Logging**: Per-module log level configuration
- **Rich Helpers**: Timers, progress loggers, scoped logging, and indentation
- **Error Integration**: Seamless integration with the error handling system
- **OpenTelemetry Support**: Built-in OTLP export for logs, metrics, and traces
- **Thread-Safe**: Fully thread-safe logging operations
- **Configurable**: Runtime configuration changes and JSON-based setup
- **File Rotation**: Automatic log file rotation and management

## Quick Start

### Basic Setup

```cpp
#include <finch/core/logging.hpp>

int main() {
    // Initialize with default configuration
    LogConfig config;
    config.console_level = spdlog::level::info;
    config.format = LogConfig::Format::Text;
    config.log_file = "app.log";

    Logger::initialize(config);

    // Basic logging
    LOG_INFO("Application started");
    LOG_DEBUG("Debug information: {}", 42);
    LOG_WARN("Warning message");
    LOG_ERROR("Error occurred: {}", "file not found");

    // Cleanup
    Logger::shutdown();
    return 0;
}
```

### Include Headers

```cpp
#include <finch/core/logging.hpp>          // Core logging functionality
#include <finch/core/logging_helpers.hpp>  // Timers, progress, etc.
#include <finch/core/module_logger.hpp>    // Module-specific logging
#include <finch/core/error_logging.hpp>    // Error integration
#include <finch/core/otel_integration.hpp> // OpenTelemetry support
```

## Configuration

### LogConfig Structure

```cpp
struct LogConfig {
    // Console output configuration
    spdlog::level::level_enum console_level = spdlog::level::info;
    bool use_color = true;

    // File output configuration
    std::string log_file = "";
    size_t max_file_size_mb = 10;
    size_t max_files = 3;

    // Format configuration
    enum class Format { Text, JSON, Both } format = Format::Text;

    // Performance configuration
    enum class Mode { Synchronous, Asynchronous } mode = Mode::Synchronous;
    size_t async_queue_size = 8192;
};
```

### Configuration Examples

```cpp
// Production configuration
LogConfig prod_config;
prod_config.console_level = spdlog::level::warn;
prod_config.format = LogConfig::Format::JSON;
prod_config.log_file = "/var/log/finch/app.log";
prod_config.mode = LogConfig::Mode::Asynchronous;
prod_config.use_color = false;

// Development configuration
LogConfig dev_config;
dev_config.console_level = spdlog::level::debug;
dev_config.format = LogConfig::Format::Text;
dev_config.log_file = "debug.log";
dev_config.use_color = true;

// High-performance configuration
LogConfig perf_config;
perf_config.console_level = spdlog::level::off;
perf_config.mode = LogConfig::Mode::Asynchronous;
perf_config.async_queue_size = 32768;
perf_config.log_file = "performance.log";
```

## Basic Logging

### Log Levels

The framework supports standard log levels:

```cpp
LOG_TRACE("Detailed trace information");
LOG_DEBUG("Debug information: value = {}", value);
LOG_INFO("General information");
LOG_WARN("Warning: {}", warning_msg);
LOG_ERROR("Error occurred: {}", error_details);
LOG_CRITICAL("Critical system error!");
```

### Formatted Logging

Uses fmt library syntax for string formatting:

```cpp
LOG_INFO("Processing file {} ({} bytes)", filename, file_size);
LOG_DEBUG("User {} has {} permissions", user_id, permission_count);
LOG_ERROR("Failed to connect to {}:{} - {}", host, port, error_msg);
```

### Runtime Level Control

```cpp
// Change log level at runtime
Logger::set_level(spdlog::level::debug);

// Check current level
auto current_level = Logger::get_level();

// Flush logs immediately
Logger::flush();
```

## Module-Specific Logging

### Creating Module Loggers

```cpp
#include <finch/core/module_logger.hpp>

// Create module-specific loggers
ModuleLogger parser_log("parser");
ModuleLogger generator_log("generator");

// Use module loggers
parser_log.info("Starting CMake parsing");
parser_log.debug("Found target: {}", target_name);
generator_log.warn("Unsupported feature: {}", feature);
```

### Module Level Configuration

```cpp
// Set individual module levels
ModuleLoggerRegistry::set_module_level("parser", spdlog::level::debug);
ModuleLoggerRegistry::set_module_level("generator", spdlog::level::warn);

// Set default level for all modules
ModuleLoggerRegistry::set_default_level(spdlog::level::info);

// Set all modules to same level
ModuleLoggerRegistry::set_all_modules_level(spdlog::level::error);
```

### JSON Configuration

```cpp
// Load module configuration from JSON
nlohmann::json config = {
    {"default", "info"},
    {"modules", {
        {"parser", "debug"},
        {"generator", "warn"},
        {"analyzer", "trace"}
    }}
};

ModuleLoggerRegistry::load_module_levels_json(config);

// Export current configuration
auto current_config = ModuleLoggerRegistry::get_module_levels_json();
```

## Logging Helpers

### LogTimer - Automatic Duration Logging

```cpp
#include <finch/core/logging_helpers.hpp>

{
    LogTimer timer("File processing operation");
    // ... do work ...
} // Automatically logs duration on destruction

// Custom log level
LogTimer timer("Critical operation", spdlog::level::warn);

// Module-specific timer
ModuleLogger parser("parser");
auto timer = parser.create_timer("Parse CMakeLists.txt");
```

### ProgressLogger - Progress Reporting

```cpp
ProgressLogger progress("Converting files", 100, 25); // Report every 25%

for (size_t i = 0; i <= 100; ++i) {
    // ... process file ...
    progress.update(i);
}
progress.complete();

// Get current progress
double percentage = progress.get_percentage();
```

### LogIndent - Hierarchical Logging

```cpp
LOG_INFO("Starting main operation");
{
    LogIndent indent1;
    LOG_INFO("{}Sub-operation 1", LogIndent::indent());

    {
        LogIndent indent2;
        LOG_INFO("{}Nested operation", LogIndent::indent());
    }

    LOG_INFO("{}Sub-operation 2", LogIndent::indent());
}
LOG_INFO("Main operation complete");
```

### ScopedLogger - Automatic Enter/Exit Logging

```cpp
{
    ScopedLogger scoped("Complex analysis");
    // ... do complex work ...
} // Automatically logs completion with duration
```

## Error Integration

### Direct Error Logging

```cpp
#include <finch/core/error_logging.hpp>

// Create error with context
ParseError error(ParseError::Category::InvalidSyntax, "Missing semicolon");
error.at(SourceLocation("file.cpp", 42, 10))
     .with_context("in function definition")
     .with_help("Add semicolon at end of statement");

// Log the error
log_error(error);                    // Global logging
log_error("parser", error);          // Module-specific logging
```

### Result Type Integration

```cpp
// Log successful results
Result<int, ParseError> success = Ok<int, ParseError>(42);
log_result(success, "Parse operation");

// Log error results
Result<int, ParseError> failure = Err<ParseError, int>(parse_error);
log_result("parser", failure, "Parse integer");
```

### Error Scope - Contextual Error Tracking

```cpp
{
    ErrorScope scope("CMake file processing");

    // Log errors within scope
    scope.log_error(some_error);

    // Log results within scope
    scope.log_result(result, "parse step");

    // Scope automatically logs completion status
}
```

### Structured Error Logging

```cpp
// Log with structured data for analysis
log_error_structured(error);  // Generates JSON error data
```

## OpenTelemetry Integration

### OTEL Configuration

```cpp
#include <finch/core/otel_integration.hpp>

OtelConfig otel_config;
otel_config.enabled = true;
otel_config.endpoint = "http://localhost:4318";
otel_config.service_name = "finch-converter";
otel_config.service_version = "1.0.0";
otel_config.metrics.enabled = true;
otel_config.traces.enabled = true;

OtelIntegration::initialize(otel_config);
```

### Structured Logging with OTLP Export

```cpp
StructuredLogger("Processing project", "info", "converter")
    .with("project_name", "my-project")
    .with("file_count", static_cast<int64_t>(25))
    .with("has_tests", true)
    .with_duration(std::chrono::milliseconds(150))
    .with_error("ParseError", "Invalid syntax")
    .log();  // Automatically exports to OTLP
```

### Metrics

```cpp
// Record different types of metrics
std::map<std::string, std::string> labels = {
    {"operation", "parse"},
    {"file_type", "cmake"}
};

OtelIntegration::record_counter("files_processed", 25, labels);
OtelIntegration::record_histogram("parse_duration_ms", 150.5, labels, "ms");
OtelIntegration::record_metric("memory_usage_mb", 64.2, {{"component", "parser"}}, "MB");
```

### Distributed Tracing

```cpp
{
    auto span = OtelIntegration::start_span("convert_project", {
        {"project.name", "example"},
        {"project.type", "cmake"}
    });

    if (span) {
        span->set_attribute("files.total", static_cast<int64_t>(25));
        span->set_attribute("success", true);

        // ... do work ...

        span->set_status("ok");
    }
} // Span automatically ends and exports
```

### Convenience Macros

```cpp
// Structured logging macros
LOG_STRUCTURED_INFO("Operation completed");
LOG_STRUCTURED_ERROR("Operation failed");

// Span and metrics macros
OTEL_SPAN("operation_name");
OTEL_METRIC_COUNTER("operation_count", 1, {{"type", "success"}});
OTEL_METRIC_HISTOGRAM("operation_duration", 123.45, {}, "ms");
```

## Performance Considerations

### Asynchronous Logging

For high-performance applications, use asynchronous logging:

```cpp
LogConfig config;
config.mode = LogConfig::Mode::Asynchronous;
config.async_queue_size = 32768;  // Larger queue for high throughput
config.console_level = spdlog::level::warn;  // Reduce console output
```

### Log Level Filtering

Filter logs at compile time for maximum performance:

```cpp
// Only include in debug builds
#ifdef DEBUG
LOG_DEBUG("Expensive debug operation: {}", expensive_calculation());
#endif

// Use level checks for expensive operations
if (Logger::get_level() <= spdlog::level::debug) {
    LOG_DEBUG("Debug info: {}", compute_debug_info());
}
```

### Benchmarking Results

Typical performance characteristics:

- Synchronous logging: ~1M messages/second
- Asynchronous logging: ~5M messages/second  
- JSON format: ~20% overhead vs text format
- File output: ~10% overhead vs console only

## Best Practices

### 1. Use Appropriate Log Levels

```cpp
LOG_TRACE("Function entry/exit, variable values");
LOG_DEBUG("Detailed diagnostic information");
LOG_INFO("General application flow");
LOG_WARN("Unexpected but recoverable conditions");
LOG_ERROR("Error conditions that affect functionality");
LOG_CRITICAL("Severe errors that may cause termination");
```

### 2. Include Context

```cpp
// Good: Includes context
LOG_ERROR("Failed to parse file '{}' at line {}: {}", filename, line_no, error_msg);

// Bad: Lacks context
LOG_ERROR("Parse failed");
```

### 3. Use Module Loggers

```cpp
// Good: Module-specific logging
ModuleLogger parser("parser");
parser.error("Failed to parse target '{}'", target_name);

// Acceptable: Global logging with context
LOG_ERROR("[parser] Failed to parse target '{}'", target_name);
```

### 4. Leverage Helpers

```cpp
// Use timers for performance monitoring
{
    LogTimer timer("File conversion");
    convert_file(input, output);
}

// Use progress for long operations
ProgressLogger progress("Processing files", file_count, 10);
for (const auto& file : files) {
    process_file(file);
    progress.update(++processed);
}
```

### 5. Structure for Analysis

```cpp
// Use structured logging for metrics and analysis
StructuredLogger("conversion_complete", "info", "converter")
    .with("input_files", static_cast<int64_t>(input_count))
    .with("output_files", static_cast<int64_t>(output_count))
    .with("success_rate", success_rate)
    .with_duration(conversion_time)
    .log();
```

### 6. Handle Errors Consistently

```cpp
// Integrate with error system
try {
    auto result = parse_file(filename);
    log_result("parser", result, "parse file");

} catch (const std::exception& e) {
    LOG_ERROR("Unexpected exception in parser: {}", e.what());
    throw;
}
```

## API Reference

### Core Logger

```cpp
class Logger {
    static void initialize(const LogConfig& config);
    static void shutdown();
    static bool is_initialized() noexcept;
    static std::shared_ptr<spdlog::logger> get();
    static void set_level(spdlog::level::level_enum level);
    static spdlog::level::level_enum get_level();
    static void set_pattern(const std::string& pattern);
    static LogConfig get_config();
    static void flush();
};
```

### Module Logger

```cpp
class ModuleLogger {
    explicit ModuleLogger(std::string module_name);

    template<typename... Args>
    void trace(spdlog::format_string_t<Args...> fmt, Args&&... args);
    // ... similar for debug, info, warn, error, critical

    LogTimer create_timer(const std::string& operation_name);
    ProgressLogger create_progress(const std::string& operation, size_t total, size_t report_interval = 10);
    ScopedLogger create_scoped(const std::string& operation_name);
};
```

### Logging Helpers

```cpp
class LogTimer {
    LogTimer(std::string operation, spdlog::level::level_enum level = spdlog::level::info);
    ~LogTimer();  // Logs duration
};

class ProgressLogger {
    ProgressLogger(std::string operation, size_t total, size_t report_interval = 10);
    void update(size_t current);
    void complete();
    double get_percentage() const noexcept;
};

class LogIndent {
    LogIndent();
    ~LogIndent();
    static std::string indent();
    static int level() noexcept;
};

class ScopedLogger {
    explicit ScopedLogger(std::string operation);
    ~ScopedLogger();  // Logs completion
};
```

### Error Integration

```cpp
// Error logging functions
template<typename ErrorType>
void log_error(const ErrorType& error);

template<typename ErrorType>
void log_error(const std::string& module, const ErrorType& error);

template<typename T, typename E>
void log_result(const Result<T, E>& result, const std::string& operation);

// Error scope
class ErrorScope {
    explicit ErrorScope(std::string operation);
    void mark_error();
    template<typename ErrorType> void log_error(const ErrorType& error);
    template<typename T, typename E> void log_result(const Result<T, E>& result, const std::string& sub_operation = "");
};
```

### OpenTelemetry Integration

```cpp
class OtelIntegration {
    static bool initialize(const OtelConfig& config);
    static void shutdown();
    static bool is_enabled() noexcept;

    static bool export_log(const LogEntry& entry);
    static bool record_metric(const std::string& name, double value, const std::map<std::string, std::string>& labels = {}, const std::string& unit = "");
    static bool record_counter(const std::string& name, int64_t value, const std::map<std::string, std::string>& labels = {});
    static bool record_histogram(const std::string& name, double value, const std::map<std::string, std::string>& labels = {}, const std::string& unit = "");
    static std::unique_ptr<Span> start_span(const std::string& name, const std::map<std::string, std::string>& attributes = {});
};

class StructuredLogger {
    explicit StructuredLogger(std::string message, std::string level = "info", std::string module = "");
    StructuredLogger& with(const std::string& key, const std::string& value);
    StructuredLogger& with(const std::string& key, int64_t value);
    StructuredLogger& with(const std::string& key, double value);
    StructuredLogger& with(const std::string& key, bool value);
    StructuredLogger& with_duration(std::chrono::milliseconds ms);
    StructuredLogger& with_error(const std::string& error_type, const std::string& error_message);
    void log();
};
```

---

For more examples, see `examples/logging_example.cpp` and the test suite in `test/core/logging_test.cpp`.
