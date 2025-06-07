#pragma once

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace finch {

/// Forward declarations for result construction helpers
template <typename T, typename E>
class Result;

namespace detail {
template <typename T>
struct is_result : std::false_type {};

template <typename T, typename E>
struct is_result<Result<T, E>> : std::true_type {};

template <typename T>
inline constexpr bool is_result_v = is_result<T>::value;
} // namespace detail

/// A type that represents either a successful value T or an error E
/// Compatible with C++23 std::expected API for future migration
template <typename T, typename E>
class [[nodiscard]] Result {
  public:
    using value_type = T;
    using error_type = E;

  private:
    std::variant<T, E> storage_;

  public:
    /// Constructor for error creation (used by Err factory)
    template <typename G>
    explicit Result(std::in_place_index_t<1>, G&& error)
        : storage_(std::in_place_index<1>, std::forward<G>(error)) {}
    /// Default constructor - creates a default-constructed value
    template <typename = void>
        requires std::is_default_constructible_v<T>
    constexpr Result() noexcept(std::is_nothrow_default_constructible_v<T>) : storage_(T{}) {}

    /// Copy constructor
    constexpr Result(const Result&) = default;

    /// Move constructor
    constexpr Result(Result&&) = default;

    /// Converting constructor from compatible Result types
    template <typename U, typename G>
        requires std::is_constructible_v<T, const U&> && std::is_constructible_v<E, const G&>
    constexpr explicit(!std::is_convertible_v<const U&, T> || !std::is_convertible_v<const G&, E>)
        Result(const Result<U, G>& other)
        : storage_(other.has_value() ? std::variant<T, E>{std::in_place_index<0>, other.value()}
                                     : std::variant<T, E>{std::in_place_index<1>, other.error()}) {}

    /// Constructor from value
    template <typename U = T>
        requires std::is_constructible_v<T, U> && (!detail::is_result_v<std::decay_t<U>>) &&
                 (!std::is_same_v<std::decay_t<U>, std::in_place_t>)
    constexpr explicit(!std::is_convertible_v<U, T>)
        Result(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>)
        : storage_(std::in_place_index<0>, std::forward<U>(value)) {}

    /// In-place value constructor
    template <typename... Args>
        requires std::is_constructible_v<T, Args...>
    constexpr explicit Result(std::in_place_t,
                              Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : storage_(std::in_place_index<0>, std::forward<Args>(args)...) {}

    /// Copy assignment
    constexpr Result& operator=(const Result&) = default;

    /// Move assignment
    constexpr Result& operator=(Result&&) = default;

    /// Assignment from value
    template <typename U = T>
        requires std::is_constructible_v<T, U> && std::is_assignable_v<T&, U> &&
                 (!detail::is_result_v<std::decay_t<U>>)
    constexpr Result& operator=(U&& value) {
        storage_.template emplace<0>(std::forward<U>(value));
        return *this;
    }

    /// Check if contains a value
    [[nodiscard]] constexpr bool has_value() const noexcept {
        return storage_.index() == 0;
    }

    /// Check if contains an error (alias for !has_value())
    [[nodiscard]] constexpr bool has_error() const noexcept {
        return !has_value();
    }

    /// Conversion to bool - true if has value
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    /// Access the value (lvalue reference)
    [[nodiscard]] constexpr T& value() & {
        if (!has_value()) {
            throw std::bad_variant_access{};
        }
        return std::get<0>(storage_);
    }

    /// Access the value (const lvalue reference)
    [[nodiscard]] constexpr const T& value() const& {
        if (!has_value()) {
            throw std::bad_variant_access{};
        }
        return std::get<0>(storage_);
    }

    /// Access the value (rvalue reference)
    [[nodiscard]] constexpr T&& value() && {
        if (!has_value()) {
            throw std::bad_variant_access{};
        }
        return std::get<0>(std::move(storage_));
    }

    /// Access the value (const rvalue reference)
    [[nodiscard]] constexpr const T&& value() const&& {
        if (!has_value()) {
            throw std::bad_variant_access{};
        }
        return std::get<0>(std::move(storage_));
    }

    /// Access the error (lvalue reference)
    [[nodiscard]] constexpr E& error() & {
        if (has_value()) {
            throw std::bad_variant_access{};
        }
        return std::get<1>(storage_);
    }

    /// Access the error (const lvalue reference)
    [[nodiscard]] constexpr const E& error() const& {
        if (has_value()) {
            throw std::bad_variant_access{};
        }
        return std::get<1>(storage_);
    }

    /// Access the error (rvalue reference)
    [[nodiscard]] constexpr E&& error() && {
        if (has_value()) {
            throw std::bad_variant_access{};
        }
        return std::get<1>(std::move(storage_));
    }

    /// Access the error (const rvalue reference)
    [[nodiscard]] constexpr const E&& error() const&& {
        if (has_value()) {
            throw std::bad_variant_access{};
        }
        return std::get<1>(std::move(storage_));
    }

    /// Get value or default
    template <typename U>
    [[nodiscard]] constexpr T value_or(U&& default_value) const& {
        return has_value() ? value() : static_cast<T>(std::forward<U>(default_value));
    }

    /// Get value or default (move version)
    template <typename U>
    [[nodiscard]] constexpr T value_or(U&& default_value) && {
        return has_value() ? std::move(value()) : static_cast<T>(std::forward<U>(default_value));
    }

    /// Transform the value if present (monadic map)
    template <typename F>
    [[nodiscard]] constexpr auto
    transform(F&& func) const& -> Result<std::invoke_result_t<F, const T&>, E> {
        using U = std::invoke_result_t<F, const T&>;
        if (has_value()) {
            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(func), value());
                return Result<void, E>{};
            } else {
                return Result<U, E>{std::invoke(std::forward<F>(func), value())};
            }
        }
        return Result<U, E>{error()};
    }

    /// Transform the value if present (move version)
    template <typename F>
    [[nodiscard]] constexpr auto transform(F&& func) && -> Result<std::invoke_result_t<F, T&&>, E> {
        using U = std::invoke_result_t<F, T&&>;
        if (has_value()) {
            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(func), std::move(value()));
                return Result<void, E>{};
            } else {
                return Result<U, E>{std::invoke(std::forward<F>(func), std::move(value()))};
            }
        }
        return Result<U, E>{std::move(error())};
    }

    /// Chain operations that return Result (monadic bind)
    template <typename F>
    [[nodiscard]] constexpr auto and_then(F&& func) const& -> std::invoke_result_t<F, const T&> {
        static_assert(detail::is_result_v<std::invoke_result_t<F, const T&>>,
                      "Function must return a Result type");
        if (has_value()) {
            return std::invoke(std::forward<F>(func), value());
        }
        using ReturnType = std::invoke_result_t<F, const T&>;
        return ReturnType{error()};
    }

    /// Chain operations that return Result (move version)
    template <typename F>
    [[nodiscard]] constexpr auto and_then(F&& func) && -> std::invoke_result_t<F, T&&> {
        static_assert(detail::is_result_v<std::invoke_result_t<F, T&&>>,
                      "Function must return a Result type");
        if (has_value()) {
            return std::invoke(std::forward<F>(func), std::move(value()));
        }
        using ReturnType = std::invoke_result_t<F, T&&>;
        return ReturnType{std::move(error())};
    }

    /// Transform the error if present
    template <typename F>
    [[nodiscard]] constexpr auto
    transform_error(F&& func) const& -> Result<T, std::invoke_result_t<F, const E&>> {
        using G = std::invoke_result_t<F, const E&>;
        if (has_value()) {
            return Result<T, G>{value()};
        }
        return Result<T, G>{std::invoke(std::forward<F>(func), error())};
    }

    /// Transform the error if present (move version)
    template <typename F>
    [[nodiscard]] constexpr auto
    transform_error(F&& func) && -> Result<T, std::invoke_result_t<F, E&&>> {
        using G = std::invoke_result_t<F, E&&>;
        if (has_value()) {
            return Result<T, G>{std::move(value())};
        }
        return Result<T, G>{std::invoke(std::forward<F>(func), std::move(error()))};
    }

    /// Provide alternative Result on error
    template <typename F>
    [[nodiscard]] constexpr auto or_else(F&& func) const& -> std::invoke_result_t<F, const E&> {
        static_assert(detail::is_result_v<std::invoke_result_t<F, const E&>>,
                      "Function must return a Result type");
        if (has_value()) {
            using ReturnType = std::invoke_result_t<F, const E&>;
            return ReturnType{value()};
        }
        return std::invoke(std::forward<F>(func), error());
    }

    /// Provide alternative Result on error (move version)
    template <typename F>
    [[nodiscard]] constexpr auto or_else(F&& func) && -> std::invoke_result_t<F, E&&> {
        static_assert(detail::is_result_v<std::invoke_result_t<F, E&&>>,
                      "Function must return a Result type");
        if (has_value()) {
            using ReturnType = std::invoke_result_t<F, E&&>;
            return ReturnType{std::move(value())};
        }
        return std::invoke(std::forward<F>(func), std::move(error()));
    }

    /// Equality comparison
    template <typename T2, typename E2>
    [[nodiscard]] friend constexpr bool operator==(const Result& lhs, const Result<T2, E2>& rhs) {
        if (lhs.has_value() != rhs.has_value()) {
            return false;
        }
        return lhs.has_value() ? lhs.value() == rhs.value() : lhs.error() == rhs.error();
    }

    /// Compare with value
    template <typename T2>
    [[nodiscard]] friend constexpr bool operator==(const Result& lhs, const T2& rhs) {
        return lhs.has_value() && lhs.value() == rhs;
    }

    /// Friend function for Err factory
    template <typename E2, typename T2>
    friend constexpr auto Err(E2&& error) -> Result<T2, std::decay_t<E2>>;
};

/// Specialization for void value type
template <typename E>
class [[nodiscard]] Result<void, E> {
  public:
    using value_type = void;
    using error_type = E;

  private:
    std::optional<E> error_;

  public:
    /// Default constructor - success
    constexpr Result() noexcept = default;

    /// Copy constructor
    constexpr Result(const Result&) = default;

    /// Move constructor
    constexpr Result(Result&&) = default;

    /// Copy assignment
    constexpr Result& operator=(const Result&) = default;

    /// Move assignment
    constexpr Result& operator=(Result&&) = default;

    /// Check if successful
    [[nodiscard]] constexpr bool has_value() const noexcept {
        return !error_.has_value();
    }

    /// Check if has error
    [[nodiscard]] constexpr bool has_error() const noexcept {
        return error_.has_value();
    }

    /// Conversion to bool
    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    /// Access the error
    [[nodiscard]] constexpr E& error() & {
        return error_.value();
    }

    [[nodiscard]] constexpr const E& error() const& {
        return error_.value();
    }

    [[nodiscard]] constexpr E&& error() && {
        return std::move(error_.value());
    }

    [[nodiscard]] constexpr const E&& error() const&& {
        return std::move(error_.value());
    }

    /// Transform with void-returning function
    template <typename F>
    [[nodiscard]] constexpr auto transform(F&& func) const& -> Result<std::invoke_result_t<F>, E> {
        using U = std::invoke_result_t<F>;
        if (has_value()) {
            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(func));
                return Result<void, E>{};
            } else {
                return Result<U, E>{std::invoke(std::forward<F>(func))};
            }
        }
        return Result<U, E>{error()};
    }

    /// Chain operations
    template <typename F>
    [[nodiscard]] constexpr auto and_then(F&& func) const& -> std::invoke_result_t<F> {
        static_assert(detail::is_result_v<std::invoke_result_t<F>>,
                      "Function must return a Result type");
        if (has_value()) {
            return std::invoke(std::forward<F>(func));
        }
        using ReturnType = std::invoke_result_t<F>;
        return ReturnType{error()};
    }

    /// Create error result
    template <typename G>
        requires std::is_constructible_v<E, G>
    static constexpr Result error(G&& err) {
        Result result;
        result.error_ = std::forward<G>(err);
        return result;
    }
};

/// Factory functions for creating Results

/// Create a successful Result
template <typename T, typename E = void>
[[nodiscard]] constexpr auto Ok(T&& value) -> Result<std::decay_t<T>, E> {
    return Result<std::decay_t<T>, E>{std::forward<T>(value)};
}

/// Create a successful void Result
template <typename E = void>
[[nodiscard]] constexpr auto Ok() -> Result<void, E> {
    return Result<void, E>{};
}

/// Create an error Result
template <typename E, typename T = void>
[[nodiscard]] constexpr auto Err(E&& error) -> Result<T, std::decay_t<E>> {
    if constexpr (std::is_void_v<T>) {
        return Result<void, std::decay_t<E>>::error(std::forward<E>(error));
    } else {
        return Result<T, std::decay_t<E>>(std::in_place_index<1>, std::forward<E>(error));
    }
}

} // namespace finch
