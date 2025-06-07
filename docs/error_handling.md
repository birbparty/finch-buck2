# Finch Error Handling System

## Overview

The Finch project implements a comprehensive, type-safe error handling system inspired by Rust's `Result<T,E>` type and modern C++ error handling practices. This system provides both ergonomic error propagation and rich error context for debugging and user experience.

## Core Components

### 1. Result<T,E> Type (`include/finch/core/result.hpp`)

A type that represents either a successful value `T` or an error `E`, compatible with C++23 `std::expected` API for future migration.

#### Basic Usage

```cpp
#include <finch/core/result.hpp>

using namespace finch;

// Function that can fail
Result<int, std::string> divide(int a, int b) {
    if (b == 0) {
        return Err<std::string, int>("Division by zero");
    }
    return Ok<int, std::string>(a / b);
}

// Using the result
auto result = divide(10, 2);
if (result.has_value()) {
    std::cout << "Result: " << result.value() << std::endl;
} else {
    std::cout << "Error: " << result.error() << std::endl;
}
```

#### Factory Functions

- `Ok<T, E>(value)` - Create successful result
- `Ok<E>()` - Create successful void result  
- `Err<E, T>(error)` - Create error result

#### Monadic Operations

```cpp
// Transform values (map)
auto doubled = result.transform([](int x) { return x * 2; });

// Chain operations (flatMap/bind)
auto chained = result.and_then([](int x) -> Result<std::string, std::string> {
    return Ok<std::string, std::string>(std::to_string(x));
});

// Error recovery
auto recovered = failed_result.or_else([](const auto& error) {
    return Ok<int, std::string>(42); // default value
});

// Get value or default
int value = result.value_or(0);
```

### 2. Rich Error Hierarchy (`include/finch/core/error.hpp`)

Structured error types with rich context information including source locations, contextual messages, and help suggestions.

#### Base Error Class

```cpp
Error error("Something went wrong");
error.at(SourceLocation("file.cpp", 42, 10))
     .with_context("While processing configuration")
     .with_context("In network section")
     .with_help("Check your network settings");
```

#### Specialized Error Types

- **ParseError** - Issues during CMake file parsing
- **AnalysisError** - Issues during semantic analysis  
- **GenerationError** - Issues during Buck2 code generation
- **IOError** - File system and network related errors
- **ConfigError** - Configuration file and settings issues

#### Error Categories

Each error type has categorized subtypes:

```cpp
// Parse errors
ParseError::Category::UnexpectedToken
ParseError::Category::UnterminatedString
ParseError::Category::InvalidSyntax

// Analysis errors  
AnalysisError::Category::CircularDependency
AnalysisError::Category::MissingDependency
AnalysisError::Category::UnsupportedFeature

// And more...
```

#### Source Location Tracking

```cpp
SourceLocation loc("config.cmake", 15, 23);
SourceRange range(start_loc, end_loc);

error.at(loc);
if (range.contains(some_location)) {
    // Handle location within range
}
```

### 3. TRY Macros (`include/finch/core/try.hpp`)

Ergonomic macros for error propagation, compatible with both GCC/Clang and MSVC.

#### Basic TRY Usage (GCC/Clang)

```cpp
Result<int, std::string> complex_calculation() {
    auto x = TRY(step_one());     // Early return on error
    auto y = TRY(step_two(x));    // Chain operations
    auto z = TRY(step_three(y));  // Continue chaining
    return Ok<int, std::string>(z);
}
```

#### MSVC Compatible Version

```cpp
Result<int, std::string> complex_calculation() {
    TRY(x, step_one());           // Assign or return
    TRY(y, step_two(x));          // Continue chaining  
    TRY(z, step_three(y));        // Final step
    return Ok<int, std::string>(z);
}
```

#### Additional TRY Variants

```cpp
// For void results
TRY_VOID(operation_that_might_fail());

// With added context
TRY_WITH_CONTEXT(result, risky_operation(), "while processing data");

// Condition validation
TRY_ENSURE(port > 0 && port < 65536,
           ConfigError("Invalid port number"));

// Exception to Result conversion
auto safe_result = TRY_CATCH(throwing_function(), IOError);
```

### 4. Error Reporter (`include/finch/core/error_reporter.hpp`)

Configurable error formatting and display system supporting multiple output formats.

#### Human-Readable Format

```cpp
ErrorReporter reporter = create_default_reporter();
reporter.report(error);

// Output:
// config.cmake:15:23: error: unexpected token '{'
//   note: while parsing function definition
//   note: in target 'my_library'
//   help: check bracket matching
```

#### Structured Format (for IDEs/tools)

```cpp
auto reporter = create_structured_reporter();
reporter.report(error);

// Output:
// ERROR:config.cmake:15:23:ParseError:unexpected token '{'
// NOTE:::while parsing function definition  
// HELP:::check bracket matching
```

#### Configuration Options

```cpp
ErrorReportConfig config;
config.use_color = true;                    // Colored output
config.show_source_snippets = true;        // Code context
config.max_context_lines = 5;              // Limit context
config.format = ErrorReportConfig::Format::Human;
config.output_stream = &std::cerr;

ErrorReporter reporter(config);
```

### 5. Integration Examples

#### Chaining Operations with Rich Errors

```cpp
Result<Configuration, Error> load_config(const std::string& path) {
    // Each step can fail with detailed context
    auto content = TRY_WITH_CONTEXT(
        read_file(path),
        fmt::format("while reading config from {}", path)
    );

    auto parsed = TRY_WITH_CONTEXT(
        parse_toml(content),
        "while parsing TOML configuration"
    );

    auto validated = TRY_WITH_CONTEXT(
        validate_config(parsed),
        "while validating configuration schema"
    );

    return Ok<Configuration, Error>(std::move(validated));
}
```

#### Error Recovery Patterns

```cpp
Result<ServerConfig, ConfigError> get_server_config() {
    return load_config("server.toml")
        .or_else([](const auto& error) {
            // Try fallback location
            return load_config("/etc/myapp/server.toml");
        })
        .or_else([](const auto& error) {
            // Use defaults
            return Ok<ServerConfig, ConfigError>(ServerConfig::defaults());
        });
}
```

#### Multiple Error Collection

```cpp
std::vector<Error> errors;
ErrorReporter reporter = create_default_reporter();

// Collect multiple errors during processing
if (auto result = validate_targets(); !result.has_value()) {
    errors.push_back(result.error());
}

if (auto result = check_dependencies(); !result.has_value()) {
    errors.push_back(result.error());
}

// Report all at once
if (!errors.empty()) {
    reporter.report_all(errors);
    return EXIT_FAILURE;
}
```

## Integration with Finch Buck2

The error handling system integrates seamlessly with Finch's architecture:

### CMake Parsing Errors

```cpp
Result<CMakeAST, ParseError> parse_cmake_file(const std::string& path) {
    auto content = TRY(read_file(path).transform_error([&](const IOError& e) {
        ParseError parse_err(ParseError::Category::InvalidSyntax, "Failed to read CMake file");
        parse_err.at(SourceLocation(path, 1, 1))
               .with_context(e.message())
               .with_help("Ensure file exists and is readable");
        return parse_err;
    }));

    return parse_cmake_content(content, path);
}
```

### Analysis Phase Errors

```cpp
Result<AnalyzedProject, AnalysisError> analyze_project(const CMakeAST& ast) {
    auto targets = TRY(extract_targets(ast));

    TRY_VOID(check_circular_dependencies(targets));
    TRY_VOID(validate_target_properties(targets));

    return Ok<AnalyzedProject, AnalysisError>(
        AnalyzedProject{std::move(targets)}
    );
}
```

### Buck2 Generation Errors

```cpp
Result<void, GenerationError> generate_buck_files(const AnalyzedProject& project) {
    for (const auto& target : project.targets()) {
        auto buck_rule = TRY(
            convert_target_to_buck(target)
                .transform_error([&](const auto& e) {
                    GenerationError gen_err(
                        GenerationError::Category::UnsupportedTarget,
                        "Cannot convert target to Buck2 rule"
                    );
                    gen_err.for_target(target.name())
                          .with_context(e.message());
                    return gen_err;
                })
        );

        TRY_VOID(write_buck_file(buck_rule));
    }

    return Ok<GenerationError>();
}
```

## Testing

Comprehensive test suite in `test/core/error_handling_test.cpp` covering:

- Basic Result<T,E> operations
- Monadic transformations  
- Error hierarchy and categories
- TRY macro functionality
- Error reporter formatting
- Integration scenarios

## Examples

See `examples/error_handling_example.cpp` for complete working examples demonstrating:

- Basic Result usage
- Rich error context
- Monadic operations
- Error recovery patterns
- Multiple error types

## Best Practices

### 1. Prefer Result<T,E> over Exceptions

```cpp
// Good: Explicit error handling
Result<Data, ParseError> parse_data(const std::string& input);

// Avoid: Hidden control flow
Data parse_data(const std::string& input); // throws
```

### 2. Provide Rich Error Context

```cpp
// Good: Helpful error context
ParseError error(ParseError::Category::UnexpectedToken, "Missing closing brace");
error.at(SourceLocation(filename, line, col))
     .with_context("while parsing function body")
     .with_help("add '}' to close the function");

// Avoid: Bare error messages
return Err<std::string, Data>("parse failed");
```

### 3. Use TRY for Error Propagation

```cpp
// Good: Clear error propagation
Result<ProcessedData, Error> process() {
    auto raw = TRY(read_input());
    auto parsed = TRY(parse_data(raw));
    auto validated = TRY(validate_data(parsed));
    return Ok<ProcessedData, Error>(std::move(validated));
}

// Avoid: Manual error checking
Result<ProcessedData, Error> process() {
    auto raw = read_input();
    if (!raw.has_value()) return raw.transform_error(...);
    // ... repetitive error checking
}
```

### 4. Implement Error Recovery

```cpp
// Good: Graceful degradation
auto config = load_user_config()
    .or_else([](const auto&) { return load_default_config(); })
    .or_else([](const auto&) { return create_minimal_config(); });
```

### 5. Collect and Report Multiple Errors

```cpp
// Good: Show all validation errors at once
std::vector<Error> errors;
for (const auto& target : targets) {
    if (auto result = validate_target(target); !result.has_value()) {
        errors.push_back(result.error());
    }
}

if (!errors.empty()) {
    reporter.report_all(errors);
    return failure_result;
}
```

## Future Enhancements

- **Source Snippets**: Display relevant source code with error highlighting
- **Error Codes**: Unique identifiers for programmatic error handling  
- **Internationalization**: Multi-language error messages
- **JSON Schema**: Structured error output for tooling integration
- **Error Analytics**: Collection and analysis of error patterns
- **Recovery Suggestions**: AI-powered fix suggestions

## Conclusion

The Finch error handling system provides a robust foundation for reliable software with excellent developer experience. It combines type safety, rich context, and ergonomic APIs to make error handling both comprehensive and pleasant to use.
