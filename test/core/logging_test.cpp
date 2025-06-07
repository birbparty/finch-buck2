#include <chrono>
#include <filesystem>
#include <finch/core/error_logging.hpp>
#include <finch/core/logging.hpp>
#include <finch/core/logging_helpers.hpp>
#include <finch/core/module_logger.hpp>
#include <finch/core/otel_integration.hpp>
#include <gtest/gtest.h>
#include <thread>

using namespace finch;

class LoggingTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Initialize with trace level for testing
        LogConfig config;
        config.console_level = spdlog::level::trace;
        config.format = LogConfig::Format::Text;
        config.use_color = false; // Disable color for test consistency
        Logger::initialize(config);
    }

    void TearDown() override {
        Logger::shutdown();
    }
};

TEST_F(LoggingTest, BasicLogging) {
    // Test all log levels
    LOG_TRACE("Trace message: {}", 42);
    LOG_DEBUG("Debug message: {}", "test");
    LOG_INFO("Info message: {}", 3.14);
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");
    LOG_CRITICAL("Critical message");

    // Should not crash - success
    SUCCEED();
}

TEST_F(LoggingTest, LogLevelControl) {
    Logger::set_level(spdlog::level::warn);
    EXPECT_EQ(Logger::get_level(), spdlog::level::warn);

    Logger::set_level(spdlog::level::info);
    EXPECT_EQ(Logger::get_level(), spdlog::level::info);
}

TEST_F(LoggingTest, LoggerConfiguration) {
    auto config = Logger::get_config();
    EXPECT_EQ(config.format, LogConfig::Format::Text);
    EXPECT_FALSE(config.use_color);
    EXPECT_EQ(config.mode, LogConfig::Mode::Synchronous);
}

TEST_F(LoggingTest, FileLogging) {
    // Test file logging configuration
    std::string test_log_file = "test_finch.log";

    LogConfig config;
    config.console_level = spdlog::level::info;
    config.log_file = test_log_file;
    config.format = LogConfig::Format::JSON;

    Logger::initialize(config);

    LOG_INFO("Test message for file logging");
    Logger::flush();

    // Check if file was created
    EXPECT_TRUE(std::filesystem::exists(test_log_file));

    // Clean up
    std::filesystem::remove(test_log_file);
}

TEST_F(LoggingTest, AsyncLogging) {
    LogConfig config;
    config.console_level = spdlog::level::info;
    config.mode = LogConfig::Mode::Asynchronous;
    config.async_queue_size = 1024;

    Logger::initialize(config);

    LOG_INFO("Async test message");
    Logger::flush();

    // Should not crash
    SUCCEED();
}

class LoggingHelpersTest : public ::testing::Test {
  protected:
    void SetUp() override {
        LogConfig config;
        config.console_level = spdlog::level::trace;
        Logger::initialize(config);
    }

    void TearDown() override {
        Logger::shutdown();
    }
};

TEST_F(LoggingHelpersTest, LogTimer) {
    {
        LogTimer timer("Test operation", spdlog::level::info);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } // Timer should log on destruction

    SUCCEED();
}

TEST_F(LoggingHelpersTest, ProgressLogger) {
    ProgressLogger progress("Processing items", 100, 25); // Report every 25%

    for (size_t i = 0; i <= 100; i += 25) {
        progress.update(i);
    }
    progress.complete();

    EXPECT_EQ(progress.get_percentage(), 100.0);
}

TEST_F(LoggingHelpersTest, LogIndent) {
    {
        LogIndent indent1;
        LOG_INFO("{}Level 1", LogIndent::indent());
        EXPECT_EQ(LogIndent::level(), 1);

        {
            LogIndent indent2;
            LOG_INFO("{}Level 2", LogIndent::indent());
            EXPECT_EQ(LogIndent::level(), 2);
        }

        EXPECT_EQ(LogIndent::level(), 1);
    }

    EXPECT_EQ(LogIndent::level(), 0);
}

TEST_F(LoggingHelpersTest, ScopedLogger) {
    {
        ScopedLogger scoped("Test operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } // Should log completion

    SUCCEED();
}

class ModuleLoggerTest : public ::testing::Test {
  protected:
    void SetUp() override {
        LogConfig config;
        config.console_level = spdlog::level::trace;
        Logger::initialize(config);

        // Clear any existing module levels
        ModuleLoggerRegistry::clear_module_levels();
    }

    void TearDown() override {
        Logger::shutdown();
    }
};

TEST_F(ModuleLoggerTest, ModuleLevelControl) {
    ModuleLoggerRegistry::set_module_level("parser", spdlog::level::debug);
    ModuleLoggerRegistry::set_module_level("generator", spdlog::level::warn);

    EXPECT_EQ(ModuleLoggerRegistry::get_module_level("parser"), spdlog::level::debug);
    EXPECT_EQ(ModuleLoggerRegistry::get_module_level("generator"), spdlog::level::warn);
    EXPECT_EQ(ModuleLoggerRegistry::get_module_level("unknown"), spdlog::level::info); // default
}

TEST_F(ModuleLoggerTest, ModuleLogging) {
    ModuleLogger parser_log("parser");
    ModuleLogger gen_log("generator");

    parser_log.debug("Parsing file: {}", "CMakeLists.txt");
    parser_log.info("Found {} targets", 5);
    parser_log.warn("Unsupported feature: {}", "CUSTOM_COMMAND");

    gen_log.info("Generating Buck2 file");
    gen_log.error("Failed to generate rule for target: {}", "test_target");

    SUCCEED();
}

TEST_F(ModuleLoggerTest, ModuleHelpers) {
    ModuleLogger parser_log("parser");

    // Test timer creation
    {
        auto timer = parser_log.create_timer("Parse operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // Test progress creation
    auto progress = parser_log.create_progress("Processing files", 10);
    progress.update(5);
    progress.complete();

    // Test scoped logger creation
    {
        auto scoped = parser_log.create_scoped("Complex operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    SUCCEED();
}

TEST_F(ModuleLoggerTest, JSONConfiguration) {
    nlohmann::json config = {
        {"default", "info"},
        {"modules", {{"parser", "debug"}, {"generator", "warn"}, {"analyzer", "trace"}}}};

    ModuleLoggerRegistry::load_module_levels_json(config);

    EXPECT_EQ(ModuleLoggerRegistry::get_module_level("parser"), spdlog::level::debug);
    EXPECT_EQ(ModuleLoggerRegistry::get_module_level("generator"), spdlog::level::warn);
    EXPECT_EQ(ModuleLoggerRegistry::get_module_level("analyzer"), spdlog::level::trace);

    auto exported = ModuleLoggerRegistry::get_module_levels_json();
    EXPECT_EQ(exported["default"], "info");
    EXPECT_EQ(exported["modules"]["parser"], "debug");
}

class ErrorLoggingTest : public ::testing::Test {
  protected:
    void SetUp() override {
        LogConfig config;
        config.console_level = spdlog::level::trace;
        Logger::initialize(config);
    }

    void TearDown() override {
        Logger::shutdown();
    }
};

TEST_F(ErrorLoggingTest, ErrorLogging) {
    ParseError error(ParseError::Category::InvalidSyntax, "Missing semicolon");
    error.at(SourceLocation("test.cmake", 10, 5))
        .with_context("in function definition")
        .with_help("Add semicolon at end of line");

    log_error(error);
    log_error("parser", error);

    SUCCEED();
}

TEST_F(ErrorLoggingTest, ResultLogging) {
    // Test successful result
    Result<int, ParseError> success_result = Ok<int, ParseError>(42);
    log_result(success_result, "Parse integer");
    log_result("parser", success_result, "Parse integer");

    // Test error result
    auto error = ParseError(ParseError::Category::UnexpectedToken, "Expected number");
    Result<int, ParseError> error_result = Err<ParseError, int>(std::move(error));
    log_result(error_result, "Parse integer");
    log_result("parser", error_result, "Parse integer");

    SUCCEED();
}

TEST_F(ErrorLoggingTest, ErrorScope) {
    {
        ErrorScope scope("Test operation");

        ParseError error(ParseError::Category::InvalidSyntax, "Test error");
        scope.log_error(error);

        Result<int, ParseError> result =
            Err<ParseError, int>(ParseError(ParseError::Category::UnexpectedEOF, "Unexpected end"));
        scope.log_result(result, "sub-operation");
    }

    SUCCEED();
}

class OtelIntegrationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        LogConfig config;
        config.console_level = spdlog::level::trace;
        Logger::initialize(config);
    }

    void TearDown() override {
        OtelIntegration::shutdown();
        Logger::shutdown();
    }
};

TEST_F(OtelIntegrationTest, InitializationAndShutdown) {
    OtelConfig config;
    config.enabled = true;
    config.endpoint = "http://localhost:4318";
    config.service_name = "test-service";

    EXPECT_TRUE(OtelIntegration::initialize(config));
    EXPECT_TRUE(OtelIntegration::is_enabled());

    OtelIntegration::shutdown();
    EXPECT_FALSE(OtelIntegration::is_enabled());
}

TEST_F(OtelIntegrationTest, LogExport) {
    OtelConfig config;
    config.enabled = true;
    OtelIntegration::initialize(config);

    LogEntry entry("Test log message", "info", "test-module");
    entry.with_attribute("key1", "value1").with_attribute("key2", "value2");

    // Note: This will return false since we don't have a real endpoint
    // but it tests the payload generation
    bool result = OtelIntegration::export_log(entry);
    EXPECT_FALSE(result); // Expected since no real endpoint
}

TEST_F(OtelIntegrationTest, MetricRecording) {
    OtelConfig config;
    config.enabled = true;
    config.metrics.enabled = true;
    OtelIntegration::initialize(config);

    std::map<std::string, std::string> labels = {{"endpoint", "/test"}, {"method", "GET"}};

    // Test different metric types
    EXPECT_FALSE(OtelIntegration::record_metric("test_gauge", 42.0, labels, "bytes"));
    EXPECT_FALSE(OtelIntegration::record_counter("test_counter", 100, labels));
    EXPECT_FALSE(OtelIntegration::record_histogram("test_histogram", 123.45, labels, "ms"));
}

TEST_F(OtelIntegrationTest, SpanCreation) {
    OtelConfig config;
    config.enabled = true;
    config.traces.enabled = true;
    OtelIntegration::initialize(config);

    std::map<std::string, std::string> attributes = {{"operation", "test"},
                                                     {"component", "logging_test"}};

    auto span = OtelIntegration::start_span("test_operation", attributes);
    EXPECT_NE(span, nullptr);

    span->set_attribute("result", "success");
    span->set_attribute("duration", static_cast<int64_t>(100));
    span->set_attribute("error", false);
    span->end();
}

TEST_F(OtelIntegrationTest, StructuredLogging) {
    OtelConfig config;
    config.enabled = true;
    OtelIntegration::initialize(config);

    StructuredLogger("Test structured message", "info", "test")
        .with("operation", "test_op")
        .with("duration", static_cast<int64_t>(42))
        .with("success", true)
        .with_duration(std::chrono::milliseconds(100))
        .with_error("TestError", "Test error message")
        .log();

    SUCCEED();
}

// Performance test
TEST_F(LoggingTest, PerformanceTest) {
    Logger::set_level(spdlog::level::info);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i) {
        LOG_INFO("Performance test message {}", i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete within reasonable time (adjust threshold as needed)
    EXPECT_LT(duration.count(), 1000); // Less than 1 second for 10k messages

    LOG_INFO("Performance test: {} messages in {}ms", 10000, duration.count());
}

// Thread safety test
TEST_F(LoggingTest, ThreadSafetyTest) {
    const int num_threads = 4;
    const int messages_per_thread = 1000;

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, messages_per_thread]() {
            for (int i = 0; i < messages_per_thread; ++i) {
                LOG_INFO("Thread {} message {}", t, i);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    SUCCEED();
}
