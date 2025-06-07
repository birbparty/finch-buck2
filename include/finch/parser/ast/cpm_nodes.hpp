#pragma once

#include "node.hpp"
#include <fmt/format.h>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace finch::ast {

// CPM package source types
enum class CPMSourceType {
    GitHub, // gh:owner/repo or GITHUB_REPOSITORY
    GitURL, // GIT_REPOSITORY with URL
    URL,    // URL download
    Local   // Local path
};

// Version constraint
struct CPMVersion {
    std::string version;
    bool exact = false;  // @ vs >= syntax
    std::string git_tag; // GIT_TAG if specified
};

// CPMAddPackage node
class CPMAddPackage : public ASTNode {
  private:
    std::string name_;
    CPMSourceType source_type_;
    std::string source_; // URL or repo
    std::optional<CPMVersion> version_;
    std::map<std::string, std::string> options_;
    bool find_package_fallback_ = true;

  public:
    CPMAddPackage(SourceLocation loc, std::string name)
        : ASTNode(std::move(loc)), name_(std::move(name)) {}

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::CPMAddPackage;
    }

    // Setters for builder pattern
    CPMAddPackage& set_source(CPMSourceType type, std::string source) {
        source_type_ = type;
        source_ = std::move(source);
        return *this;
    }

    CPMAddPackage& set_version(CPMVersion version) {
        version_ = std::move(version);
        return *this;
    }

    CPMAddPackage& add_option(std::string key, std::string value) {
        options_[std::move(key)] = std::move(value);
        return *this;
    }

    CPMAddPackage& set_find_package_fallback(bool fallback) {
        find_package_fallback_ = fallback;
        return *this;
    }

    // Getters
    [[nodiscard]] const std::string& name() const {
        return name_;
    }
    [[nodiscard]] CPMSourceType source_type() const {
        return source_type_;
    }
    [[nodiscard]] const std::string& source() const {
        return source_;
    }
    [[nodiscard]] const std::optional<CPMVersion>& version() const {
        return version_;
    }
    [[nodiscard]] const std::map<std::string, std::string>& options() const {
        return options_;
    }
    [[nodiscard]] bool find_package_fallback() const {
        return find_package_fallback_;
    }

    [[nodiscard]] std::string to_string() const override {
        std::string result = fmt::format("CPMAddPackage(name={}", name_);
        if (!source_.empty()) {
            result += fmt::format(", source={}", source_);
        }
        if (version_.has_value()) {
            result += fmt::format(", version={}", version_->version);
        }
        if (!options_.empty()) {
            result += fmt::format(", options={}", options_.size());
        }
        result += ")";
        return result;
    }

    [[nodiscard]] std::string pretty_print(size_t indent) const override {
        std::string result = std::string(indent, ' ') + "CPMAddPackage(\n";
        std::string ind = std::string(indent + 2, ' ');

        result += ind + fmt::format("name: {}\n", name_);

        switch (source_type_) {
        case CPMSourceType::GitHub:
            result += ind + fmt::format("github: {}\n", source_);
            break;
        case CPMSourceType::GitURL:
            result += ind + fmt::format("git_url: {}\n", source_);
            break;
        case CPMSourceType::URL:
            result += ind + fmt::format("url: {}\n", source_);
            break;
        case CPMSourceType::Local:
            result += ind + fmt::format("local: {}\n", source_);
            break;
        }

        if (version_.has_value()) {
            result += ind + fmt::format("version: {}", version_->version);
            if (version_->exact) {
                result += " (exact)";
            }
            if (!version_->git_tag.empty()) {
                result += fmt::format(", tag: {}", version_->git_tag);
            }
            result += "\n";
        }

        if (!options_.empty()) {
            result += ind + "options:\n";
            for (const auto& [key, value] : options_) {
                result += std::string(indent + 4, ' ') + fmt::format("{}: {}\n", key, value);
            }
        }

        result += std::string(indent, ' ') + ")";
        return result;
    }

    // Clone method
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        auto cloned = std::make_unique<CPMAddPackage>(location_, name_);
        cloned->source_type_ = source_type_;
        cloned->source_ = source_;
        cloned->version_ = version_;
        cloned->options_ = options_;
        cloned->find_package_fallback_ = find_package_fallback_;
        return cloned;
    }
};

// CPMFindPackage node
class CPMFindPackage : public ASTNode {
  private:
    std::string name_;
    std::optional<std::string> version_;
    std::vector<std::string> components_;
    std::optional<std::string> github_repository_;
    std::optional<std::string> git_tag_;

  public:
    CPMFindPackage(SourceLocation loc, std::string name)
        : ASTNode(std::move(loc)), name_(std::move(name)) {}

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::CPMFindPackage;
    }

    // Setters
    CPMFindPackage& set_version(std::string version) {
        version_ = std::move(version);
        return *this;
    }

    CPMFindPackage& add_component(std::string component) {
        components_.push_back(std::move(component));
        return *this;
    }

    CPMFindPackage& set_github_repository(std::string repo) {
        github_repository_ = std::move(repo);
        return *this;
    }

    CPMFindPackage& set_git_tag(std::string tag) {
        git_tag_ = std::move(tag);
        return *this;
    }

    // Getters
    [[nodiscard]] const std::string& name() const {
        return name_;
    }
    [[nodiscard]] const std::optional<std::string>& version() const {
        return version_;
    }
    [[nodiscard]] const std::vector<std::string>& components() const {
        return components_;
    }
    [[nodiscard]] const std::optional<std::string>& github_repository() const {
        return github_repository_;
    }
    [[nodiscard]] const std::optional<std::string>& git_tag() const {
        return git_tag_;
    }

    [[nodiscard]] std::string to_string() const override {
        std::string result = fmt::format("CPMFindPackage(name={}", name_);
        if (version_.has_value()) {
            result += fmt::format(", version={}", *version_);
        }
        if (!components_.empty()) {
            result += fmt::format(", components={}", components_.size());
        }
        if (github_repository_.has_value()) {
            result += fmt::format(", github={}", *github_repository_);
        }
        result += ")";
        return result;
    }

    [[nodiscard]] std::string pretty_print(size_t indent) const override {
        std::string result = std::string(indent, ' ') + "CPMFindPackage(\n";
        std::string ind = std::string(indent + 2, ' ');

        result += ind + fmt::format("name: {}\n", name_);

        if (version_.has_value()) {
            result += ind + fmt::format("version: {}\n", *version_);
        }

        if (!components_.empty()) {
            result += ind + "components: [";
            for (size_t i = 0; i < components_.size(); ++i) {
                if (i > 0)
                    result += ", ";
                result += components_[i];
            }
            result += "]\n";
        }

        if (github_repository_.has_value()) {
            result += ind + fmt::format("github_repository: {}\n", *github_repository_);
        }

        if (git_tag_.has_value()) {
            result += ind + fmt::format("git_tag: {}\n", *git_tag_);
        }

        result += std::string(indent, ' ') + ")";
        return result;
    }

    // Clone method
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        auto cloned = std::make_unique<CPMFindPackage>(location_, name_);
        cloned->version_ = version_;
        cloned->components_ = components_;
        cloned->github_repository_ = github_repository_;
        cloned->git_tag_ = git_tag_;
        return cloned;
    }
};

// CPMUsePackageLock node
class CPMUsePackageLock : public ASTNode {
  private:
    std::string lock_file_path_;

  public:
    CPMUsePackageLock(SourceLocation loc, std::string path)
        : ASTNode(std::move(loc)), lock_file_path_(std::move(path)) {}

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::CPMUsePackageLock;
    }

    [[nodiscard]] const std::string& lock_file_path() const {
        return lock_file_path_;
    }

    [[nodiscard]] std::string to_string() const override {
        return fmt::format("CPMUsePackageLock({})", lock_file_path_);
    }

    [[nodiscard]] std::string pretty_print(size_t indent) const override {
        return std::string(indent, ' ') + to_string();
    }

    // Clone method
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        return std::make_unique<CPMUsePackageLock>(location_, lock_file_path_);
    }
};

// CPMDeclarePackage node
class CPMDeclarePackage : public ASTNode {
  private:
    std::string name_;
    std::string version_;
    std::optional<std::string> github_repository_;
    std::optional<std::string> git_repository_;

  public:
    CPMDeclarePackage(SourceLocation loc, std::string name)
        : ASTNode(std::move(loc)), name_(std::move(name)) {}

    void accept(ASTVisitor& visitor) const override;

    [[nodiscard]] NodeType type() const override {
        return NodeType::CPMDeclarePackage;
    }

    // Setters
    CPMDeclarePackage& set_version(std::string version) {
        version_ = std::move(version);
        return *this;
    }

    CPMDeclarePackage& set_github_repository(std::string repo) {
        github_repository_ = std::move(repo);
        return *this;
    }

    CPMDeclarePackage& set_git_repository(std::string repo) {
        git_repository_ = std::move(repo);
        return *this;
    }

    // Getters
    [[nodiscard]] const std::string& name() const {
        return name_;
    }
    [[nodiscard]] const std::string& version() const {
        return version_;
    }
    [[nodiscard]] const std::optional<std::string>& github_repository() const {
        return github_repository_;
    }
    [[nodiscard]] const std::optional<std::string>& git_repository() const {
        return git_repository_;
    }

    [[nodiscard]] std::string to_string() const override {
        return fmt::format("CPMDeclarePackage(name={}, version={})", name_, version_);
    }

    [[nodiscard]] std::string pretty_print(size_t indent) const override {
        std::string result = std::string(indent, ' ') + "CPMDeclarePackage(\n";
        std::string ind = std::string(indent + 2, ' ');

        result += ind + fmt::format("name: {}\n", name_);
        result += ind + fmt::format("version: {}\n", version_);

        if (github_repository_.has_value()) {
            result += ind + fmt::format("github_repository: {}\n", *github_repository_);
        }

        if (git_repository_.has_value()) {
            result += ind + fmt::format("git_repository: {}\n", *git_repository_);
        }

        result += std::string(indent, ' ') + ")";
        return result;
    }

    // Clone method
    [[nodiscard]] std::unique_ptr<ASTNode> clone() const override {
        auto cloned = std::make_unique<CPMDeclarePackage>(location_, name_);
        cloned->version_ = version_;
        cloned->github_repository_ = github_repository_;
        cloned->git_repository_ = git_repository_;
        return cloned;
    }
};

} // namespace finch::ast
