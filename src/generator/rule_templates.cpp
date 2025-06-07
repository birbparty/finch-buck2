#include <finch/generator/rule_templates.hpp>
#include <finch/generator/target_mapper.hpp>

namespace finch::generator {

// CxxLibraryTemplate implementation
std::string CxxLibraryTemplate::generate(const TargetMapper::MappedTarget& target) const {
    std::string result = "cxx_library(\n";
    result += "    name = \"" + target.name + "\",\n";

    // Add sources with proper formatting
    if (!target.srcs.empty()) {
        result += "    srcs = [\n";
        for (const auto& src : target.srcs) {
            result += "        \"" + src + "\",\n";
        }
        result += "    ],\n";
    }

    // Add headers with glob pattern if appropriate
    if (!target.headers.empty()) {
        result += "    headers = glob([\"**/*.h\", \"**/*.hpp\"]),\n";
    } else {
        // Even without explicit headers, we often want to include headers
        result += "    headers = glob([\"**/*.h\", \"**/*.hpp\"]),\n";
    }

    // Add visibility
    result += "    visibility = [\"PUBLIC\"],\n";

    // Add header namespace (same as target name by default)
    result += "    header_namespace = \"" + target.name + "\",\n";

    // Add dependencies
    if (!target.deps.empty()) {
        result += "    deps = [\n";
        for (const auto& dep : target.deps) {
            result += "        \"" + dep + "\",\n";
        }
        result += "    ],\n";
    }

    // Add custom properties
    for (const auto& [key, value] : target.properties) {
        result += "    " + key + " = " + value + ",\n";
    }

    result += ")";
    return result;
}

// CxxBinaryTemplate implementation
std::string CxxBinaryTemplate::generate(const TargetMapper::MappedTarget& target) const {
    std::string result = "cxx_binary(\n";
    result += "    name = \"" + target.name + "\",\n";

    // Add sources with proper formatting
    if (!target.srcs.empty()) {
        result += "    srcs = [\n";
        for (const auto& src : target.srcs) {
            result += "        \"" + src + "\",\n";
        }
        result += "    ],\n";
    }

    // Add headers if present
    if (!target.headers.empty()) {
        result += "    headers = [\n";
        for (const auto& header : target.headers) {
            result += "        \"" + header + "\",\n";
        }
        result += "    ],\n";
    }

    // Add dependencies
    if (!target.deps.empty()) {
        result += "    deps = [\n";
        for (const auto& dep : target.deps) {
            result += "        \"" + dep + "\",\n";
        }
        result += "    ],\n";
    }

    // Add custom properties
    for (const auto& [key, value] : target.properties) {
        result += "    " + key + " = " + value + ",\n";
    }

    result += ")";
    return result;
}

// CxxTestTemplate implementation
std::string CxxTestTemplate::generate(const TargetMapper::MappedTarget& target) const {
    std::string result = "cxx_test(\n";
    result += "    name = \"" + target.name + "\",\n";

    if (!target.srcs.empty()) {
        result += "    srcs = [";
        for (size_t i = 0; i < target.srcs.size(); ++i) {
            result += "\"" + target.srcs[i] + "\"";
            if (i < target.srcs.size() - 1)
                result += ", ";
        }
        result += "],\n";
    }

    if (!target.headers.empty()) {
        result += "    headers = [";
        for (size_t i = 0; i < target.headers.size(); ++i) {
            result += "\"" + target.headers[i] + "\"";
            if (i < target.headers.size() - 1)
                result += ", ";
        }
        result += "],\n";
    }

    if (!target.deps.empty()) {
        result += "    deps = [";
        for (size_t i = 0; i < target.deps.size(); ++i) {
            result += "\"" + target.deps[i] + "\"";
            if (i < target.deps.size() - 1)
                result += ", ";
        }
        result += "],\n";
    }

    // Add custom properties
    for (const auto& [key, value] : target.properties) {
        result += "    " + key + " = " + value + ",\n";
    }

    result += ")";
    return result;
}

// TemplateRegistry implementation
TemplateRegistry::TemplateRegistry() {
    register_default_templates();
}

TemplateRegistry::~TemplateRegistry() = default;

void TemplateRegistry::register_template(Buck2RuleType type, std::unique_ptr<RuleTemplate> tmpl) {
    templates_[type] = std::move(tmpl);
}

const RuleTemplate* TemplateRegistry::get_template(Buck2RuleType type) const {
    auto it = templates_.find(type);
    if (it != templates_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void TemplateRegistry::register_default_templates() {
    register_template(Buck2RuleType::CxxLibrary, std::make_unique<CxxLibraryTemplate>());
    register_template(Buck2RuleType::CxxBinary, std::make_unique<CxxBinaryTemplate>());
    register_template(Buck2RuleType::CxxTest, std::make_unique<CxxTestTemplate>());
}

} // namespace finch::generator
