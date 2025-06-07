#include <algorithm>
#include <finch/parser/lexer/source_buffer.hpp>

namespace finch::lexer {

SourceBuffer::SourceBuffer(std::string content, std::string filename)
    : content_(std::move(content)), filename_(std::move(filename)) {
    compute_line_starts();
}

void SourceBuffer::compute_line_starts() {
    line_starts_.clear();
    line_starts_.push_back(0); // First line starts at offset 0

    for (size_t i = 0; i < content_.size(); ++i) {
        if (content_[i] == '\n') {
            line_starts_.push_back(i + 1);
        }
    }
}

SourceLocation SourceBuffer::location_at(size_t offset) const {
    auto [line, column] = line_column_at(offset);
    return SourceLocation{filename_, line, column, offset};
}

std::pair<size_t, size_t> SourceBuffer::line_column_at(size_t offset) const {
    if (offset >= content_.size()) {
        offset = content_.size();
    }

    // Binary search for the line
    auto it = std::upper_bound(line_starts_.begin(), line_starts_.end(), offset);
    size_t line_idx = std::distance(line_starts_.begin(), it) - 1;

    size_t line = line_idx + 1;                          // 1-based line number
    size_t column = offset - line_starts_[line_idx] + 1; // 1-based column

    return {line, column};
}

std::string_view SourceBuffer::line_content(size_t line_number) const {
    if (line_number == 0 || line_number > line_starts_.size()) {
        return {};
    }

    size_t line_idx = line_number - 1;
    size_t start = line_starts_[line_idx];

    // Find the end of the line
    size_t end = content_.size();
    if (line_idx + 1 < line_starts_.size()) {
        end = line_starts_[line_idx + 1] - 1; // Exclude the newline
    }

    // Also exclude \r if present
    if (end > start && end <= content_.size() && content_[end - 1] == '\r') {
        --end;
    }

    return slice(start, end);
}

} // namespace finch::lexer
