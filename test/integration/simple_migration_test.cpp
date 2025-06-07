#include <finch/testing/migration_test_runner.hpp>
#include <finch/testing/test_project.hpp>
#include <gtest/gtest.h>

namespace finch::testing {

class SimpleMigrationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Configure test runner for syntax validation only (no Buck2 build)
        config_.validate_syntax_only = false;
        config_.run_buck_build = false;      // Don't actually run Buck2 for now
        config_.compare_with_golden = false; // No golden files yet
        config_.measure_performance = true;
        runner_ = std::make_unique<MigrationTestRunner>(config_);
    }

    MigrationTestRunner::TestConfig config_;
    std::unique_ptr<MigrationTestRunner> runner_;
};

TEST_F(SimpleMigrationTest, MigrateSimpleLibrary) {
    // Create test project
    TestProject project("test/projects/simple-library");

    // Setup the project (loads metadata and creates temp copy)
    auto setup_result = project.setup();
    ASSERT_TRUE(setup_result.has_value()) << "Failed to setup test project";

    // Run the migration test
    auto result = runner_->run_test(project);
    ASSERT_TRUE(result.has_value()) << "Failed to run test";

    const auto& test_result = result.value();

    // Check basic expectations
    EXPECT_EQ(test_result.project_name, "simple-library");
    EXPECT_TRUE(test_result.migration_succeeded) << "Migration should succeed";
    EXPECT_GT(test_result.targets_generated, 0) << "Should generate at least one target";

    // Check performance
    EXPECT_LT(test_result.duration.count(), 5000) << "Migration should complete in < 5 seconds";

    // Print any errors for debugging
    if (!test_result.errors.empty()) {
        for (const auto& error : test_result.errors) {
            std::cerr << "Error: " << error << std::endl;
        }
    }

    // Cleanup
    auto cleanup_result = project.cleanup();
    EXPECT_TRUE(cleanup_result.has_value()) << "Failed to cleanup test project";
}

TEST_F(SimpleMigrationTest, TestProjectMetadataLoading) {
    TestProject project("test/projects/simple-library");

    auto setup_result = project.setup();
    ASSERT_TRUE(setup_result.has_value());

    // Check metadata was loaded correctly
    const auto& metadata = project.metadata();
    EXPECT_EQ(metadata.name, "simple-library");
    EXPECT_EQ(metadata.description, "Basic static library with single source and header");
    EXPECT_EQ(metadata.difficulty_level, "simple");
    EXPECT_FALSE(metadata.features.empty());

    // Check expected results
    const auto& expected = project.expected_results();
    EXPECT_EQ(expected.expected_targets, 1);
    EXPECT_TRUE(expected.should_build_successfully);
    EXPECT_FALSE(expected.expected_buck_files.empty());

    project.cleanup();
}

TEST_F(SimpleMigrationTest, GenerateTestReport) {
    // Run multiple test projects
    std::vector<TestProject> projects;
    projects.emplace_back("test/projects/simple-library");

    // Setup all projects
    for (auto& project : projects) {
        auto setup_result = project.setup();
        ASSERT_TRUE(setup_result.has_value());
    }

    // Run test suite
    auto suite_result = runner_->run_test_suite(projects);
    ASSERT_TRUE(suite_result.has_value());

    // Generate report
    auto report_path = std::filesystem::temp_directory_path() / "finch_test_report.json";
    runner_->generate_report(suite_result.value(), report_path);

    // Verify report was created
    EXPECT_TRUE(std::filesystem::exists(report_path));

    // Cleanup
    for (auto& project : projects) {
        project.cleanup();
    }
    std::filesystem::remove(report_path);
}

} // namespace finch::testing
