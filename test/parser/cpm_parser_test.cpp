#include <finch/parser/ast/cpm_nodes.hpp>
#include <finch/parser/ast/visitor.hpp>
#include <finch/parser/parser.hpp>
#include <gtest/gtest.h>

using namespace finch;
using namespace finch::parser;
using namespace finch::ast;

class CPMParserTest : public ::testing::Test {
  protected:
    std::unique_ptr<File> parse_string(const std::string& source) {
        Parser parser(source, "test.cmake");
        auto result = parser.parse_file();
        if (result.has_value()) {
            return std::move(result.value());
        }

        // Print errors for debugging
        for (const auto& error : result.error()) {
            std::cerr << "Parse error: " << error.message() << " at " << error.location().line
                      << ":" << error.location().column << std::endl;
        }
        return nullptr;
    }
};

TEST_F(CPMParserTest, ParseCPMAddPackage) {
    const char* source = R"(
CPMAddPackage(
    NAME fmt
    VERSION 10.0.0
    GITHUB_REPOSITORY fmtlib/fmt
    OPTIONS "FMT_INSTALL ON"
)
)";

    auto file = parse_string(source);
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(file->statements().size(), 1);

    auto* cpm_add = dynamic_cast<const CPMAddPackage*>(file->statements()[0].get());
    ASSERT_NE(cpm_add, nullptr);

    EXPECT_EQ(cpm_add->name(), "fmt");
    EXPECT_EQ(cpm_add->version().to_string(), "10.0.0");
    EXPECT_TRUE(cpm_add->github_repository().has_value());
    EXPECT_EQ(cpm_add->github_repository().value(), "fmtlib/fmt");
    EXPECT_EQ(cpm_add->options().size(), 1);
    EXPECT_EQ(cpm_add->options()[0].first, "FMT_INSTALL");
    EXPECT_EQ(cpm_add->options()[0].second, "ON");
}

TEST_F(CPMParserTest, ParseCPMAddPackageWithGitTag) {
    const char* source = R"(
CPMAddPackage(
    NAME spdlog
    GIT_TAG v1.11.0
    GITHUB_REPOSITORY gabime/spdlog
)
)";

    auto file = parse_string(source);
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(file->statements().size(), 1);

    auto* cpm_add = dynamic_cast<const CPMAddPackage*>(file->statements()[0].get());
    ASSERT_NE(cpm_add, nullptr);

    EXPECT_EQ(cpm_add->name(), "spdlog");
    EXPECT_TRUE(cpm_add->git_tag().has_value());
    EXPECT_EQ(cpm_add->git_tag().value(), "v1.11.0");
}

TEST_F(CPMParserTest, ParseCPMFindPackage) {
    const char* source = R"(
CPMFindPackage(
    NAME nlohmann_json
    VERSION 3.11
    REQUIRED
    COMPONENTS JSON
)
)";

    auto file = parse_string(source);
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(file->statements().size(), 1);

    auto* cpm_find = dynamic_cast<const CPMFindPackage*>(file->statements()[0].get());
    ASSERT_NE(cpm_find, nullptr);

    EXPECT_EQ(cpm_find->name(), "nlohmann_json");
    EXPECT_EQ(cpm_find->version(), "3.11");
    EXPECT_TRUE(cpm_find->required());
    EXPECT_EQ(cpm_find->components().size(), 1);
    EXPECT_EQ(cpm_find->components()[0], "JSON");
}

TEST_F(CPMParserTest, ParseCPMUsePackageLock) {
    const char* source = R"(
CPMUsePackageLock(package-lock.cmake)
)";

    auto file = parse_string(source);
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(file->statements().size(), 1);

    auto* cpm_lock = dynamic_cast<const CPMUsePackageLock*>(file->statements()[0].get());
    ASSERT_NE(cpm_lock, nullptr);

    EXPECT_EQ(cpm_lock->lockfile_path(), "package-lock.cmake");
}

TEST_F(CPMParserTest, ParseCPMDeclarePackage) {
    const char* source = R"(
CPMDeclarePackage(
    mylib
    VERSION 1.2.3
    DEPENDENCIES fmt spdlog
)
)";

    auto file = parse_string(source);
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(file->statements().size(), 1);

    auto* cpm_declare = dynamic_cast<const CPMDeclarePackage*>(file->statements()[0].get());
    ASSERT_NE(cpm_declare, nullptr);

    EXPECT_EQ(cpm_declare->name(), "mylib");
    EXPECT_EQ(cpm_declare->version().to_string(), "1.2.3");
    EXPECT_EQ(cpm_declare->dependencies().size(), 2);
    EXPECT_EQ(cpm_declare->dependencies()[0], "fmt");
    EXPECT_EQ(cpm_declare->dependencies()[1], "spdlog");
}

TEST_F(CPMParserTest, ParseMultipleCPMCommands) {
    const char* source = R"(
CPMAddPackage(NAME fmt VERSION 10.0.0)
CPMAddPackage(NAME spdlog VERSION 1.11.0)
CPMFindPackage(NAME Boost VERSION 1.80 REQUIRED)
)";

    auto file = parse_string(source);
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(file->statements().size(), 3);

    EXPECT_NE(dynamic_cast<const CPMAddPackage*>(file->statements()[0].get()), nullptr);
    EXPECT_NE(dynamic_cast<const CPMAddPackage*>(file->statements()[1].get()), nullptr);
    EXPECT_NE(dynamic_cast<const CPMFindPackage*>(file->statements()[2].get()), nullptr);
}

// Test visitor pattern works with CPM nodes
class CPMVisitorTest : public ASTVisitorBase {
  public:
    int cpm_add_count = 0;
    int cpm_find_count = 0;
    int cpm_lock_count = 0;
    int cpm_declare_count = 0;

    void visit(const CPMAddPackage&) override {
        cpm_add_count++;
    }
    void visit(const CPMFindPackage&) override {
        cpm_find_count++;
    }
    void visit(const CPMUsePackageLock&) override {
        cpm_lock_count++;
    }
    void visit(const CPMDeclarePackage&) override {
        cpm_declare_count++;
    }
};

TEST_F(CPMParserTest, VisitorPattern) {
    const char* source = R"(
CPMAddPackage(NAME fmt VERSION 10.0.0)
CPMFindPackage(NAME Boost REQUIRED)
CPMUsePackageLock(lock.cmake)
CPMDeclarePackage(mylib VERSION 1.0.0)
)";

    auto file = parse_string(source);
    ASSERT_NE(file, nullptr);

    CPMVisitorTest visitor;
    for (const auto& stmt : file->statements()) {
        stmt->accept(visitor);
    }

    EXPECT_EQ(visitor.cpm_add_count, 1);
    EXPECT_EQ(visitor.cpm_find_count, 1);
    EXPECT_EQ(visitor.cpm_lock_count, 1);
    EXPECT_EQ(visitor.cpm_declare_count, 1);
}

// Test error handling
TEST_F(CPMParserTest, ErrorMissingName) {
    const char* source = R"(
CPMAddPackage(VERSION 10.0.0)
)";

    auto file = parse_string(source);
    // Should still parse but might have error node or regular command
    ASSERT_NE(file, nullptr);
}

TEST_F(CPMParserTest, ErrorInvalidVersion) {
    const char* source = R"(
CPMAddPackage(NAME fmt VERSION invalid-version)
)";

    auto file = parse_string(source);
    ASSERT_NE(file, nullptr);
    // Version parsing should handle invalid versions gracefully
}
