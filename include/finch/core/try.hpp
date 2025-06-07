#pragma once

#include "result.hpp"

namespace finch {

/// Implementation details for TRY macros
namespace detail {

/// Helper struct to enable perfect forwarding in TRY macros
template <typename T>
struct try_helper {
    T&& value;

    explicit try_helper(T&& val) : value(std::forward<T>(val)) {}

    template <typename U>
    [[nodiscard]] auto operator>>(U&& func) -> decltype(func(std::forward<T>(value))) {
        return func(std::forward<T>(value));
    }
};

/// Create a try_helper for perfect forwarding
template <typename T>
[[nodiscard]] auto make_try_helper(T&& value) -> try_helper<T> {
    return try_helper<T>(std::forward<T>(value));
}

} // namespace detail

} // namespace finch

/// TRY macro for early return on error (GNU statement expression version)
/// This version works with GCC and Clang but not MSVC
#if defined(__GNUC__) || defined(__clang__)

#define TRY(expr)                                                                                  \
    ({                                                                                             \
        auto&& _try_result = (expr);                                                               \
        if (!_try_result.has_value()) {                                                            \
            return ::finch::Err<decltype(_try_result.error())>(std::move(_try_result.error()));    \
        }                                                                                          \
        std::move(_try_result.value());                                                            \
    })

#define TRY_WITH_CONTEXT(expr, ctx)                                                                \
    ({                                                                                             \
        auto&& _try_result = (expr);                                                               \
        if (!_try_result.has_value()) {                                                            \
            auto _try_error = std::move(_try_result.error());                                      \
            _try_error.with_context(ctx);                                                          \
            return ::finch::Err<decltype(_try_error)>(std::move(_try_error));                      \
        }                                                                                          \
        std::move(_try_result.value());                                                            \
    })

/// TRY macro for functions returning void Results
#define TRY_VOID(expr)                                                                             \
    do {                                                                                           \
        auto&& _try_result = (expr);                                                               \
        if (!_try_result.has_value()) {                                                            \
            return ::finch::Err<decltype(_try_result.error())>(std::move(_try_result.error()));    \
        }                                                                                          \
    } while (0)

#else // MSVC or other compilers without statement expressions

/// MSVC-compatible version using a helper function approach
/// Note: This version doesn't return the value directly, must be used differently

namespace finch {
namespace detail {

/// MSVC helper for TRY functionality
template <typename ResultType>
struct try_msvc_helper {
    ResultType&& result;
    bool should_return = false;

    explicit try_msvc_helper(ResultType&& res) : result(std::forward<ResultType>(res)) {
        should_return = !result.has_value();
    }

    /// Check if we should return early
    [[nodiscard]] bool should_early_return() const noexcept {
        return should_return;
    }

    /// Get the error for early return
    template <typename ErrorType = typename std::decay_t<ResultType>::error_type>
    [[nodiscard]] auto get_error() -> Result<void, ErrorType> {
        return Err<ErrorType>(std::move(result.error()));
    }

    /// Get the value (only call if !should_early_return())
    [[nodiscard]] auto get_value() -> decltype(std::move(result.value())) {
        return std::move(result.value());
    }
};

template <typename ResultType>
[[nodiscard]] auto make_try_msvc(ResultType&& result) -> try_msvc_helper<ResultType> {
    return try_msvc_helper<ResultType>(std::forward<ResultType>(result));
}

} // namespace detail
} // namespace finch

/// MSVC-compatible TRY macro (more verbose usage)
#define TRY_MSVC(var, expr)                                                                        \
    auto _try_helper_##var = ::finch::detail::make_try_msvc(expr);                                 \
    if (_try_helper_##var.should_early_return()) {                                                 \
        return _try_helper_##var.get_error();                                                      \
    }                                                                                              \
    auto var = _try_helper_##var.get_value()

/// Alternative: assign-or-return pattern for MSVC
#define TRY_ASSIGN_OR_RETURN(var, expr)                                                            \
    auto _try_result_##var = (expr);                                                               \
    if (!_try_result_##var.has_value()) {                                                          \
        return ::finch::Err<decltype(_try_result_##var.error())>(                                  \
            std::move(_try_result_##var.error()));                                                 \
    }                                                                                              \
    auto var = std::move(_try_result_##var.value())

/// For void results on MSVC
#define TRY_VOID_MSVC(expr)                                                                        \
    do {                                                                                           \
        auto&& _try_result = (expr);                                                               \
        if (!_try_result.has_value()) {                                                            \
            return ::finch::Err<decltype(_try_result.error())>(std::move(_try_result.error()));    \
        }                                                                                          \
    } while (0)

/// Use the MSVC versions as default for non-GNU compilers
#define TRY(var, expr) TRY_ASSIGN_OR_RETURN(var, expr)
#define TRY_VOID(expr) TRY_VOID_MSVC(expr)

/// For MSVC, TRY_WITH_CONTEXT requires the assign-or-return pattern
#define TRY_WITH_CONTEXT(var, expr, ctx)                                                           \
    auto _try_result_##var = (expr);                                                               \
    if (!_try_result_##var.has_value()) {                                                          \
        auto _try_error = std::move(_try_result_##var.error());                                    \
        _try_error.with_context(ctx);                                                              \
        return ::finch::Err<decltype(_try_error)>(std::move(_try_error));                          \
    }                                                                                              \
    auto var = std::move(_try_result_##var.value())

#endif

/// Utility macros for common patterns

/// Unwrap an optional, returning an error if none
#define TRY_OPTIONAL(expr, error_msg)                                                              \
    [&]() {                                                                                        \
        auto&& _opt = (expr);                                                                      \
        if (!_opt.has_value()) {                                                                   \
            return ::finch::Err<std::string>(error_msg);                                           \
        }                                                                                          \
        return ::finch::Ok(std::move(*_opt));                                                      \
    }()

/// Convert exceptions to Results
#define TRY_CATCH(expr, error_type)                                                                \
    [&]() -> ::finch::Result<decltype(expr), error_type> {                                         \
        try {                                                                                      \
            return ::finch::Ok(expr);                                                              \
        } catch (const std::exception& e) {                                                        \
            return ::finch::Err<error_type>(error_type(e.what()));                                 \
        } catch (...) {                                                                            \
            return ::finch::Err<error_type>(error_type("unknown exception"));                      \
        }                                                                                          \
    }()

/// Validate a condition, returning an error if false
#define TRY_ENSURE(condition, error)                                                               \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            return ::finch::Err<decltype(error)>(error);                                           \
        }                                                                                          \
    } while (0)

/// Validate a condition with custom error message
#define TRY_ENSURE_MSG(condition, error_type, msg)                                                 \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            return ::finch::Err<error_type>(error_type(msg));                                      \
        }                                                                                          \
    } while (0)

/// Chain multiple Results, stopping at first error
#define TRY_ALL(...)                                                                               \
    [&]() {                                                                                        \
        auto _results = std::make_tuple(__VA_ARGS__);                                              \
        /* Implementation would need template metaprogramming to handle variadic Results */        \
        /* For now, this is a placeholder for a future enhancement */                              \
        static_assert(false, "TRY_ALL not yet implemented");                                       \
    }()

/// Documentation for TRY macro usage
/*
Usage Examples:

1. Basic TRY (GNU/Clang):
```cpp
Result<int, ParseError> parse_number(const std::string& str) {
    auto trimmed = TRY(trim_string(str));  // Returns trimmed string or early returns error
    auto parsed = TRY(convert_to_int(trimmed));
    return Ok(parsed * 2);
}
```

2. MSVC-compatible TRY:
```cpp
Result<int, ParseError> parse_number(const std::string& str) {
    TRY(trimmed, trim_string(str));  // Assigns to 'trimmed' or early returns
    TRY(parsed, convert_to_int(trimmed));
    return Ok(parsed * 2);
}
```

3. With context:
```cpp
Result<void, AnalysisError> process_file(const std::string& path) {
    TRY_WITH_CONTEXT(content, read_file(path), fmt::format("while reading {}", path));
    TRY_VOID(parse_content(content));
    return Ok();
}
```

4. Error validation:
```cpp
Result<int, ConfigError> get_port(const Config& config) {
    auto port = config.get_int("port");
    TRY_ENSURE_MSG(port > 0 && port < 65536, ConfigError,
                   fmt::format("invalid port number: {}", port));
    return Ok(port);
}
```

5. Exception handling:
```cpp
Result<std::string, IOError> read_file_safe(const std::string& path) {
    return TRY_CATCH(read_file_throwing(path), IOError);
}
```
*/
