#pragma once

#include <finch/core/error.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace finch::lexer {

/// Manages source text with efficient line/column tracking
class SourceBuffer {
  private:
    std::string content_;
    std::string filename_;
    std::vector<size_t> line_starts_; // Offset of each line start

    void compute_line_starts();

  public:
    /// Constructor with source content
    SourceBuffer(std::string content, std::string filename);

    /// Get the full source content
    [[nodiscard]] std::string_view content() const noexcept {
        return content_;
    }

    /// Get the filename
    [[nodiscard]] const std::string& filename() const noexcept {
        return filename_;
    }

    /// Get character at position
    [[nodiscard]] char at(size_t pos) const {
        return pos < content_.size() ? content_[pos] : '\0';
    }

    /// Get substring
    [[nodiscard]] std::string_view slice(size_t start, size_t end) const {
        if (start >= content_.size())
            return {};
        end = std::min(end, content_.size());
        return std::string_view(content_).substr(start, end - start);
    }

    /// Get source location from offset
    [[nodiscard]] SourceLocation location_at(size_t offset) const;

    /// Get line and column from offset
    [[nodiscard]] std::pair<size_t, size_t> line_column_at(size_t offset) const;

    /// Get line content
    [[nodiscard]] std::string_view line_content(size_t line_number) const;

    /// Get total number of lines
    [[nodiscard]] size_t line_count() const noexcept {
        return line_starts_.size();
    }

    /// Get size of content
    [[nodiscard]] size_t size() const noexcept {
        return content_.size();
    }

    /// Check if offset is valid
    [[nodiscard]] bool is_valid_offset(size_t offset) const noexcept {
        return offset < content_.size();
    }
};

} // namespace finch::lexer
