#pragma once

#include <fmt/format.h>
#include <optional>
#include <string>
#include <vector>

namespace finch {

/// Source location information for error reporting
struct SourceLocation {
    std::string file;
    size_t line;
    size_t column;
    size_t offset;

    /// Default constructor
    SourceLocation() = default;

    /// Constructor
    SourceLocation(std::string file, size_t line, size_t column, size_t offset = 0)
        : file(std::move(file)), line(line), column(column), offset(offset) {}

    /// Convert to string representation
    [[nodiscard]] std::string to_string() const {
        return fmt::format("{}:{}:{}", file, line, column);
    }

    /// Check if location is valid
    [[nodiscard]] bool is_valid() const noexcept {
        return !file.empty() && line > 0 && column > 0;
    }

    /// Equality comparison
    [[nodiscard]] bool operator==(const SourceLocation& other) const noexcept {
        return file == other.file && line == other.line && column == other.column &&
               offset == other.offset;
    }
};

/// Source range representing a span in source code
struct SourceRange {
    SourceLocation start;
    SourceLocation end;

    /// Default constructor
    SourceRange() = default;

    /// Constructor
    SourceRange(SourceLocation start, SourceLocation end)
        : start(std::move(start)), end(std::move(end)) {}

    /// Single location constructor
    explicit SourceRange(SourceLocation location) : start(location), end(location) {}

    /// Check if range contains a location
    [[nodiscard]] bool contains(const SourceLocation& loc) const noexcept {
        if (start.file != loc.file || start.file != end.file) {
            return false;
        }
        return (start.line < loc.line || (start.line == loc.line && start.column <= loc.column)) &&
               (end.line > loc.line || (end.line == loc.line && end.column >= loc.column));
    }

    /// Convert to string representation
    [[nodiscard]] std::string to_string() const {
        if (start == end) {
            return start.to_string();
        }
        return fmt::format("{}-{}", start.to_string(), end.to_string());
    }

    /// Check if range is valid
    [[nodiscard]] bool is_valid() const noexcept {
        return start.is_valid() && end.is_valid();
    }
};

/// Base error class with rich context information
class Error {
  protected:
    std::string message_;
    std::optional<SourceLocation> location_;
    std::vector<std::string> context_;
    std::optional<std::string> help_;

  public:
    /// Constructor with message
    explicit Error(std::string message) : message_(std::move(message)) {}

    /// Virtual destructor for inheritance
    virtual ~Error() = default;

    /// Copy constructor
    Error(const Error&) = default;

    /// Move constructor
    Error(Error&&) = default;

    /// Copy assignment
    Error& operator=(const Error&) = default;

    /// Move assignment
    Error& operator=(Error&&) = default;

    /// Add source location (builder pattern)
    Error& at(SourceLocation loc) {
        location_ = std::move(loc);
        return *this;
    }

    /// Add context information (builder pattern)
    Error& with_context(std::string ctx) {
        context_.push_back(std::move(ctx));
        return *this;
    }

    /// Add help text (builder pattern)
    Error& with_help(std::string help_text) {
        help_ = std::move(help_text);
        return *this;
    }

    /// Get the error message
    [[nodiscard]] const std::string& message() const noexcept {
        return message_;
    }

    /// Get the source location if available
    [[nodiscard]] const std::optional<SourceLocation>& location() const noexcept {
        return location_;
    }

    /// Get the context information
    [[nodiscard]] const std::vector<std::string>& context() const noexcept {
        return context_;
    }

    /// Get the help text if available
    [[nodiscard]] const std::optional<std::string>& help() const noexcept {
        return help_;
    }

    /// Format error for display
    [[nodiscard]] virtual std::string format() const {
        std::string result;

        // Add location if available
        if (location_) {
            result += fmt::format("{}: ", location_->to_string());
        }

        // Add error type and message
        result += fmt::format("error: {}\n", message_);

        // Add context information
        for (const auto& ctx : context_) {
            result += fmt::format("  note: {}\n", ctx);
        }

        // Add help text
        if (help_) {
            result += fmt::format("  help: {}\n", *help_);
        }

        return result;
    }

    /// Get error type name (for polymorphic error handling)
    [[nodiscard]] virtual std::string error_type() const {
        return "Error";
    }

    /// Equality comparison
    [[nodiscard]] bool operator==(const Error& other) const {
        return message_ == other.message_ && location_ == other.location_ &&
               context_ == other.context_ && help_ == other.help_;
    }
};

/// Parse error - issues during CMake file parsing
class ParseError : public Error {
  public:
    /// Error categories for parse errors
    enum class Category {
        UnexpectedToken,
        UnterminatedString,
        InvalidSyntax,
        UnknownCommand,
        TooManyArguments,
        TooFewArguments,
        InvalidEscape,
        UnbalancedParens,
        UnexpectedEOF
    };

  private:
    Category category_;

  public:
    /// Constructor with category and message
    ParseError(Category category, std::string message)
        : Error(std::move(message)), category_(category) {}

    /// Constructor with just message (defaults to InvalidSyntax)
    explicit ParseError(std::string message)
        : Error(std::move(message)), category_(Category::InvalidSyntax) {}

    /// Get the parse error category
    [[nodiscard]] Category category() const noexcept {
        return category_;
    }

    /// Get error type name
    [[nodiscard]] std::string error_type() const override {
        return "ParseError";
    }

    /// Get category name as string
    [[nodiscard]] static std::string category_name(Category cat) {
        switch (cat) {
        case Category::UnexpectedToken:
            return "unexpected token";
        case Category::UnterminatedString:
            return "unterminated string";
        case Category::InvalidSyntax:
            return "invalid syntax";
        case Category::UnknownCommand:
            return "unknown command";
        case Category::TooManyArguments:
            return "too many arguments";
        case Category::TooFewArguments:
            return "too few arguments";
        case Category::InvalidEscape:
            return "invalid escape sequence";
        case Category::UnbalancedParens:
            return "unbalanced parentheses";
        case Category::UnexpectedEOF:
            return "unexpected end of file";
        default:
            return "unknown parse error";
        }
    }

    /// Format with category information
    [[nodiscard]] std::string format() const override {
        std::string result;

        if (location_) {
            result += fmt::format("{}: ", location_->to_string());
        }

        result += fmt::format("parse error ({}): {}\n", category_name(category_), message_);

        for (const auto& ctx : context_) {
            result += fmt::format("  note: {}\n", ctx);
        }

        if (help_) {
            result += fmt::format("  help: {}\n", *help_);
        }

        return result;
    }
};

/// Analysis error - issues during semantic analysis
class AnalysisError : public Error {
  public:
    /// Error categories for analysis errors
    enum class Category {
        UnknownTarget,
        CircularDependency,
        MissingDependency,
        InvalidProperty,
        UnsupportedFeature,
        PlatformSpecific,
        TypeMismatch,
        UndefinedVariable,
        InvalidConfiguration
    };

  private:
    Category category_;

  public:
    /// Constructor with category and message
    AnalysisError(Category category, std::string message)
        : Error(std::move(message)), category_(category) {}

    /// Constructor with just message (defaults to InvalidConfiguration)
    explicit AnalysisError(std::string message)
        : Error(std::move(message)), category_(Category::InvalidConfiguration) {}

    /// Get the analysis error category
    [[nodiscard]] Category category() const noexcept {
        return category_;
    }

    /// Get error type name
    [[nodiscard]] std::string error_type() const override {
        return "AnalysisError";
    }

    /// Get category name as string
    [[nodiscard]] static std::string category_name(Category cat) {
        switch (cat) {
        case Category::UnknownTarget:
            return "unknown target";
        case Category::CircularDependency:
            return "circular dependency";
        case Category::MissingDependency:
            return "missing dependency";
        case Category::InvalidProperty:
            return "invalid property";
        case Category::UnsupportedFeature:
            return "unsupported feature";
        case Category::PlatformSpecific:
            return "platform-specific issue";
        case Category::TypeMismatch:
            return "type mismatch";
        case Category::UndefinedVariable:
            return "undefined variable";
        case Category::InvalidConfiguration:
            return "invalid configuration";
        default:
            return "unknown analysis error";
        }
    }

    /// Format with category information
    [[nodiscard]] std::string format() const override {
        std::string result;

        if (location_) {
            result += fmt::format("{}: ", location_->to_string());
        }

        result += fmt::format("analysis error ({}): {}\n", category_name(category_), message_);

        for (const auto& ctx : context_) {
            result += fmt::format("  note: {}\n", ctx);
        }

        if (help_) {
            result += fmt::format("  help: {}\n", *help_);
        }

        return result;
    }
};

/// Generation error - issues during Buck2 code generation
class GenerationError : public Error {
  public:
    /// Error categories for generation errors
    enum class Category {
        UnsupportedTarget,
        InvalidRule,
        MissingTemplate,
        FileWriteError,
        FormattingError,
        InvalidAttribute,
        MissingDependency
    };

  private:
    Category category_;
    std::optional<std::string> target_name_;

  public:
    /// Constructor with category and message
    GenerationError(Category category, std::string message)
        : Error(std::move(message)), category_(category) {}

    /// Constructor with just message (defaults to InvalidRule)
    explicit GenerationError(std::string message)
        : Error(std::move(message)), category_(Category::InvalidRule) {}

    /// Get the generation error category
    [[nodiscard]] Category category() const noexcept {
        return category_;
    }

    /// Set target name associated with error
    GenerationError& for_target(std::string target) {
        target_name_ = std::move(target);
        return *this;
    }

    /// Get target name if available
    [[nodiscard]] const std::optional<std::string>& target_name() const noexcept {
        return target_name_;
    }

    /// Get error type name
    [[nodiscard]] std::string error_type() const override {
        return "GenerationError";
    }

    /// Get category name as string
    [[nodiscard]] static std::string category_name(Category cat) {
        switch (cat) {
        case Category::UnsupportedTarget:
            return "unsupported target type";
        case Category::InvalidRule:
            return "invalid rule";
        case Category::MissingTemplate:
            return "missing template";
        case Category::FileWriteError:
            return "file write error";
        case Category::FormattingError:
            return "formatting error";
        case Category::InvalidAttribute:
            return "invalid attribute";
        case Category::MissingDependency:
            return "missing dependency";
        default:
            return "unknown generation error";
        }
    }

    /// Format with category and target information
    [[nodiscard]] std::string format() const override {
        std::string result;

        if (location_) {
            result += fmt::format("{}: ", location_->to_string());
        }

        result += fmt::format("generation error ({})", category_name(category_));
        if (target_name_) {
            result += fmt::format(" for target '{}'", *target_name_);
        }
        result += fmt::format(": {}\n", message_);

        for (const auto& ctx : context_) {
            result += fmt::format("  note: {}\n", ctx);
        }

        if (help_) {
            result += fmt::format("  help: {}\n", *help_);
        }

        return result;
    }
};

/// I/O error - file system and network related errors
class IOError : public Error {
  public:
    /// Error categories for I/O errors
    enum class Category {
        FileNotFound,
        PermissionDenied,
        NetworkError,
        DiskFull,
        InvalidPath,
        TimeoutError
    };

  private:
    Category category_;
    std::optional<std::string> path_;

  public:
    /// Constructor with category and message
    IOError(Category category, std::string message)
        : Error(std::move(message)), category_(category) {}

    /// Constructor with just message (defaults to InvalidPath)
    explicit IOError(std::string message)
        : Error(std::move(message)), category_(Category::InvalidPath) {}

    /// Get the I/O error category
    [[nodiscard]] Category category() const noexcept {
        return category_;
    }

    /// Set file path associated with error
    IOError& with_path(std::string file_path) {
        path_ = std::move(file_path);
        return *this;
    }

    /// Get file path if available
    [[nodiscard]] const std::optional<std::string>& path() const noexcept {
        return path_;
    }

    /// Get error type name
    [[nodiscard]] std::string error_type() const override {
        return "IOError";
    }

    /// Get category name as string
    [[nodiscard]] static std::string category_name(Category cat) {
        switch (cat) {
        case Category::FileNotFound:
            return "file not found";
        case Category::PermissionDenied:
            return "permission denied";
        case Category::NetworkError:
            return "network error";
        case Category::DiskFull:
            return "disk full";
        case Category::InvalidPath:
            return "invalid path";
        case Category::TimeoutError:
            return "timeout error";
        default:
            return "unknown I/O error";
        }
    }

    /// Format with category and path information
    [[nodiscard]] std::string format() const override {
        std::string result;

        if (location_) {
            result += fmt::format("{}: ", location_->to_string());
        }

        result += fmt::format("I/O error ({})", category_name(category_));
        if (path_) {
            result += fmt::format(" for path '{}'", *path_);
        }
        result += fmt::format(": {}\n", message_);

        for (const auto& ctx : context_) {
            result += fmt::format("  note: {}\n", ctx);
        }

        if (help_) {
            result += fmt::format("  help: {}\n", *help_);
        }

        return result;
    }
};

/// Configuration error - issues with configuration files and settings
class ConfigError : public Error {
  public:
    /// Error categories for configuration errors
    enum class Category {
        InvalidFormat,
        MissingRequired,
        InvalidValue,
        UnknownOption,
        ConflictingOptions,
        ParseError
    };

  private:
    Category category_;
    std::optional<std::string> option_name_;

  public:
    /// Constructor with category and message
    ConfigError(Category category, std::string message)
        : Error(std::move(message)), category_(category) {}

    /// Constructor with just message (defaults to InvalidFormat)
    explicit ConfigError(std::string message)
        : Error(std::move(message)), category_(Category::InvalidFormat) {}

    /// Get the configuration error category
    [[nodiscard]] Category category() const noexcept {
        return category_;
    }

    /// Set option name associated with error
    ConfigError& for_option(std::string option) {
        option_name_ = std::move(option);
        return *this;
    }

    /// Get option name if available
    [[nodiscard]] const std::optional<std::string>& option_name() const noexcept {
        return option_name_;
    }

    /// Get error type name
    [[nodiscard]] std::string error_type() const override {
        return "ConfigError";
    }

    /// Get category name as string
    [[nodiscard]] static std::string category_name(Category cat) {
        switch (cat) {
        case Category::InvalidFormat:
            return "invalid format";
        case Category::MissingRequired:
            return "missing required option";
        case Category::InvalidValue:
            return "invalid value";
        case Category::UnknownOption:
            return "unknown option";
        case Category::ConflictingOptions:
            return "conflicting options";
        case Category::ParseError:
            return "parse error";
        default:
            return "unknown configuration error";
        }
    }

    /// Format with category and option information
    [[nodiscard]] std::string format() const override {
        std::string result;

        if (location_) {
            result += fmt::format("{}: ", location_->to_string());
        }

        result += fmt::format("configuration error ({})", category_name(category_));
        if (option_name_) {
            result += fmt::format(" for option '{}'", *option_name_);
        }
        result += fmt::format(": {}\n", message_);

        for (const auto& ctx : context_) {
            result += fmt::format("  note: {}\n", ctx);
        }

        if (help_) {
            result += fmt::format("  help: {}\n", *help_);
        }

        return result;
    }
};

} // namespace finch
