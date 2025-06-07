#pragma once

#include <finch/core/error.hpp>
#include <finch/core/result.hpp>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace finch::analyzer {
struct Target;
}

namespace finch::generator {

enum class Buck2RuleType {
    CxxLibrary,
    CxxBinary,
    CxxTest,
    FileGroup,
    PrebuiltCxxLibrary,
    HttpArchive,
    Unknown
};

struct PlatformSelect {
    struct SelectClause {
        std::string condition;
        std::string value;
    };

    std::vector<SelectClause> clauses;
    std::optional<std::string> default_value;
};

class TargetMapper {
  public:
    struct MappedTarget {
        std::string name;
        Buck2RuleType rule_type;
        std::vector<std::string> srcs;
        std::vector<std::string> headers;
        std::vector<std::string> deps;
        std::map<std::string, std::string> properties;
        std::optional<PlatformSelect> platform_config;
    };

    TargetMapper();
    ~TargetMapper();

    Result<MappedTarget, GenerationError> map_cmake_target(const analyzer::Target& cmake_target);

  private:
    Buck2RuleType determine_rule_type(const analyzer::Target& target);
    std::vector<std::string> transform_sources(const std::vector<std::string>& sources);
    std::vector<std::string> resolve_dependencies(const std::vector<std::string>& deps);
    std::string normalize_target_name(const std::string& cmake_name);
};

} // namespace finch::generator
