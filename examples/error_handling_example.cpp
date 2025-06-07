#include <finch/core/error.hpp>
#include <finch/core/error_reporter.hpp>
#include <finch/core/result.hpp>
#include <fstream>
#include <iostream>
#include <string>

using namespace finch;

// Example 1: Basic Result usage
Result<int, std::string> divide(int a, int b) {
    if (b == 0) {
        return Err<std::string, int>("Division by zero");
    }
    return Ok<int, std::string>(a / b);
}

// Example 2: File operations with rich error context
Result<std::string, IOError> read_config_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        IOError error(IOError::Category::FileNotFound, "Failed to open configuration file");
        error.with_path(path)
            .at(SourceLocation("read_config_file", 1, 1))
            .with_context("Configuration file is required for application startup")
            .with_help("Ensure the file exists and has proper read permissions");
        return Err<IOError, std::string>(std::move(error));
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (content.empty()) {
        IOError error(IOError::Category::InvalidPath, "Configuration file is empty");
        error.with_path(path).with_help("Add configuration parameters to the file");
        return Err<IOError, std::string>(std::move(error));
    }

    return Ok<std::string, IOError>(std::move(content));
}

// Example 3: Parse configuration with detailed error reporting
Result<int, ParseError> parse_port_number(const std::string& port_str) {
    if (port_str.empty()) {
        ParseError error(ParseError::Category::UnexpectedEOF, "Port number cannot be empty");
        error.with_help("Specify a port number between 1 and 65535");
        return Err<ParseError, int>(std::move(error));
    }

    // Check for non-digit characters
    for (size_t i = 0; i < port_str.size(); ++i) {
        if (!std::isdigit(port_str[i])) {
            ParseError error(ParseError::Category::InvalidSyntax,
                             "Invalid character in port number");
            error.at(SourceLocation("config", 1, i + 1))
                .with_context("Port numbers must contain only digits")
                .with_help("Remove any non-numeric characters");
            return Err<ParseError, int>(std::move(error));
        }
    }

    try {
        int port = std::stoi(port_str);
        if (port < 1 || port > 65535) {
            ParseError error(ParseError::Category::InvalidSyntax, "Port number out of valid range");
            error.with_context("Valid port range is 1-65535")
                .with_help("Choose a port number within the valid range");
            return Err<ParseError, int>(std::move(error));
        }
        return Ok<int, ParseError>(port);
    } catch (const std::exception& e) {
        ParseError error(ParseError::Category::InvalidSyntax, "Failed to parse port number");
        error.with_context(e.what()).with_help("Ensure the number is within integer range");
        return Err<ParseError, int>(std::move(error));
    }
}

// Example 4: Monadic operations - chaining operations
Result<std::string, Error> process_config_pipeline(const std::string& config_path) {
    // Read config file
    auto config_content = read_config_file(config_path);
    if (!config_content.has_value()) {
        // Convert IOError to base Error for consistent return type
        Error error("Configuration processing failed");
        error.with_context("Failed to read configuration file")
            .with_help("Check file permissions and path");
        return Err<Error, std::string>(std::move(error));
    }

    // Transform content - extract port line (simplified example)
    auto port_line = config_content.transform([](const std::string& content) -> std::string {
        size_t pos = content.find("port=");
        if (pos == std::string::npos) {
            return "";
        }
        size_t start = pos + 5; // length of "port="
        size_t end = content.find('\n', start);
        if (end == std::string::npos) {
            end = content.length();
        }
        return content.substr(start, end - start);
    });

    if (!port_line.has_value() || port_line.value().empty()) {
        Error error("No port configuration found");
        error.with_context("Configuration file must contain 'port=' line")
            .with_help("Add a line like 'port=8080' to your config file");
        return Err<Error, std::string>(std::move(error));
    }

    // Parse port number
    auto port_result = parse_port_number(port_line.value());
    if (!port_result.has_value()) {
        Error error("Invalid port configuration");
        error.with_context("Port parsing failed")
            .with_help("Fix the port number in your configuration");
        return Err<Error, std::string>(std::move(error));
    }

    return Ok<std::string, Error>("Server configured on port " +
                                  std::to_string(port_result.value()));
}

// Example 5: Error recovery with or_else
Result<int, ConfigError> get_port_with_fallback(const std::string& config_path) {
    auto config_result = read_config_file(config_path);

    return config_result
        .and_then([](const std::string& content) -> Result<std::string, ConfigError> {
            size_t pos = content.find("port=");
            if (pos == std::string::npos) {
                ConfigError error(ConfigError::Category::MissingRequired, "Port not specified");
                return Err<ConfigError, std::string>(std::move(error));
            }
            return Ok<std::string, ConfigError>(
                content.substr(pos + 5, content.find('\n', pos + 5) - pos - 5));
        })
        .and_then([](const std::string& port_str) -> Result<int, ConfigError> {
            try {
                int port = std::stoi(port_str);
                if (port < 1 || port > 65535) {
                    ConfigError error(ConfigError::Category::InvalidValue, "Port out of range");
                    return Err<ConfigError, int>(std::move(error));
                }
                return Ok<int, ConfigError>(port);
            } catch (...) {
                ConfigError error(ConfigError::Category::InvalidValue, "Invalid port format");
                return Err<ConfigError, int>(std::move(error));
            }
        })
        .or_else([](const auto& error) -> Result<int, ConfigError> {
            // Fallback to default port
            std::cout << "Using default port 8080 due to error: " << error.message() << std::endl;
            return Ok<int, ConfigError>(8080);
        });
}

int main() {
    std::cout << "=== Finch Error Handling Examples ===\n\n";

    // Example 1: Basic division
    std::cout << "1. Basic Result usage:\n";
    auto div_result = divide(10, 2);
    if (div_result.has_value()) {
        std::cout << "   10 / 2 = " << div_result.value() << std::endl;
    }

    auto div_error = divide(10, 0);
    if (div_error.has_error()) {
        std::cout << "   Error: " << div_error.error() << std::endl;
    }
    std::cout << std::endl;

    // Example 2: Error reporting
    std::cout << "2. Rich error reporting:\n";
    ParseError parse_err(ParseError::Category::InvalidSyntax, "Unexpected character '@'");
    parse_err.at(SourceLocation("config.txt", 5, 12))
        .with_context("While parsing server configuration")
        .with_context("In section [network]")
        .with_help("Remove the '@' character or escape it properly");

    ErrorReporter reporter = create_default_reporter();
    reporter.report(parse_err);
    std::cout << std::endl;

    // Example 3: Configuration parsing
    std::cout << "3. Configuration processing pipeline:\n";

    // Create a sample config file for demonstration
    {
        std::ofstream config("/tmp/sample_config.txt");
        config << "# Sample configuration\n";
        config << "host=localhost\n";
        config << "port=8080\n";
        config << "debug=true\n";
    }

    auto config_result = process_config_pipeline("/tmp/sample_config.txt");
    if (config_result.has_value()) {
        std::cout << "   Success: " << config_result.value() << std::endl;
    } else {
        std::cout << "   Configuration failed:\n";
        reporter.report(config_result.error());
    }
    std::cout << std::endl;

    // Example 4: Error recovery
    std::cout << "4. Error recovery with fallback:\n";
    auto port_result = get_port_with_fallback("/nonexistent/config.txt");
    if (port_result.has_value()) {
        std::cout << "   Using port: " << port_result.value() << std::endl;
    }
    std::cout << std::endl;

    // Example 5: Multiple error types
    std::cout << "5. Different error categories:\n";

    std::vector<std::unique_ptr<Error>> errors;

    auto parse_error =
        std::make_unique<ParseError>(ParseError::Category::UnexpectedToken, "Missing semicolon");
    parse_error->at(SourceLocation("main.cpp", 42, 15));

    auto io_error =
        std::make_unique<IOError>(IOError::Category::PermissionDenied, "Cannot write to file");
    io_error->with_path("/etc/config.conf");

    auto analysis_error = std::make_unique<AnalysisError>(
        AnalysisError::Category::CircularDependency, "Circular import detected");
    analysis_error->with_context("Library A depends on Library B");
    analysis_error->with_context("Library B depends on Library A");

    std::cout << "   Parse Error:\n";
    reporter.report(*parse_error);

    std::cout << "   I/O Error:\n";
    reporter.report(*io_error);

    std::cout << "   Analysis Error:\n";
    reporter.report(*analysis_error);

    std::cout << "\n=== Error Handling Examples Complete ===\n";

    return 0;
}
