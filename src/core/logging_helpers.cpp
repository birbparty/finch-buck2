#include <finch/core/logging_helpers.hpp>

namespace finch {

// Define thread_local static member
thread_local int LogIndent::indent_level_ = 0;

} // namespace finch
