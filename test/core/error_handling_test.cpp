#include <finch/core/error.hpp>
#include <finch/core/error_reporter.hpp>
#include <finch/core/result.hpp>
#include <finch/core/try.hpp>
#include <gtest/gtest.h>
#include <sstream>

using namespace finch;

// Test fixtures and helper functions
class ErrorHandlingTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Reset any global state if needed
    }

    void TearDown() override {
        // Clean up after each test
    }
};

// Basic Result<T,E> functionality tests
TEST_F(ErrorHandlingTest, ResultBasicSuccess) {
    auto result = Ok<int, std::string>(42);

    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.has_error());
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_EQ(result.value(), 42);
}

TEST_F(ErrorHandlingTest, ResultBasicError) {
    Result<int, std::string> result = Result<int, std::string>(std::in_place_index<1>, "failed");

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.has_error());
    EXPECT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(result.error(), "failed");
}

TEST_F(ErrorHandlingTest, ResultValueOr) {
    auto success = Ok<int, std::string>(42);
    Result<int, std::string> failure = Result<int, std::string>(std::in_place_index<1>, "failed");

    EXPECT_EQ(success.value_or(0), 42);
    EXPECT_EQ(failure.value_or(0), 0);
}

// Monadic operations tests
TEST_F(ErrorHandlingTest, ResultTransform) {
    auto result = Ok<int, std::string>(42);

    auto transformed = result.transform([](int x) { return x * 2; });

    EXPECT_TRUE(transformed.has_value());
    EXPECT_EQ(transformed.value(), 84);
}

TEST_F(ErrorHandlingTest, ResultTransformError) {
    auto result = Err<std::string, int>("failed");

    auto transformed = result.transform([](int x) { return x * 2; });

    EXPECT_FALSE(transformed.has_value());
    EXPECT_EQ(transformed.error(), "failed");
}

TEST_F(ErrorHandlingTest, ResultAndThen) {
    auto result = Ok<int, std::string>(42);

    auto chained = result.and_then([](int x) -> Result<std::string, std::string> {
        if (x > 0) {
            return Ok<std::string, std::string>(std::to_string(x));
        }
        return Err<std::string, std::string>("negative");
    });

    EXPECT_TRUE(chained.has_value());
    EXPECT_EQ(chained.value(), "42");
}

TEST_F(ErrorHandlingTest, ResultAndThenError) {
    auto result = Err<std::string, int>("failed");

    auto chained = result.and_then([](int x) -> Result<std::string, std::string> {
        return Ok<std::string, std::string>(std::to_string(x));
    });

    EXPECT_FALSE(chained.has_value());
    EXPECT_EQ(chained.error(), "failed");
}

TEST_F(ErrorHandlingTest, ResultOrElse) {
    auto failure = Err<std::string, int>("failed");

    auto recovered = failure.or_else([](const std::string& error) -> Result<int, std::string> {
        if (error == "failed") {
            return Ok<int, std::string>(99);
        }
        return Err<std::string, int>("unrecoverable");
    });

    EXPECT_TRUE(recovered.has_value());
    EXPECT_EQ(recovered.value(), 99);
}

// Void Result tests
TEST_F(ErrorHandlingTest, VoidResultSuccess) {
    auto result = Ok<std::string>();

    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.has_error());
}

TEST_F(ErrorHandlingTest, VoidResultError) {
    auto result = Result<void, std::string>::error("failed");

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.has_error());
    EXPECT_EQ(result.error(), "failed");
}

// Error hierarchy tests
TEST_F(ErrorHandlingTest, BasicError) {
    Error error("test message");

    EXPECT_EQ(error.message(), "test message");
    EXPECT_FALSE(error.location().has_value());
    EXPECT_TRUE(error.context().empty());
    EXPECT_FALSE(error.help().has_value());
}

TEST_F(ErrorHandlingTest, ErrorWithLocation) {
    SourceLocation loc("test.cpp", 10, 5);
    Error error("test message");
    error.at(loc);

    EXPECT_TRUE(error.location().has_value());
    EXPECT_EQ(error.location()->file, "test.cpp");
    EXPECT_EQ(error.location()->line, 10);
    EXPECT_EQ(error.location()->column, 5);
}

TEST_F(ErrorHandlingTest, ErrorWithContext) {
    Error error("test message");
    error.with_context("first context").with_context("second context");

    EXPECT_EQ(error.context().size(), 2);
    EXPECT_EQ(error.context()[0], "first context");
    EXPECT_EQ(error.context()[1], "second context");
}

TEST_F(ErrorHandlingTest, ErrorWithHelp) {
    Error error("test message");
    error.with_help("try this instead");

    EXPECT_TRUE(error.help().has_value());
    EXPECT_EQ(*error.help(), "try this instead");
}

TEST_F(ErrorHandlingTest, ParseErrorCategory) {
    ParseError error(ParseError::Category::UnexpectedToken, "unexpected '{'");

    EXPECT_EQ(error.category(), ParseError::Category::UnexpectedToken);
    EXPECT_EQ(error.message(), "unexpected '{'");
    EXPECT_EQ(error.error_type(), "ParseError");
}

TEST_F(ErrorHandlingTest, AnalysisErrorCategory) {
    AnalysisError error(AnalysisError::Category::CircularDependency, "circular reference detected");

    EXPECT_EQ(error.category(), AnalysisError::Category::CircularDependency);
    EXPECT_EQ(error.message(), "circular reference detected");
    EXPECT_EQ(error.error_type(), "AnalysisError");
}

TEST_F(ErrorHandlingTest, GenerationErrorWithTarget) {
    GenerationError error(GenerationError::Category::UnsupportedTarget, "cannot generate rule");
    error.for_target("my_target");

    EXPECT_EQ(error.category(), GenerationError::Category::UnsupportedTarget);
    EXPECT_TRUE(error.target_name().has_value());
    EXPECT_EQ(*error.target_name(), "my_target");
}

TEST_F(ErrorHandlingTest, IOErrorWithPath) {
    IOError error(IOError::Category::FileNotFound, "file does not exist");
    error.with_path("/path/to/file.txt");

    EXPECT_EQ(error.category(), IOError::Category::FileNotFound);
    EXPECT_TRUE(error.path().has_value());
    EXPECT_EQ(*error.path(), "/path/to/file.txt");
}

TEST_F(ErrorHandlingTest, ConfigErrorWithOption) {
    ConfigError error(ConfigError::Category::InvalidValue, "invalid port number");
    error.for_option("server.port");

    EXPECT_EQ(error.category(), ConfigError::Category::InvalidValue);
    EXPECT_TRUE(error.option_name().has_value());
    EXPECT_EQ(*error.option_name(), "server.port");
}

// TRY macro tests (GNU/Clang version)
#if defined(__GNUC__) || defined(__clang__)

// Helper functions for TRY tests
Result<int, std::string> divide(int a, int b) {
    if (b == 0) {
        return Err<std::string, int>("division by zero");
    }
    return Ok<int, std::string>(a / b);
}

Result<int, std::string> calculate() {
    auto x = TRY(divide(10, 2));        // Should be 5
    auto y = TRY(divide(x, 1));         // Should be 5
    return Ok<int, std::string>(x + y); // Should be 10
}

Result<int, std::string> calculate_with_error() {
    auto x = TRY(divide(10, 2));        // Should be 5
    auto y = TRY(divide(x, 0));         // Should fail here
    return Ok<int, std::string>(x + y); // Should not reach this
}

TEST_F(ErrorHandlingTest, TryMacroSuccess) {
    auto result = calculate();

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 10);
}

TEST_F(ErrorHandlingTest, TryMacroError) {
    auto result = calculate_with_error();

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "division by zero");
}

Result<void, std::string> process_void() {
    TRY_VOID(divide(10, 2)); // Check it doesn't fail
    TRY_VOID(divide(5, 1));  // Check it doesn't fail
    return Ok<std::string>();
}

Result<void, std::string> process_void_with_error() {
    TRY_VOID(divide(10, 2)); // Check it doesn't fail
    TRY_VOID(divide(5, 0));  // Should fail here
    return Ok<std::string>();
}

TEST_F(ErrorHandlingTest, TryVoidSuccess) {
    auto result = process_void();

    EXPECT_TRUE(result.has_value());
}

TEST_F(ErrorHandlingTest, TryVoidError) {
    auto result = process_void_with_error();

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "division by zero");
}

#endif // GNU/Clang TRY tests

// Error reporter tests
TEST_F(ErrorHandlingTest, ErrorReporterHuman) {
    std::ostringstream oss;
    ErrorReportConfig config;
    config.output_stream = &oss;
    config.use_color = false; // Disable color for testing

    ErrorReporter reporter(config);

    ParseError error(ParseError::Category::UnexpectedToken, "unexpected '{'");
    error.at(SourceLocation("test.cpp", 10, 5))
        .with_context("while parsing function")
        .with_help("check bracket matching");

    reporter.report(error);

    std::string output = oss.str();
    EXPECT_NE(output.find("test.cpp:10:5"), std::string::npos);
    EXPECT_NE(output.find("error:"), std::string::npos);
    EXPECT_NE(output.find("unexpected '{'"), std::string::npos);
    EXPECT_NE(output.find("note: while parsing function"), std::string::npos);
    EXPECT_NE(output.find("help: check bracket matching"), std::string::npos);
}

TEST_F(ErrorHandlingTest, ErrorReporterStructured) {
    std::ostringstream oss;
    ErrorReportConfig config;
    config.output_stream = &oss;
    config.format = ErrorReportConfig::Format::Structured;
    config.use_color = false;

    ErrorReporter reporter(config);

    ParseError error(ParseError::Category::UnexpectedToken, "unexpected '{'");
    error.at(SourceLocation("test.cpp", 10, 5))
        .with_context("while parsing function")
        .with_help("check bracket matching");

    reporter.report(error);

    std::string output = oss.str();
    EXPECT_NE(output.find("ERROR:test.cpp:10:5:ParseError:unexpected '{'"), std::string::npos);
    EXPECT_NE(output.find("NOTE:::while parsing function"), std::string::npos);
    EXPECT_NE(output.find("HELP:::check bracket matching"), std::string::npos);
}

TEST_F(ErrorHandlingTest, ErrorReporterMultiple) {
    std::ostringstream oss;
    ErrorReportConfig config;
    config.output_stream = &oss;
    config.use_color = false;

    ErrorReporter reporter(config);

    std::vector<Error> errors;
    errors.emplace_back("first error");
    errors.emplace_back("second error");

    reporter.report_all(errors);

    std::string output = oss.str();
    EXPECT_NE(output.find("first error"), std::string::npos);
    EXPECT_NE(output.find("second error"), std::string::npos);
    EXPECT_NE(output.find("found 2 errors"), std::string::npos);
}

// Source location tests
TEST_F(ErrorHandlingTest, SourceLocationBasic) {
    SourceLocation loc("test.cpp", 42, 10, 500);

    EXPECT_EQ(loc.file, "test.cpp");
    EXPECT_EQ(loc.line, 42);
    EXPECT_EQ(loc.column, 10);
    EXPECT_EQ(loc.offset, 500);
    EXPECT_TRUE(loc.is_valid());
    EXPECT_EQ(loc.to_string(), "test.cpp:42:10");
}

TEST_F(ErrorHandlingTest, SourceLocationInvalid) {
    SourceLocation loc; // Default constructor

    EXPECT_FALSE(loc.is_valid());
}

TEST_F(ErrorHandlingTest, SourceRangeBasic) {
    SourceLocation start("test.cpp", 10, 5);
    SourceLocation end("test.cpp", 10, 15);
    SourceRange range(start, end);

    EXPECT_TRUE(range.is_valid());
    EXPECT_EQ(range.to_string(), "test.cpp:10:5-test.cpp:10:15");

    SourceLocation middle("test.cpp", 10, 8);
    EXPECT_TRUE(range.contains(middle));

    SourceLocation outside("test.cpp", 11, 5);
    EXPECT_FALSE(range.contains(outside));
}

TEST_F(ErrorHandlingTest, SourceRangeEqual) {
    SourceLocation loc("test.cpp", 10, 5);
    SourceRange range(loc); // Single location constructor

    EXPECT_EQ(range.to_string(), "test.cpp:10:5");
}

// Integration test - realistic error handling scenario
TEST_F(ErrorHandlingTest, IntegrationTest) {
    // Simulate a parsing scenario
    auto parse_result = [](const std::string& input) -> Result<int, ParseError> {
        if (input.empty()) {
            ParseError error(ParseError::Category::UnexpectedEOF, "empty input");
            error.at(SourceLocation("input", 1, 1)).with_help("provide a valid number");
            return Err<ParseError, int>(std::move(error));
        }

        if (input == "abc") {
            ParseError error(ParseError::Category::InvalidSyntax, "not a number");
            error.at(SourceLocation("input", 1, 1))
                .with_context("expected numeric value")
                .with_help("use digits 0-9");
            return Err<ParseError, int>(std::move(error));
        }

        return Ok<int, ParseError>(std::stoi(input));
    };

    // Test successful case
    auto success = parse_result("42");
    EXPECT_TRUE(success.has_value());
    EXPECT_EQ(success.value(), 42);

    // Test error case
    auto failure = parse_result("abc");
    EXPECT_FALSE(failure.has_value());
    EXPECT_EQ(failure.error().category(), ParseError::Category::InvalidSyntax);
    EXPECT_EQ(failure.error().message(), "not a number");
    EXPECT_TRUE(failure.error().location().has_value());
    EXPECT_EQ(failure.error().context().size(), 1);
    EXPECT_TRUE(failure.error().help().has_value());

    // Test error reporting
    std::ostringstream oss;
    ErrorReportConfig config;
    config.output_stream = &oss;
    config.use_color = false;

    ErrorReporter reporter(config);
    reporter.report(failure.error());

    std::string output = oss.str();
    EXPECT_NE(output.find("input:1:1"), std::string::npos);
    EXPECT_NE(output.find("not a number"), std::string::npos);
    EXPECT_NE(output.find("expected numeric value"), std::string::npos);
    EXPECT_NE(output.find("use digits 0-9"), std::string::npos);
}
