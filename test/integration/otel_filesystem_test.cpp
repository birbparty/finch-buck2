#include <cstdlib>
#include <filesystem>
#include <finch/core/logging.hpp>
#include <finch/core/otel_integration.hpp>
#include <fstream>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

class OtelFilesystemTest : public ::testing::Test {
  protected:
    std::string otel_dir_;
    std::string config_file_;

    void SetUp() override {
        // Get home directory and construct path to local-otel
        const char* home = std::getenv("HOME");
        ASSERT_NE(home, nullptr) << "HOME environment variable not set";

        otel_dir_ = fs::path(home) / "git" / "local-otel";
        config_file_ = fs::path(otel_dir_) / "configs" / "wasm-collector.yaml";

        // Initialize logging for tests
        finch::LogConfig log_config;
        log_config.console_level = spdlog::level::info;
        finch::Logger::initialize(log_config);
    }

    void TearDown() override {
        finch::OtelIntegration::shutdown();
        finch::Logger::shutdown();
    }

    bool DirectoryExists() const {
        return fs::exists(otel_dir_) && fs::is_directory(otel_dir_);
    }

    bool ConfigFileExists() const {
        return fs::exists(config_file_) && fs::is_regular_file(config_file_);
    }
};

TEST_F(OtelFilesystemTest, DirectoryStructureExists) {
    if (!DirectoryExists()) {
        GTEST_SKIP() << "local-otel directory not found at: " << otel_dir_;
    }

    EXPECT_TRUE(fs::exists(otel_dir_));
    EXPECT_TRUE(fs::is_directory(otel_dir_));

    // Check for expected subdirectories
    EXPECT_TRUE(fs::exists(fs::path(otel_dir_) / "configs"));

    // Log the directory structure for debugging
    LOG_INFO("Found OTEL directory at: {}", otel_dir_);

    for (const auto& entry : fs::directory_iterator(otel_dir_)) {
        if (entry.is_directory()) {
            LOG_INFO("  Directory: {}", entry.path().filename().string());
        } else if (entry.is_regular_file()) {
            LOG_INFO("  File: {}", entry.path().filename().string());
        }
    }
}

TEST_F(OtelFilesystemTest, ConfigFileAccess) {
    if (!DirectoryExists()) {
        GTEST_SKIP() << "local-otel directory not found at: " << otel_dir_;
    }

    if (!ConfigFileExists()) {
        GTEST_SKIP() << "OTEL config file not found at: " << config_file_;
    }

    // Test file accessibility
    EXPECT_TRUE(fs::exists(config_file_));
    EXPECT_TRUE(fs::is_regular_file(config_file_));

    // Test file permissions
    auto perms = fs::status(config_file_).permissions();
    EXPECT_TRUE((perms & fs::perms::owner_read) != fs::perms::none);

    // Test file size
    auto file_size = fs::file_size(config_file_);
    EXPECT_GT(file_size, 0) << "Config file is empty";

    LOG_INFO("Config file size: {} bytes", file_size);
}

TEST_F(OtelFilesystemTest, FileContentReading) {
    if (!ConfigFileExists()) {
        GTEST_SKIP() << "OTEL config file not found at: " << config_file_;
    }

    std::ifstream file(config_file_);
    ASSERT_TRUE(file.is_open()) << "Failed to open config file";

    std::string content;
    std::string line;
    int line_count = 0;

    while (std::getline(file, line)) {
        content += line + "\n";
        line_count++;
    }

    EXPECT_GT(line_count, 0) << "Config file appears to be empty";
    EXPECT_FALSE(content.empty()) << "No content read from file";

    // Check for expected YAML keywords
    EXPECT_NE(content.find("receivers:"), std::string::npos);
    EXPECT_NE(content.find("processors:"), std::string::npos);
    EXPECT_NE(content.find("exporters:"), std::string::npos);
    EXPECT_NE(content.find("service:"), std::string::npos);
    EXPECT_NE(content.find("otlp:"), std::string::npos);

    LOG_INFO("Successfully read {} lines from config file", line_count);
}

TEST_F(OtelFilesystemTest, DirectoryListing) {
    if (!DirectoryExists()) {
        GTEST_SKIP() << "local-otel directory not found at: " << otel_dir_;
    }

    std::vector<std::string> expected_files = {"README.md", "docker-compose.yml",
                                               "INTEGRATION_TEST_GUIDE.md"};

    std::vector<std::string> expected_dirs = {"configs"};

    int file_count = 0;
    int dir_count = 0;

    // List directory contents
    for (const auto& entry : fs::directory_iterator(otel_dir_)) {
        if (entry.is_regular_file()) {
            file_count++;
            std::string filename = entry.path().filename().string();

            // Check if it's one of our expected files
            auto it = std::find(expected_files.begin(), expected_files.end(), filename);
            if (it != expected_files.end()) {
                expected_files.erase(it);
                LOG_INFO("Found expected file: {}", filename);
            }
        } else if (entry.is_directory()) {
            dir_count++;
            std::string dirname = entry.path().filename().string();

            // Check if it's one of our expected directories
            auto it = std::find(expected_dirs.begin(), expected_dirs.end(), dirname);
            if (it != expected_dirs.end()) {
                expected_dirs.erase(it);
                LOG_INFO("Found expected directory: {}", dirname);
            }
        }
    }

    // All expected directories should be found
    EXPECT_TRUE(expected_dirs.empty()) << "Missing expected directories";

    LOG_INFO("Directory listing complete: {} files, {} directories", file_count, dir_count);
}

TEST_F(OtelFilesystemTest, ConfigsDirectoryContents) {
    auto configs_dir = fs::path(otel_dir_) / "configs";

    if (!fs::exists(configs_dir)) {
        GTEST_SKIP() << "configs directory not found at: " << configs_dir;
    }

    EXPECT_TRUE(fs::is_directory(configs_dir));

    // Count YAML files
    int yaml_count = 0;
    for (const auto& entry : fs::directory_iterator(configs_dir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".yaml" || ext == ".yml") {
                yaml_count++;
                LOG_INFO("Found YAML config: {}", entry.path().filename().string());
            }
        }
    }

    EXPECT_GT(yaml_count, 0) << "No YAML configuration files found";
}

TEST_F(OtelFilesystemTest, OtelConfigurationValidation) {
    if (!ConfigFileExists()) {
        GTEST_SKIP() << "OTEL config file not found at: " << config_file_;
    }

    // Read the file and validate basic structure
    std::ifstream file(config_file_);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Basic validation - should contain OTEL collector structure
    EXPECT_NE(content.find("receivers:"), std::string::npos);
    EXPECT_NE(content.find("otlp:"), std::string::npos);
    EXPECT_NE(content.find("protocols:"), std::string::npos);
    EXPECT_NE(content.find("grpc:"), std::string::npos);
    EXPECT_NE(content.find("http:"), std::string::npos);
    EXPECT_NE(content.find("endpoint:"), std::string::npos);

    // Check for standard ports
    EXPECT_NE(content.find("4317"), std::string::npos) << "GRPC port not found";
    EXPECT_NE(content.find("4318"), std::string::npos) << "HTTP port not found";

    LOG_INFO("OTEL configuration validation passed");
}

TEST_F(OtelFilesystemTest, DataDirectoriesCheck) {
    if (!DirectoryExists()) {
        GTEST_SKIP() << "local-otel directory not found at: " << otel_dir_;
    }

    // Check for data output directories (optional)
    std::vector<std::string> data_dirs = {"data", "logs", "metrics", "traces"};

    for (const auto& dir_name : data_dirs) {
        auto dir_path = fs::path(otel_dir_) / dir_name;
        if (fs::exists(dir_path)) {
            EXPECT_TRUE(fs::is_directory(dir_path));
            LOG_INFO("Found data directory: {}", dir_name);

            // Count files in data directory
            int file_count = 0;
            try {
                for (const auto& entry : fs::directory_iterator(dir_path)) {
                    if (entry.is_regular_file()) {
                        file_count++;
                    }
                }
                LOG_INFO("  Contains {} files", file_count);
            } catch (const fs::filesystem_error& e) {
                LOG_WARN("Cannot read directory {}: {}", dir_name, e.what());
            }
        } else {
            LOG_INFO("Data directory {} does not exist (optional)", dir_name);
        }
    }
}
