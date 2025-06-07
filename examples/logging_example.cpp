#include <chrono>
#include <finch/core/error_logging.hpp>
#include <finch/core/logging.hpp>
#include <finch/core/logging_helpers.hpp>
#include <finch/core/module_logger.hpp>
#include <finch/core/otel_integration.hpp>
#include <thread>
#include <vector>

using namespace finch;

// Example function that demonstrates basic logging
void demonstrate_basic_logging() {
    LOG_INFO("=== Basic Logging Demo ===");

    LOG_TRACE("This is a trace message with value: {}", 42);
    LOG_DEBUG("Debug message: processing file {}", "CMakeLists.txt");
    LOG_INFO("Info: Found {} targets in project", 15);
    LOG_WARN("Warning: Deprecated feature used in line {}", 123);
    LOG_ERROR("Error: Failed to parse expression at column {}", 25);
    LOG_CRITICAL("Critical: System out of memory!");
}

// Example function demonstrating logging helpers
void demonstrate_logging_helpers() {
    LOG_INFO("=== Logging Helpers Demo ===");

    // Timer example
    {
        LogTimer timer("File processing operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } // Timer logs duration on destruction

    // Progress logger example
    ProgressLogger progress("Converting files", 100, 20); // Report every 20%
    for (size_t i = 0; i <= 100; i += 20) {
        progress.update(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    progress.complete();

    // Indented logging example
    LOG_INFO("Processing main project");
    {
        LogIndent indent1;
        LOG_INFO("{}Parsing CMakeLists.txt", LogIndent::indent());
        LOG_INFO("{}Found {} targets", LogIndent::indent(), 5);

        {
            LogIndent indent2;
            LOG_INFO("{}Analyzing target dependencies", LogIndent::indent());
            LOG_INFO("{}Processing target: main_app", LogIndent::indent());
        }

        LOG_INFO("{}Generating Buck2 rules", LogIndent::indent());
    }
    LOG_INFO("Project conversion complete");

    // Scoped logger example
    {
        ScopedLogger scoped("Complex analysis operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        LOG_INFO("Performing complex calculations...");
    } // Logs completion automatically
}

// Example function demonstrating module-specific logging
void demonstrate_module_logging() {
    LOG_INFO("=== Module Logging Demo ===");

    // Configure different log levels for different modules
    ModuleLoggerRegistry::set_module_level("parser", spdlog::level::debug);
    ModuleLoggerRegistry::set_module_level("generator", spdlog::level::warn);
    ModuleLoggerRegistry::set_module_level("analyzer", spdlog::level::trace);

    // Create module-specific loggers
    ModuleLogger parser_log("parser");
    ModuleLogger generator_log("generator");
    ModuleLogger analyzer_log("analyzer");

    // Parser module logging
    parser_log.info("Starting CMake file parsing");
    parser_log.debug("Parsing file: {}", "src/CMakeLists.txt");
    parser_log.debug("Found target: {}", "main_executable");
    parser_log.warn("Unsupported CMake feature: CUSTOM_COMMAND");

    // Generator module logging (only warns and errors will show due to level)
    generator_log.debug("This debug message won't show");
    generator_log.info("This info message won't show");
    generator_log.warn("Generating Buck2 rule for target: {}", "test_lib");
    generator_log.error("Failed to generate rule for target: {}", "broken_target");

    // Analyzer module logging
    analyzer_log.trace("Analyzing dependency graph");
    analyzer_log.debug("Found circular dependency: A -> B -> A");
    analyzer_log.info("Dependency analysis complete");

    // Using module logger helpers
    {
        auto timer = parser_log.create_timer("Parse all files");
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
    }

    auto progress = generator_log.create_progress("Generate rules", 50);
    for (int i = 0; i <= 50; i += 10) {
        progress.update(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    progress.complete();
}

// Example function demonstrating error logging integration
void demonstrate_error_logging() {
    LOG_INFO("=== Error Logging Demo ===");

    // Create an error with context
    ParseError parse_error(ParseError::Category::InvalidSyntax, "Missing closing parenthesis");
    parse_error.at(SourceLocation("CMakeLists.txt", 45, 12))
        .with_context("in add_executable() function call")
        .with_context("while parsing target dependencies")
        .with_help("Add closing ')' after target name");

    // Log the error
    log_error(parse_error);
    log_error("parser", parse_error);

    // Example with Result types
    Result<int, ParseError> success_result = Ok<int, ParseError>(42);
    log_result(success_result, "Parse target count");

    ParseError error2(ParseError::Category::UnexpectedToken, "Expected string literal");
    Result<std::string, ParseError> error_result = Err<ParseError, std::string>(std::move(error2));
    log_result("parser", error_result, "Parse target name");

    // Error scope example
    {
        ErrorScope scope("CMake file processing");

        // Simulate some operations that might fail
        ParseError scope_error(ParseError::Category::UnexpectedEOF, "Unexpected end of file");
        scope.log_error(scope_error);

        Result<bool, ParseError> result = Err<ParseError, bool>(
            ParseError(ParseError::Category::InvalidSyntax, "Malformed expression"));
        scope.log_result(result, "validate syntax");
    } // Logs scope completion with error status
}

// Example function demonstrating OpenTelemetry integration
void demonstrate_otel_integration() {
    LOG_INFO("=== OpenTelemetry Integration Demo ===");

    // Configure OTEL (in practice, this would be done at startup)
    OtelConfig otel_config;
    otel_config.enabled = true;
    otel_config.endpoint = "http://localhost:4318";
    otel_config.service_name = "finch-logging-example";
    otel_config.service_version = "1.0.0";
    otel_config.metrics.enabled = true;
    otel_config.traces.enabled = true;

    if (OtelIntegration::initialize(otel_config)) {
        LOG_INFO("OpenTelemetry integration initialized");
    } else {
        LOG_WARN("OpenTelemetry integration failed to initialize (endpoint unreachable)");
    }

    // Structured logging with OTLP export
    StructuredLogger("Processing CMake project", "info", "converter")
        .with("project_name", "example_project")
        .with("file_count", static_cast<int64_t>(25))
        .with("has_tests", true)
        .with_duration(std::chrono::milliseconds(150))
        .log();

    // Record metrics
    std::map<std::string, std::string> labels = {{"operation", "parse"}, {"file_type", "cmake"}};

    OtelIntegration::record_counter("files_processed", 25, labels);
    OtelIntegration::record_histogram("parse_duration_ms", 150.5, labels, "ms");
    OtelIntegration::record_metric("memory_usage_mb", 64.2, {{"component", "parser"}}, "MB");

    // Distributed tracing example
    {
        auto span =
            OtelIntegration::start_span("convert_project", {{"project.name", "example_project"},
                                                            {"project.language", "cmake"}});

        if (span) {
            span->set_attribute("files.total", static_cast<int64_t>(25));
            span->set_attribute("files.processed", static_cast<int64_t>(23));
            span->set_attribute("conversion.success", true);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            span->set_status("ok");
        }
    } // Span ends automatically

    LOG_INFO("OpenTelemetry telemetry sent (check your OTLP collector)");
}

// Example function demonstrating different log formats and configurations
void demonstrate_log_configuration() {
    LOG_INFO("=== Log Configuration Demo ===");

    // Show current configuration
    auto current_config = Logger::get_config();
    LOG_INFO("Current log format: {}",
             current_config.format == LogConfig::Format::JSON ? "JSON" : "Text");
    LOG_INFO("Current log level: {}", spdlog::level::to_string_view(Logger::get_level()));

    // Demonstrate level changes
    LOG_INFO("Setting log level to WARNING...");
    Logger::set_level(spdlog::level::warn);

    LOG_DEBUG("This debug message should not appear");
    LOG_INFO("This info message should not appear");
    LOG_WARN("This warning message should appear");
    LOG_ERROR("This error message should appear");

    // Reset to info level
    Logger::set_level(spdlog::level::info);
    LOG_INFO("Log level reset to INFO");

    // Demonstrate JSON configuration for modules
    nlohmann::json module_config = {
        {"default", "info"},
        {"modules", {{"parser", "debug"}, {"generator", "warn"}, {"analyzer", "trace"}}}};

    ModuleLoggerRegistry::load_module_levels_json(module_config);
    LOG_INFO("Module logging configuration loaded from JSON");

    // Export current module configuration
    auto exported_config = ModuleLoggerRegistry::get_module_levels_json();
    LOG_INFO("Current module config: {}", exported_config.dump());
}

// Performance demonstration
void demonstrate_performance() {
    LOG_INFO("=== Performance Demo ===");

    const int message_count = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < message_count; ++i) {
        LOG_DEBUG("Performance test message #{}", i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    LOG_INFO("Logged {} messages in {}ms ({:.2f} msg/sec)", message_count, duration.count(),
             message_count * 1000.0 / duration.count());
}

int main() {
    // Initialize logging with a comprehensive configuration
    LogConfig config;
    config.console_level = spdlog::level::trace;
    config.format = LogConfig::Format::Text;
    config.use_color = true;
    config.mode = LogConfig::Mode::Synchronous;
    config.log_file = "finch_example.log";
    config.max_file_size_mb = 10;
    config.max_files = 3;

    Logger::initialize(config);

    LOG_INFO("Finch Logging Framework Example");
    LOG_INFO("=====================================");

    try {
        demonstrate_basic_logging();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        demonstrate_logging_helpers();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        demonstrate_module_logging();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        demonstrate_error_logging();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        demonstrate_otel_integration();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        demonstrate_log_configuration();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        demonstrate_performance();

    } catch (const std::exception& e) {
        LOG_CRITICAL("Example failed with exception: {}", e.what());
        return 1;
    }

    LOG_INFO("=====================================");
    LOG_INFO("Logging example completed successfully!");
    LOG_INFO("Check 'finch_example.log' for file output");

    // Cleanup
    OtelIntegration::shutdown();
    Logger::shutdown();

    return 0;
}
