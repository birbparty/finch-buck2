#pragma once

#include <finch/generator/target_mapper.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace finch::generator {

class RuleTemplate {
  public:
    virtual ~RuleTemplate() = default;
    virtual std::string generate(const TargetMapper::MappedTarget& target) const = 0;
    virtual std::string rule_type() const = 0;
};

class CxxLibraryTemplate : public RuleTemplate {
  public:
    std::string generate(const TargetMapper::MappedTarget& target) const override;
    std::string rule_type() const override {
        return "cxx_library";
    }
};

class CxxBinaryTemplate : public RuleTemplate {
  public:
    std::string generate(const TargetMapper::MappedTarget& target) const override;
    std::string rule_type() const override {
        return "cxx_binary";
    }
};

class CxxTestTemplate : public RuleTemplate {
  public:
    std::string generate(const TargetMapper::MappedTarget& target) const override;
    std::string rule_type() const override {
        return "cxx_test";
    }
};

class TemplateRegistry {
  public:
    TemplateRegistry();
    ~TemplateRegistry();

    void register_template(Buck2RuleType type, std::unique_ptr<RuleTemplate> tmpl);
    const RuleTemplate* get_template(Buck2RuleType type) const;
    void register_default_templates();

  private:
    std::unordered_map<Buck2RuleType, std::unique_ptr<RuleTemplate>> templates_;
};

} // namespace finch::generator
