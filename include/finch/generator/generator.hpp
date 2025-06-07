#pragma once

#include <filesystem>
#include <finch/core/error.hpp>
#include <finch/core/result.hpp>
#include <memory>
#include <string>
#include <vector>

namespace finch::analyzer {
struct ProjectAnalysis;
struct Target;
} // namespace finch::analyzer

namespace finch::generator {

class TargetMapper;
class StarlarkWriter;
class TemplateRegistry;

class Generator {
  public:
    struct Config {
        std::filesystem::path output_directory;
        std::vector<std::string> target_platforms;
        bool dry_run = false;
        bool preserve_comments = true;
        std::optional<std::filesystem::path> template_directory;
    };

    struct GenerationResult {
        std::vector<std::filesystem::path> generated_files;
        size_t targets_processed;
        std::vector<std::string> warnings;
    };

    explicit Generator(const Config& config);
    ~Generator();

    Result<GenerationResult, GenerationError> generate(const analyzer::ProjectAnalysis& analysis);

  private:
    std::unique_ptr<TargetMapper> target_mapper_;
    std::unique_ptr<TemplateRegistry> template_registry_;
    Config config_;

    Result<void, GenerationError> generate_buck_file(const std::filesystem::path& output_path,
                                                     const std::vector<analyzer::Target>& targets);

    Result<void, GenerationError> generate_buckconfig(const analyzer::ProjectAnalysis& analysis);

    Result<void, GenerationError> write_file(const std::filesystem::path& path,
                                             const std::string& content);
};

} // namespace finch::generator
