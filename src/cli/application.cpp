#include <finch/cli/application.hpp>
#include <finch/cli/migration_pipeline.hpp>
#include <finch/cli/progress_reporter.hpp>
#include <finch/core/logging.hpp>
#include <finch/version.h>
#include <iostream>
#include <memory>

namespace finch::cli {

int Application::run(int argc, char** argv) {
    CLI::App app{"finch - CMake to Buck2 migration tool", "finch"};
    app.set_version_flag("-v,--version", finch::VERSION_STRING);

    // Global options
    app.add_option("--config", global_opts_.config_file, "Configuration file path")
        ->default_val(".finch.toml");
    app.add_flag("--verbose", global_opts_.verbose, "Enable verbose output");
    app.add_flag("--quiet", global_opts_.quiet, "Suppress non-error output");
    app.add_flag("--no-color", global_opts_.use_color, "Disable colored output")->default_val(true);
    app.add_option("--log-level", global_opts_.log_level,
                   "Set logging level (trace,debug,info,warn,error)");

    setup_commands(app);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    // Initialize error reporter
    error_reporter_ = std::make_unique<core::ErrorReporter>();

    return 0;
}

void Application::setup_commands(CLI::App& app) {
    // Migrate command
    auto* migrate = app.add_subcommand("migrate", "Migrate CMake project to Buck2");
    MigrateOptions migrate_opts;

    migrate
        ->add_option("source", migrate_opts.source_dir, "Source directory containing CMake files")
        ->default_val(".");
    migrate->add_option("-o,--output", migrate_opts.output_dir, "Output directory for Buck2 files")
        ->default_val(".");
    migrate->add_flag("-n,--dry-run", migrate_opts.dry_run,
                      "Preview changes without writing files");
    migrate->add_flag("-i,--interactive", migrate_opts.interactive,
                      "Interactive mode with prompts");
    migrate->add_option("--platform", migrate_opts.platforms, "Target platforms (can be repeated)")
        ->default_val(std::vector<std::string>{"linux", "macos", "windows"});
    migrate->add_flag("--overwrite", migrate_opts.overwrite, "Overwrite existing Buck2 files");
    migrate->add_option("--template-dir", migrate_opts.template_dir, "Custom template directory");

    migrate->callback([this, migrate_opts]() { handle_migrate(migrate_opts); });

    // Validate command
    auto* validate = app.add_subcommand("validate", "Validate CMake files");
    std::string validate_path = ".";
    validate->add_option("path", validate_path, "Path to validate")->default_val(".");
    validate->callback([this, &validate_path]() { handle_validate(validate_path); });

    // Analyze command
    auto* analyze = app.add_subcommand("analyze", "Analyze CMake project complexity");
    std::string analyze_path = ".";
    analyze->add_option("path", analyze_path, "Path to analyze")->default_val(".");
    analyze->callback([this, &analyze_path]() { handle_analyze(analyze_path); });

    // Init command
    auto* init = app.add_subcommand("init", "Initialize finch configuration");
    std::string init_path = ".";
    init->add_option("path", init_path, "Path to initialize")->default_val(".");
    init->callback([this, &init_path]() { handle_init(init_path); });

    app.require_subcommand(1);
}

int Application::handle_migrate(const MigrateOptions& opts) {
    // Initialize progress reporter
    if (global_opts_.quiet) {
        // No progress reporting in quiet mode
        progress_reporter_ = nullptr;
    } else if (global_opts_.log_level && *global_opts_.log_level == "json") {
        progress_reporter_ = std::make_unique<JsonProgressReporter>(std::cout);
    } else {
        progress_reporter_ =
            std::make_unique<ConsoleProgressReporter>(global_opts_.use_color, global_opts_.verbose);
    }

    // Create pipeline configuration
    MigrationPipeline::PipelineConfig config{.source_directory = opts.source_dir,
                                             .output_directory = opts.output_dir,
                                             .target_platforms = opts.platforms,
                                             .dry_run = opts.dry_run,
                                             .interactive = opts.interactive,
                                             .config_file = global_opts_.config_file};

    // Create and run pipeline
    MigrationPipeline pipeline(config);

    auto result = pipeline.execute();
    if (!result.has_value()) {
        error_reporter_->report(result.error());
        return 1;
    }

    if (progress_reporter_) {
        progress_reporter_->report_summary(result.value());
    }

    return 0;
}

int Application::handle_validate(const std::string& path) {
    std::cout << "Validating CMake files in: " << path << "\n";
    std::cout << "Validate command not yet fully implemented\n";
    return 0;
}

int Application::handle_analyze(const std::string& path) {
    std::cout << "Analyzing CMake project at: " << path << "\n";
    std::cout << "Analyze command not yet fully implemented\n";
    return 0;
}

int Application::handle_init(const std::string& path) {
    std::cout << "Initializing finch configuration at: " << path << "\n";
    std::cout << "Init command not yet fully implemented\n";
    return 0;
}

} // namespace finch::cli
