#pragma once

#include <chrono>
#include <filesystem>
#include <finch/core/error.hpp>
#include <finch/core/result.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace finch::parser {
class Parser;
}

namespace finch::analyzer {
class CMakeFileEvaluator;
struct ProjectAnalysis;
} // namespace finch::analyzer

namespace finch::generator {
class Generator;
}

namespace finch::cli {

class ProgressReporter;

enum class MigrationErrorKind {
    FileSystemError,
    ParsingError,
    AnalysisError,
    GenerationError,
    ValidationError,
    ConfigurationError
};

class MigrationError : public Error {
  public:
    explicit MigrationError(MigrationErrorKind kind, const std::string& message)
        : Error(std::string(error_kind_to_string(kind)) + ": " + message), kind_(kind) {}

    MigrationErrorKind kind() const {
        return kind_;
    }

  private:
    MigrationErrorKind kind_;

    static constexpr const char* error_kind_to_string(MigrationErrorKind kind) {
        switch (kind) {
        case MigrationErrorKind::FileSystemError:
            return "FileSystemError";
        case MigrationErrorKind::ParsingError:
            return "ParsingError";
        case MigrationErrorKind::AnalysisError:
            return "AnalysisError";
        case MigrationErrorKind::GenerationError:
            return "GenerationError";
        case MigrationErrorKind::ValidationError:
            return "ValidationError";
        case MigrationErrorKind::ConfigurationError:
            return "ConfigurationError";
        }
        return "UnknownError";
    }
};

class MigrationPipeline {
  public:
    struct PipelineConfig {
        std::string source_directory;
        std::string output_directory;
        std::vector<std::string> target_platforms;
        bool dry_run;
        bool interactive;
        std::optional<std::string> config_file;
    };

    struct MigrationResult {
        size_t files_processed;
        size_t targets_generated;
        size_t errors_encountered;
        std::vector<std::string> warnings;
        std::chrono::milliseconds duration;
    };

    explicit MigrationPipeline(const PipelineConfig& config);

    Result<MigrationResult, MigrationError> execute();

  private:
    Result<std::vector<std::filesystem::path>, MigrationError> discover_cmake_files();

    Result<analyzer::ProjectAnalysis, MigrationError>
    process_file(const std::filesystem::path& cmake_file);

    Result<void, MigrationError> generate_buck_files(const analyzer::ProjectAnalysis& analysis);

    void merge_analysis(analyzer::ProjectAnalysis& target, const analyzer::ProjectAnalysis& source);

    PipelineConfig config_;
    std::unique_ptr<ProgressReporter> progress_;
    std::unique_ptr<parser::Parser> parser_;
    std::unique_ptr<analyzer::CMakeFileEvaluator> analyzer_;
    std::unique_ptr<generator::Generator> generator_;
};

} // namespace finch::cli
