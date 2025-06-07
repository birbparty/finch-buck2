#include <chrono>
#include <finch/core/logging.hpp>
#include <finch/core/logging_helpers.hpp>
#include <finch/core/module_logger.hpp>
#include <thread>

using namespace finch;

int main() {
    // Initialize logging with basic configuration
    LogConfig config;
    config.console_level = spdlog::level::info;
    config.format = LogConfig::Format::Text;
    config.use_color = true;
    config.log_file = "simple_example.log";

    Logger::initialize(config);

    // Basic logging
    LOG_INFO("Simple Logging Example Started");
    LOG_DEBUG("This debug message won't show (level is info)");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");

    // Change log level
    Logger::set_level(spdlog::level::debug);
    LOG_DEBUG("Now this debug message will show");

    // Module-specific logging
    ModuleLogger parser("parser");
    ModuleLogger generator("generator");

    parser.info("Starting CMake parsing");
    parser.debug("Found target: main_executable");
    generator.info("Generating Buck2 rules");

    // Logging helpers
    {
        LogTimer timer("File processing");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } // Timer logs duration automatically

    // Progress logging
    ProgressLogger progress("Converting files", 100, 25);
    for (size_t i = 0; i <= 100; i += 25) {
        progress.update(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    progress.complete();

    // Scoped logging
    {
        ScopedLogger scoped("Complex operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } // Logs completion automatically

    LOG_INFO("Simple Logging Example Completed");

    Logger::shutdown();
    return 0;
}
