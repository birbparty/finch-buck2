#include <finch/core/logging.hpp>
#include <iomanip>
#include <spdlog/async.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <sstream>

namespace finch {

// Static member definitions
std::shared_ptr<spdlog::logger> Logger::logger_;
LogConfig Logger::config_;
std::shared_mutex Logger::config_mutex_;
bool Logger::initialized_ = false;

std::unordered_map<std::string, spdlog::level::level_enum> ModuleLoggerRegistry::module_levels_;
std::shared_mutex ModuleLoggerRegistry::mutex_;
spdlog::level::level_enum ModuleLoggerRegistry::default_level_ = spdlog::level::info;

// Custom JSON formatter for structured logging
class JsonFormatter : public spdlog::custom_flag_formatter {
  public:
    void format(const spdlog::details::log_msg& msg, const std::tm&,
                spdlog::memory_buf_t& dest) override {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        // Create ISO 8601 timestamp
        std::tm* tm_ptr = std::gmtime(&time_t);
        std::ostringstream timestamp_stream;
        timestamp_stream << std::put_time(tm_ptr, "%Y-%m-%dT%H:%M:%S");
        timestamp_stream << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";

        // Escape message content for JSON
        std::string escaped_message = fmt::to_string(msg.payload);
        // Simple JSON escaping
        size_t pos = 0;
        while ((pos = escaped_message.find('"', pos)) != std::string::npos) {
            escaped_message.replace(pos, 1, "\\\"");
            pos += 2;
        }
        while ((pos = escaped_message.find('\n', pos)) != std::string::npos) {
            escaped_message.replace(pos, 1, "\\n");
            pos += 2;
        }

        std::string json =
            fmt::format(R"({{"timestamp":"{}","level":"{}","message":"{}","thread_id":{}}})",
                        timestamp_stream.str(), spdlog::level::to_string_view(msg.level),
                        escaped_message, msg.thread_id);

        dest.append(json.data(), json.data() + json.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return std::make_unique<JsonFormatter>();
    }
};

void Logger::initialize(const LogConfig& config) {
    std::unique_lock lock(config_mutex_);

    if (initialized_) {
        shutdown();
    }

    config_ = config;

    try {
        auto sinks = create_sinks(config);

        if (config.mode == LogConfig::Mode::Asynchronous) {
            // Initialize async logging
            spdlog::init_thread_pool(config.async_queue_size, 1);
            logger_ = std::make_shared<spdlog::async_logger>("finch", sinks.begin(), sinks.end(),
                                                             spdlog::thread_pool(),
                                                             spdlog::async_overflow_policy::block);
        } else {
            // Synchronous logging
            logger_ = std::make_shared<spdlog::logger>("finch", sinks.begin(), sinks.end());
        }

        logger_->set_level(spdlog::level::trace);
        logger_->flush_on(spdlog::level::warn);

        // Set pattern based on format
        if (config.format == LogConfig::Format::Text || config.format == LogConfig::Format::Both) {
            logger_->set_pattern(create_text_pattern());
        }

        // Register as default
        spdlog::set_default_logger(logger_);

        initialized_ = true;

        LOG_INFO("Logging system initialized (mode: {}, format: {}, level: {})",
                 config.mode == LogConfig::Mode::Asynchronous ? "async" : "sync",
                 config.format == LogConfig::Format::JSON   ? "json"
                 : config.format == LogConfig::Format::Both ? "both"
                                                            : "text",
                 spdlog::level::to_string_view(config.console_level));

    } catch (const std::exception& e) {
        // Fallback to console logging
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        logger_ = std::make_shared<spdlog::logger>("finch", console_sink);
        logger_->set_level(spdlog::level::info);
        spdlog::set_default_logger(logger_);
        initialized_ = true;

        LOG_ERROR(
            "Failed to initialize logging with configuration: {}. Using fallback console logging.",
            e.what());
    }
}

void Logger::shutdown() {
    std::unique_lock lock(config_mutex_);

    if (initialized_) {
        LOG_INFO("Shutting down logging system");

        if (logger_) {
            logger_->flush();
        }

        if (config_.mode == LogConfig::Mode::Asynchronous) {
            spdlog::shutdown();
        }

        logger_.reset();
        initialized_ = false;
    }
}

bool Logger::is_initialized() noexcept {
    std::shared_lock lock(config_mutex_);
    return initialized_;
}

std::shared_ptr<spdlog::logger> Logger::get() {
    std::shared_lock lock(config_mutex_);

    if (!initialized_) {
        // Auto-initialize with default config
        lock.unlock();
        initialize(LogConfig{});
        lock.lock();
    }

    return logger_;
}

void Logger::set_level(spdlog::level::level_enum level) {
    if (auto logger = get()) {
        logger->set_level(level);
        std::unique_lock lock(config_mutex_);
        config_.console_level = level;
    }
}

spdlog::level::level_enum Logger::get_level() {
    if (auto logger = get()) {
        return logger->level();
    }
    return spdlog::level::info;
}

void Logger::set_pattern(const std::string& pattern) {
    if (auto logger = get()) {
        logger->set_pattern(pattern);
    }
}

LogConfig Logger::get_config() {
    std::shared_lock lock(config_mutex_);
    return config_;
}

void Logger::update_config(const LogConfig& new_config) {
    // For now, require restart for config changes
    initialize(new_config);
}

void Logger::flush() {
    if (auto logger = get()) {
        logger->flush();
    }
}

std::string Logger::create_text_pattern() {
    return "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v";
}

std::string Logger::create_json_pattern() {
    return "%v"; // JSON formatter handles the structure
}

std::vector<spdlog::sink_ptr> Logger::create_sinks(const LogConfig& config) {
    std::vector<spdlog::sink_ptr> sinks;

    // Console sink
    auto console_sink = config.use_color
                            ? std::static_pointer_cast<spdlog::sinks::sink>(
                                  std::make_shared<spdlog::sinks::stdout_color_sink_mt>())
                            : std::static_pointer_cast<spdlog::sinks::sink>(
                                  std::make_shared<spdlog::sinks::stdout_sink_mt>());

    console_sink->set_level(config.console_level);

    if (config.format == LogConfig::Format::JSON) {
        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<JsonFormatter>('*');
        formatter->set_pattern("%*");
        console_sink->set_formatter(std::move(formatter));
    } else {
        console_sink->set_pattern(create_text_pattern());
    }

    sinks.push_back(console_sink);

    // File sink (if specified)
    if (!config.log_file.empty()) {
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            config.log_file, config.max_file_size_mb * 1024 * 1024, config.max_files);

        file_sink->set_level(spdlog::level::trace);

        if (config.format == LogConfig::Format::JSON || config.format == LogConfig::Format::Both) {
            auto formatter = std::make_unique<spdlog::pattern_formatter>();
            formatter->add_flag<JsonFormatter>('*');
            formatter->set_pattern("%*");
            file_sink->set_formatter(std::move(formatter));
        } else {
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
        }

        sinks.push_back(file_sink);
    }

    return sinks;
}

// ModuleLoggerRegistry implementation

void ModuleLoggerRegistry::set_module_level(const std::string& module,
                                            spdlog::level::level_enum level) {
    std::unique_lock lock(mutex_);
    module_levels_[module] = level;
}

spdlog::level::level_enum ModuleLoggerRegistry::get_module_level(const std::string& module) {
    std::shared_lock lock(mutex_);
    auto it = module_levels_.find(module);
    return it != module_levels_.end() ? it->second : default_level_;
}

void ModuleLoggerRegistry::set_default_level(spdlog::level::level_enum level) {
    std::unique_lock lock(mutex_);
    default_level_ = level;
}

void ModuleLoggerRegistry::set_all_modules_level(spdlog::level::level_enum level) {
    std::unique_lock lock(mutex_);
    default_level_ = level;
    for (auto& [module, module_level] : module_levels_) {
        module_level = level;
    }
}

nlohmann::json ModuleLoggerRegistry::get_module_levels_json() {
    std::shared_lock lock(mutex_);
    nlohmann::json result;

    result["default"] = spdlog::level::to_string_view(default_level_);

    for (const auto& [module, level] : module_levels_) {
        result["modules"][module] = spdlog::level::to_string_view(level);
    }

    return result;
}

void ModuleLoggerRegistry::load_module_levels_json(const nlohmann::json& config) {
    std::unique_lock lock(mutex_);

    if (config.contains("default")) {
        default_level_ = spdlog::level::from_str(config["default"].get<std::string>());
    }

    if (config.contains("modules") && config["modules"].is_object()) {
        for (const auto& [module, level_str] : config["modules"].items()) {
            if (level_str.is_string()) {
                module_levels_[module] = spdlog::level::from_str(level_str.get<std::string>());
            }
        }
    }
}

void ModuleLoggerRegistry::clear_module_levels() {
    std::unique_lock lock(mutex_);
    module_levels_.clear();
    default_level_ = spdlog::level::info;
}

} // namespace finch
