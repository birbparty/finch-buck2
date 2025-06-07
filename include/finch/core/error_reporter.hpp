#pragma once

#include "error.hpp"
#include <fmt/color.h>
#include <fmt/format.h>
#include <iostream>
#include <unistd.h> // for isatty

namespace finch {

/// Configuration for error reporting
struct ErrorReportConfig {
    /// Use colored output
    bool use_color = true;

    /// Output format
    enum class Format { Human, Structured } format = Format::Human;

    /// Show source code snippets if available
    bool show_source_snippets = true;

    /// Maximum number of context lines to show
    size_t max_context_lines = 3;

    /// Show help suggestions
    bool show_help = true;

    /// Compact output (fewer newlines)
    bool compact = false;

    /// Output stream to write to
    std::ostream* output_stream = &std::cerr;
};

/// Error reporter for formatting and displaying errors
class ErrorReporter {
  private:
    ErrorReportConfig config_;

  public:
    /// Constructor with configuration
    explicit ErrorReporter(ErrorReportConfig config = {}) : config_(std::move(config)) {}

    /// Report a single error
    void report(const Error& error) const {
        switch (config_.format) {
        case ErrorReportConfig::Format::Human:
            report_human(error);
            break;
        case ErrorReportConfig::Format::Structured:
            report_structured(error);
            break;
        }
    }

    /// Report multiple errors
    void report_all(const std::vector<Error>& errors) const {
        for (size_t i = 0; i < errors.size(); ++i) {
            if (i > 0 && !config_.compact) {
                *config_.output_stream << "\n";
            }
            report(errors[i]);
        }

        // Summary for multiple errors
        if (errors.size() > 1 && config_.format == ErrorReportConfig::Format::Human) {
            *config_.output_stream << "\n";
            if (config_.use_color) {
                *config_.output_stream
                    << fmt::format(fmt::emphasis::bold | fg(fmt::color::red), "error");
                *config_.output_stream << fmt::format(fmt::emphasis::bold, ": ");
            } else {
                *config_.output_stream << "error: ";
            }
            *config_.output_stream
                << fmt::format("found {} error{}\n", errors.size(), errors.size() == 1 ? "" : "s");
        }
    }

    /// Set configuration
    void set_config(ErrorReportConfig config) {
        config_ = std::move(config);
    }

    /// Get current configuration
    [[nodiscard]] const ErrorReportConfig& config() const noexcept {
        return config_;
    }

  private:
    /// Report error in human-readable format
    void report_human(const Error& error) const {
        auto& out = *config_.output_stream;

        // Location prefix
        if (error.location()) {
            if (config_.use_color) {
                out << fmt::format(fmt::emphasis::bold, "{}: ", error.location()->to_string());
            } else {
                out << fmt::format("{}: ", error.location()->to_string());
            }
        }

        // Error type and message
        if (config_.use_color) {
            out << fmt::format(fmt::emphasis::bold | fg(fmt::color::red), "error");
            out << fmt::format(fmt::emphasis::bold, ": ");
        } else {
            out << "error: ";
        }

        out << error.message() << "\n";

        // Context information
        if (!error.context().empty() && config_.max_context_lines > 0) {
            size_t shown = 0;
            for (const auto& ctx : error.context()) {
                if (shown >= config_.max_context_lines) {
                    out << fmt::format("  note: ... and {} more context line{}\n",
                                       error.context().size() - shown,
                                       (error.context().size() - shown) == 1 ? "" : "s");
                    break;
                }

                if (config_.use_color) {
                    out << fmt::format(fg(fmt::color::blue), "  note: ");
                } else {
                    out << "  note: ";
                }
                out << ctx << "\n";
                ++shown;
            }
        }

        // Help text
        if (error.help() && config_.show_help) {
            if (config_.use_color) {
                out << fmt::format(fg(fmt::color::green), "  help: ");
            } else {
                out << "  help: ";
            }
            out << *error.help() << "\n";
        }

        // Source snippet (placeholder for future implementation)
        if (config_.show_source_snippets && error.location()) {
            // TODO: Implement source snippet display
            // This would require access to the original source files
        }
    }

    /// Report error in structured format (for tools/IDEs)
    void report_structured(const Error& error) const {
        auto& out = *config_.output_stream;

        // Structured format: LEVEL:LOCATION:TYPE:MESSAGE
        out << "ERROR:";

        if (error.location()) {
            out << fmt::format("{}:{}:{}:", error.location()->file, error.location()->line,
                               error.location()->column);
        } else {
            out << ":::";
        }

        out << fmt::format("{}:{}\n", error.error_type(), error.message());

        // Context as additional structured lines
        for (const auto& ctx : error.context()) {
            out << fmt::format("NOTE:::{}\n", ctx);
        }

        if (error.help()) {
            out << fmt::format("HELP:::{}\n", *error.help());
        }
    }
};

/// Helper function to create a default error reporter
[[nodiscard]] inline ErrorReporter create_default_reporter() {
    ErrorReportConfig config;

    // Detect if stdout/stderr supports color
    // This is a simplified detection - a full implementation would check
    // environment variables like NO_COLOR, FORCE_COLOR, TERM, etc.
    config.use_color = isatty(fileno(stderr)) != 0;

    return ErrorReporter{config};
}

/// Helper function to create a structured error reporter (for IDEs/tools)
[[nodiscard]] inline ErrorReporter create_structured_reporter(std::ostream& out = std::cout) {
    ErrorReportConfig config;
    config.format = ErrorReportConfig::Format::Structured;
    config.use_color = false;
    config.compact = true;
    config.output_stream = &out;
    return ErrorReporter{config};
}

} // namespace finch
