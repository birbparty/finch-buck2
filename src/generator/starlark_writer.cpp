#include <finch/generator/starlark_writer.hpp>
#include <sstream>

namespace finch::generator {

StarlarkWriter::StarlarkWriter(bool format_output) : format_output_(format_output) {}

void StarlarkWriter::add_load(const std::string& file, const std::vector<std::string>& symbols) {
    std::string load_stmt = format_load_statement(file, symbols);
    load_statements_.push_back(load_stmt);
    content_order_.push_back("load:" + std::to_string(load_statements_.size() - 1));
}

void StarlarkWriter::add_rule(const std::string& rule_text) {
    rules_.push_back(rule_text);
    content_order_.push_back("rule:" + std::to_string(rules_.size() - 1));
}

void StarlarkWriter::add_variable(const std::string& name, const std::string& value) {
    std::string var_def = name + " = " + value;
    variable_defs_.push_back(var_def);
    content_order_.push_back("var:" + std::to_string(variable_defs_.size() - 1));
}

void StarlarkWriter::add_comment(const std::string& comment) {
    std::string comment_line = "# " + comment;
    rules_.push_back(comment_line);
    content_order_.push_back("rule:" + std::to_string(rules_.size() - 1));
}

void StarlarkWriter::add_blank_line() {
    rules_.push_back("");
    content_order_.push_back("rule:" + std::to_string(rules_.size() - 1));
}

std::string StarlarkWriter::generate() const {
    std::ostringstream output;

    // First, output all load statements
    for (const auto& load : load_statements_) {
        output << load << "\n";
    }

    // Add blank line after loads if we have content
    if (!load_statements_.empty() && (!variable_defs_.empty() || !rules_.empty())) {
        output << "\n";
    }

    // Then variables
    for (const auto& var : variable_defs_) {
        output << var << "\n";
    }

    // Add blank line after variables if we have rules
    if (!variable_defs_.empty() && !rules_.empty()) {
        output << "\n";
    }

    // Finally rules and other content
    for (size_t i = 0; i < rules_.size(); ++i) {
        const auto& rule = rules_[i];

        if (rule.empty()) {
            output << "\n";
        } else if (rule.front() == '#') {
            output << rule << "\n";
        } else {
            if (format_output_) {
                output << format_starlark(rule);
            } else {
                output << rule;
            }
            output << "\n";
        }

        // Add spacing between non-empty rules
        if (i < rules_.size() - 1 && !rule.empty() && rule.front() != '#') {
            // Check if next rule is not empty or comment
            if (i + 1 < rules_.size() && !rules_[i + 1].empty() && rules_[i + 1].front() != '#') {
                output << "\n";
            }
        }
    }

    return output.str();
}

std::string StarlarkWriter::format_starlark(const std::string& code) const {
    if (!format_output_) {
        return code;
    }

    std::ostringstream formatted;
    std::istringstream input(code);
    std::string line;

    while (std::getline(input, line)) {
        // Remove trailing whitespace
        while (!line.empty() && std::isspace(line.back())) {
            line.pop_back();
        }

        formatted << line;
        if (!input.eof()) {
            formatted << "\n";
        }
    }

    return formatted.str();
}

std::string StarlarkWriter::format_load_statement(const std::string& file,
                                                  const std::vector<std::string>& symbols) const {
    std::ostringstream load;
    load << "load(" << quote_string(file) << ", ";

    if (symbols.size() == 1) {
        load << quote_string(symbols[0]);
    } else {
        for (size_t i = 0; i < symbols.size(); ++i) {
            load << quote_string(symbols[i]);
            if (i < symbols.size() - 1) {
                load << ", ";
            }
        }
    }

    load << ")";
    return load.str();
}

std::string StarlarkWriter::format_list(const std::vector<std::string>& items,
                                        const std::string& indent) const {
    if (items.empty()) {
        return "[]";
    }

    std::ostringstream list;
    list << "[";

    for (size_t i = 0; i < items.size(); ++i) {
        if (items.size() > 3 && format_output_) {
            // Multi-line format for long lists
            list << "\n" << indent << quote_string(items[i]);
        } else {
            // Single line format
            list << quote_string(items[i]);
        }

        if (i < items.size() - 1) {
            list << ",";
            if (items.size() <= 3 || !format_output_) {
                list << " ";
            }
        }
    }

    if (items.size() > 3 && format_output_) {
        list << "\n]";
    } else {
        list << "]";
    }

    return list.str();
}

std::string StarlarkWriter::quote_string(const std::string& str) const {
    return "\"" + str + "\"";
}

} // namespace finch::generator
