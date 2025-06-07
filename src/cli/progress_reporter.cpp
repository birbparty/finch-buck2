#include <finch/cli/migration_pipeline.hpp>
#include <finch/cli/progress_reporter.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace finch::cli {

// ConsoleProgressReporter implementation
ConsoleProgressReporter::ConsoleProgressReporter(bool use_color, bool show_files)
    : use_color_(use_color), show_files_(show_files) {}

void ConsoleProgressReporter::start_phase(Phase phase, const std::string& description) {
    phase_start_ = std::chrono::steady_clock::now();

    if (use_color_) {
        fmt::print(fmt::fg(fmt::color::cyan), "🔄 {}\n", description);
    } else {
        std::cout << "* " << description << "\n";
    }
}

void ConsoleProgressReporter::update_progress(size_t current, size_t total) {
    print_progress_bar(current, total);
}

void ConsoleProgressReporter::report_file(const std::string& filename) {
    if (show_files_) {
        if (use_color_) {
            fmt::print(fmt::fg(fmt::color::dim_gray), "   Processing: {}\n", filename);
        } else {
            std::cout << "   Processing: " << filename << "\n";
        }
    }
}

void ConsoleProgressReporter::report_warning(const std::string& message) {
    if (use_color_) {
        fmt::print(fmt::fg(fmt::color::yellow), "⚠️  Warning: {}\n", message);
    } else {
        std::cout << "Warning: " << message << "\n";
    }
}

void ConsoleProgressReporter::report_error(const finch::Error& error) {
    if (use_color_) {
        fmt::print(fmt::fg(fmt::color::red), "❌ Error: {}\n", error.message());
    } else {
        std::cout << "Error: " << error.message() << "\n";
    }
}

void ConsoleProgressReporter::finish_phase(bool success) {
    auto duration = std::chrono::steady_clock::now() - phase_start_;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    if (success) {
        if (use_color_) {
            fmt::print(fmt::fg(fmt::color::green), "✅ Done ({}ms)\n\n", ms);
        } else {
            std::cout << "Done (" << ms << "ms)\n\n";
        }
    } else {
        if (use_color_) {
            fmt::print(fmt::fg(fmt::color::red), "❌ Failed ({}ms)\n\n", ms);
        } else {
            std::cout << "Failed (" << ms << "ms)\n\n";
        }
    }
}

void ConsoleProgressReporter::report_summary(const MigrationResult& result) {
    if (use_color_) {
        fmt::print(fmt::fg(fmt::color::green), "\n✅ Migration complete!\n");
        fmt::print("   📊 Summary:\n");
        fmt::print("   • {} CMake files processed\n", result.files_processed);
        fmt::print("   • {} Buck2 targets generated\n", result.targets_generated);

        if (result.errors_encountered > 0) {
            fmt::print(fmt::fg(fmt::color::red), "   • {} errors\n", result.errors_encountered);
        } else {
            fmt::print("   • 0 errors\n");
        }

        if (!result.warnings.empty()) {
            fmt::print(fmt::fg(fmt::color::yellow), "   • {} warnings\n", result.warnings.size());
        } else {
            fmt::print("   • 0 warnings\n");
        }

        fmt::print("   • Duration: {}\n", format_duration(result.duration));
    } else {
        std::cout << "\nMigration complete!\n";
        std::cout << "Summary:\n";
        std::cout << "  * " << result.files_processed << " CMake files processed\n";
        std::cout << "  * " << result.targets_generated << " Buck2 targets generated\n";
        std::cout << "  * " << result.errors_encountered << " errors\n";
        std::cout << "  * " << result.warnings.size() << " warnings\n";
        std::cout << "  * Duration: " << format_duration(result.duration) << "\n";
    }
}

void ConsoleProgressReporter::print_progress_bar(size_t current, size_t total) {
    if (total == 0)
        return;

    const int bar_width = 50;
    float progress = static_cast<float>(current) / static_cast<float>(total);
    int filled = static_cast<int>(progress * bar_width);

    std::cout << "\r   [";
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) {
            std::cout << "█";
        } else {
            std::cout << "░";
        }
    }
    std::cout << "] " << current << "/" << total << " (" << static_cast<int>(progress * 100) << "%)"
              << std::flush;

    if (current == total) {
        std::cout << "\n";
    }
}

std::string ConsoleProgressReporter::phase_to_string(Phase phase) const {
    switch (phase) {
    case Phase::Discovery:
        return "Discovery";
    case Phase::Parsing:
        return "Parsing";
    case Phase::Analysis:
        return "Analysis";
    case Phase::Generation:
        return "Generation";
    case Phase::Writing:
        return "Writing";
    case Phase::Validation:
        return "Validation";
    }
    return "Unknown";
}

std::string ConsoleProgressReporter::format_duration(std::chrono::milliseconds duration) const {
    auto ms = duration.count();
    if (ms < 1000) {
        return fmt::format("{}ms", ms);
    } else if (ms < 60000) {
        return fmt::format("{:.1f}s", ms / 1000.0);
    } else {
        auto minutes = ms / 60000;
        auto seconds = (ms % 60000) / 1000;
        return fmt::format("{}m {}s", minutes, seconds);
    }
}

// JsonProgressReporter implementation
JsonProgressReporter::JsonProgressReporter(std::ostream& output) : output_(output) {}

void JsonProgressReporter::start_phase(Phase phase, const std::string& description) {
    emit_event("phase_start", fmt::format(R"({{"phase":"{}","description":"{}"}})",
                                          static_cast<int>(phase), description));
}

void JsonProgressReporter::update_progress(size_t current, size_t total) {
    emit_event("progress", fmt::format(R"({{"current":{},"total":{}}})", current, total));
}

void JsonProgressReporter::report_file(const std::string& filename) {
    emit_event("file", fmt::format(R"({{"filename":"{}"}})", filename));
}

void JsonProgressReporter::report_warning(const std::string& message) {
    emit_event("warning", fmt::format(R"({{"message":"{}"}})", message));
}

void JsonProgressReporter::report_error(const finch::Error& error) {
    emit_event("error", fmt::format(R"({{"message":"{}"}})", error.message()));
}

void JsonProgressReporter::finish_phase(bool success) {
    emit_event("phase_end", fmt::format(R"({{"success":{}}})", success));
}

void JsonProgressReporter::report_summary(const MigrationResult& result) {
    std::string warnings_json = "[";
    for (size_t i = 0; i < result.warnings.size(); ++i) {
        if (i > 0)
            warnings_json += ",";
        warnings_json += fmt::format(R"("{}")", result.warnings[i]);
    }
    warnings_json += "]";

    emit_event(
        "summary",
        fmt::format(
            R"({{"files_processed":{},"targets_generated":{},"errors_encountered":{},"warnings":{},"duration_ms":{}}})",
            result.files_processed, result.targets_generated, result.errors_encountered,
            warnings_json, result.duration.count()));
}

void JsonProgressReporter::emit_event(const std::string& event_type, const std::string& data) {
    output_ << fmt::format(R"({{"type":"{}","data":{}}})", event_type, data) << "\n";
    output_.flush();
}

} // namespace finch::cli
