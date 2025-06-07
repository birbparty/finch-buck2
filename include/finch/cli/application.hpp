#pragma once

#include <CLI/CLI.hpp>
#include <finch/core/error.hpp>
#include <finch/core/error_reporter.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace finch::cli {

class ProgressReporter;
class MigrationPipeline;

class Application {
  public:
    struct GlobalOptions {
        std::string config_file = ".finch.toml";
        bool verbose = false;
        bool quiet = false;
        bool use_color = true;
        std::optional<std::string> log_level;
    };

    struct MigrateOptions {
        std::string source_dir = ".";
        std::string output_dir = ".";
        bool dry_run = false;
        bool interactive = false;
        std::vector<std::string> platforms = {"linux", "macos", "windows"};
        bool overwrite = false;
        std::optional<std::string> template_dir;
    };

    int run(int argc, char** argv);

  private:
    void setup_commands(CLI::App& app);
    int handle_migrate(const MigrateOptions& opts);
    int handle_validate(const std::string& path);
    int handle_analyze(const std::string& path);
    int handle_init(const std::string& path);

    GlobalOptions global_opts_;
    std::unique_ptr<core::ErrorReporter> error_reporter_;
    std::unique_ptr<ProgressReporter> progress_reporter_;
};

} // namespace finch::cli
