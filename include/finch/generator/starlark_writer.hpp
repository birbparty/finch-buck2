#pragma once

#include <string>
#include <vector>

namespace finch::generator {

class StarlarkWriter {
  public:
    StarlarkWriter(bool format_output = true);

    void add_load(const std::string& file, const std::vector<std::string>& symbols);
    void add_rule(const std::string& rule_text);
    void add_variable(const std::string& name, const std::string& value);
    void add_comment(const std::string& comment);
    void add_blank_line();

    std::string generate() const;

  private:
    std::vector<std::string> load_statements_;
    std::vector<std::string> variable_defs_;
    std::vector<std::string> rules_;
    std::vector<std::string> content_order_; // Tracks order of additions
    bool format_output_;

    std::string format_starlark(const std::string& code) const;
    std::string format_load_statement(const std::string& file,
                                      const std::vector<std::string>& symbols) const;
    std::string format_list(const std::vector<std::string>& items,
                            const std::string& indent = "    ") const;
    std::string quote_string(const std::string& str) const;
};

} // namespace finch::generator
