#pragma once

#include <chrono>
#include <finch/core/error.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace finch::cli {

// Forward declarations and types
struct MigrationResult {
    size_t files_processed;
    size_t targets_generated;
    size_t errors_encountered;
    std::vector<std::string> warnings;
    std::chrono::milliseconds duration;
};

enum class Phase { Discovery, Parsing, Analysis, Generation, Writing, Validation };

class ProgressReporter {
  public:
    virtual ~ProgressReporter() = default;

    virtual void start_phase(Phase phase, const std::string& description) = 0;
    virtual void update_progress(size_t current, size_t total) = 0;
    virtual void report_file(const std::string& filename) = 0;
    virtual void report_warning(const std::string& message) = 0;
    virtual void report_error(const finch::Error& error) = 0;
    virtual void finish_phase(bool success) = 0;
    virtual void report_summary(const MigrationResult& result) = 0;
};

class ConsoleProgressReporter : public ProgressReporter {
  public:
    ConsoleProgressReporter(bool use_color = true, bool show_files = true);

    void start_phase(Phase phase, const std::string& description) override;
    void update_progress(size_t current, size_t total) override;
    void report_file(const std::string& filename) override;
    void report_warning(const std::string& message) override;
    void report_error(const finch::Error& error) override;
    void finish_phase(bool success) override;
    void report_summary(const MigrationResult& result) override;

  private:
    bool use_color_;
    bool show_files_;
    std::chrono::steady_clock::time_point phase_start_;

    void print_progress_bar(size_t current, size_t total);
    std::string phase_to_string(Phase phase) const;
    std::string format_duration(std::chrono::milliseconds duration) const;
};

class JsonProgressReporter : public ProgressReporter {
  public:
    explicit JsonProgressReporter(std::ostream& output);

    void start_phase(Phase phase, const std::string& description) override;
    void update_progress(size_t current, size_t total) override;
    void report_file(const std::string& filename) override;
    void report_warning(const std::string& message) override;
    void report_error(const finch::Error& error) override;
    void finish_phase(bool success) override;
    void report_summary(const MigrationResult& result) override;

  private:
    std::ostream& output_;
    void emit_event(const std::string& event_type, const std::string& data);
};

} // namespace finch::cli
