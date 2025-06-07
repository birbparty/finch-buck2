#pragma once

#include "lexer.hpp"
#include <finch/core/result.hpp>
#include <vector>

namespace finch::lexer {

/// String part for interpolated strings
struct InterpolatedPart {
    enum Type { Literal, Variable };
    Type type;
    std::string value;
    SourceLocation location;
};

/// Handles string interpolation parsing
class InterpolationLexer {
  public:
    /// Parse string with ${} interpolations
    static Result<std::vector<InterpolatedPart>, ParseError>
    parse_interpolated_string(const std::string& str, const SourceLocation& base_loc);

    /// Check if string contains interpolations
    static bool has_interpolations(std::string_view str);
};

} // namespace finch::lexer
