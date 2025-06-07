#pragma once

#include "logging.hpp"
#include <chrono>
#include <string>
#include <string_view>

namespace finch {

/// RAII timer for performance logging
class LogTimer {
  private:
    std::string operation_;
    std::chrono::steady_clock::time_point start_;
    spdlog::level::level_enum level_;

  public:
    explicit LogTimer(std::string operation, spdlog::level::level_enum level = spdlog::level::debug)
        : operation_(std::move(operation)), start_(std::chrono::steady_clock::now()),
          level_(level) {
        LOG_TRACE("Starting: {}", operation_);
    }

    ~LogTimer() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();

        Logger::get()->log(level_, "{} completed in {}ms", operation_, duration);
    }

    /// Get elapsed time without logging
    [[nodiscard]] std::chrono::milliseconds elapsed() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_);
    }
};

/// Progress logging helper
class ProgressLogger {
  private:
    std::string task_;
    size_t total_;
    size_t current_ = 0;
    size_t last_percent_ = 0;
    size_t report_interval_ = 10; // Report every 10%

  public:
    ProgressLogger(std::string task, size_t total, size_t report_interval = 10)
        : task_(std::move(task)), total_(total), report_interval_(report_interval) {
        LOG_INFO("Starting {}: {} items", task_, total_);
    }

    void update(size_t current) {
        current_ = current;
        if (total_ == 0) {
            return;
        }

        size_t percent = (current_ * 100) / total_;

        if (percent >= last_percent_ + report_interval_ && percent <= 100) {
            LOG_INFO("{}: {}% complete ({}/{})", task_, percent, current_, total_);
            last_percent_ = percent;
        }
    }

    void complete() {
        LOG_INFO("{}: 100% complete ({} items)", task_, total_);
    }

    /// Set custom reporting interval
    void set_report_interval(size_t interval) {
        report_interval_ = interval;
    }

    /// Get current progress percentage
    [[nodiscard]] double get_percentage() const {
        return total_ > 0 ? (static_cast<double>(current_) * 100.0) / static_cast<double>(total_)
                          : 0.0;
    }
};

/// Indent helper for hierarchical logging
class LogIndent {
  private:
    static thread_local int indent_level_;

  public:
    LogIndent() {
        ++indent_level_;
    }

    ~LogIndent() {
        --indent_level_;
    }

    /// Get current indentation string
    [[nodiscard]] static std::string indent() {
        return std::string(indent_level_ * 2, ' ');
    }

    /// Get current indentation level
    [[nodiscard]] static int level() {
        return indent_level_;
    }
};

/// Scoped context logger for operation boundaries
class ScopedLogger {
  private:
    std::string operation_;
    LogTimer timer_;

  public:
    explicit ScopedLogger(std::string operation,
                          spdlog::level::level_enum level = spdlog::level::info)
        : operation_(std::move(operation)), timer_(operation_, level) {
        LOG_INFO("{}▶ Starting {}", LogIndent::indent(), operation_);
    }

    ~ScopedLogger() {
        auto duration = timer_.elapsed();
        LOG_INFO("{}◀ Finished {} ({}ms)", LogIndent::indent(), operation_, duration.count());
    }
};

} // namespace finch

// Convenience macros for structured logging
#define LOG_TIMER(name) finch::LogTimer _log_timer(name)
#define LOG_SCOPED(name) finch::ScopedLogger _scoped_logger(name)
#define LOG_PROGRESS(name, total) finch::ProgressLogger _progress_logger(name, total)
#define LOG_INDENT() finch::LogIndent _log_indent
