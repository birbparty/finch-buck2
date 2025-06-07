#include <finch/core/logging.hpp>
#include <finch/parser/ast/literals.hpp>
#include <finch/parser/cpm_parser.hpp>
#include <fmt/format.h>
#include <regex>

namespace finch::parser {

namespace {
// Regular expressions for GitHub shorthand parsing
const std::regex GITHUB_SHORTHAND_REGEX(R"(^(?:gh:)?([^/@#]+)/([^/@#]+)(?:[@#](.+))?$)");
const std::regex VERSION_EXACT_REGEX(R"(^@(.+)$)");
const std::regex VERSION_MIN_REGEX(R"(^>=(.+)$)");
} // namespace

Result<ast::ASTNodePtr, ParseError> CPMParser::parse_cpm_command(const std::string& command_name,
                                                                 const ast::ASTNodeList& args) {
    LOG_DEBUG("Parsing CPM command: {}", command_name);

    if (command_name == "CPMAddPackage") {
        auto result = parse_cpm_add_package(args);
        if (result.has_value()) {
            return Ok<ast::ASTNodePtr, ParseError>(std::move(result.value()));
        }
        return Result<ast::ASTNodePtr, ParseError>{std::in_place_index<1>, result.error()};
    } else if (command_name == "CPMFindPackage") {
        auto result = parse_cpm_find_package(args);
        if (result.has_value()) {
            return Ok<ast::ASTNodePtr, ParseError>(std::move(result.value()));
        }
        return Result<ast::ASTNodePtr, ParseError>{std::in_place_index<1>, result.error()};
    } else if (command_name == "CPMUsePackageLock") {
        auto result = parse_cpm_use_package_lock(args);
        if (result.has_value()) {
            return Ok<ast::ASTNodePtr, ParseError>(std::move(result.value()));
        }
        return Result<ast::ASTNodePtr, ParseError>{std::in_place_index<1>, result.error()};
    } else if (command_name == "CPMDeclarePackage") {
        auto result = parse_cpm_declare_package(args);
        if (result.has_value()) {
            return Ok<ast::ASTNodePtr, ParseError>(std::move(result.value()));
        }
        return Result<ast::ASTNodePtr, ParseError>{std::in_place_index<1>, result.error()};
    }

    return Err<ParseError, ast::ASTNodePtr>(
        ParseError(ParseError::Category::UnknownCommand, "Unknown CPM command: " + command_name));
}

Result<std::unique_ptr<ast::CPMAddPackage>, ParseError>
CPMParser::parse_cpm_add_package(const ast::ASTNodeList& args) {
    if (args.empty()) {
        return Err<ParseError, std::unique_ptr<ast::CPMAddPackage>>(
            ParseError("CPMAddPackage requires arguments"));
    }

    // Check for shorthand syntax (single string argument)
    if (args.size() == 1) {
        auto str_result = get_string_value(args[0].get());
        if (str_result.has_value()) {
            const auto& str = str_result.value();
            if (is_github_shorthand(str)) {
                return parse_cpm_add_package_shorthand(str);
            }
        }
    }

    // Full syntax parsing
    return parse_cpm_add_package_full(args);
}

Result<std::unique_ptr<ast::CPMAddPackage>, ParseError>
CPMParser::parse_cpm_add_package_shorthand(const std::string& shorthand) {
    LOG_DEBUG("Parsing CPM shorthand: {}", shorthand);

    auto gh_result = parse_github_shorthand(shorthand);
    if (!gh_result.has_value()) {
        return Result<std::unique_ptr<ast::CPMAddPackage>, ParseError>{std::in_place_index<1>,
                                                                       gh_result.error()};
    }

    const auto& [repo_spec, version_str] = gh_result.value();

    // Extract owner and repo name
    auto slash_pos = repo_spec.find('/');
    if (slash_pos == std::string::npos) {
        return Err<ParseError, std::unique_ptr<ast::CPMAddPackage>>(
            ParseError("Invalid GitHub repository format"));
    }

    std::string owner = repo_spec.substr(0, slash_pos);
    std::string repo = repo_spec.substr(slash_pos + 1);

    auto package = std::make_unique<ast::CPMAddPackage>(SourceLocation{}, repo);
    package->set_source(ast::CPMSourceType::GitHub, repo_spec);

    if (!version_str.empty()) {
        auto version = parse_version_string(version_str);
        if (version.has_value()) {
            package->set_version(version.value());
        } else {
            return Result<std::unique_ptr<ast::CPMAddPackage>, ParseError>{std::in_place_index<1>,
                                                                           version.error()};
        }
    }

    return Ok<std::unique_ptr<ast::CPMAddPackage>, ParseError>(std::move(package));
}

Result<std::unique_ptr<ast::CPMAddPackage>, ParseError>
CPMParser::parse_cpm_add_package_full(const ast::ASTNodeList& args) {
    std::string name;
    std::unique_ptr<ast::CPMAddPackage> package;
    SourceLocation loc = args.empty() ? SourceLocation{} : args[0]->location();

    size_t i = 0;

    // First, try to find NAME argument
    auto* name_node = find_argument(args, "NAME");
    if (name_node) {
        // Find the value after NAME
        for (size_t j = 0; j < args.size() - 1; ++j) {
            if (args[j].get() == name_node) {
                auto name_result = get_string_value(args[j + 1].get());
                if (name_result.has_value()) {
                    name = name_result.value();
                    package = std::make_unique<ast::CPMAddPackage>(loc, name);
                    break;
                }
            }
        }
    } else if (args.size() > 0) {
        // Try first argument as name
        auto name_result = get_string_value(args[0].get());
        if (name_result.has_value() && !name_result.value().empty()) {
            name = name_result.value();
            package = std::make_unique<ast::CPMAddPackage>(loc, name);
            i = 1; // Start processing from next argument
        }
    }

    if (!package) {
        return Err<ParseError, std::unique_ptr<ast::CPMAddPackage>>(
            ParseError("CPMAddPackage requires NAME"));
    }

    // Parse remaining arguments
    for (; i < args.size(); ++i) {
        auto key_result = get_string_value(args[i].get());
        if (!key_result.has_value()) {
            continue;
        }

        const auto& key = key_result.value();

        if (key == "GITHUB_REPOSITORY" && i + 1 < args.size()) {
            auto value_result = get_string_value(args[i + 1].get());
            if (value_result.has_value()) {
                package->set_source(ast::CPMSourceType::GitHub, value_result.value());
                i++;
            }
        } else if (key == "GIT_REPOSITORY" && i + 1 < args.size()) {
            auto value_result = get_string_value(args[i + 1].get());
            if (value_result.has_value()) {
                package->set_source(ast::CPMSourceType::GitURL, value_result.value());
                i++;
            }
        } else if (key == "URL" && i + 1 < args.size()) {
            auto value_result = get_string_value(args[i + 1].get());
            if (value_result.has_value()) {
                package->set_source(ast::CPMSourceType::URL, value_result.value());
                i++;
            }
        } else if (key == "VERSION" && i + 1 < args.size()) {
            auto value_result = get_string_value(args[i + 1].get());
            if (value_result.has_value()) {
                auto version = parse_version_string(value_result.value());
                if (version.has_value()) {
                    package->set_version(version.value());
                }
                i++;
            }
        } else if (key == "GIT_TAG" && i + 1 < args.size()) {
            auto value_result = get_string_value(args[i + 1].get());
            if (value_result.has_value()) {
                ast::CPMVersion version;
                version.git_tag = value_result.value();
                version.version = value_result.value();
                package->set_version(version);
                i++;
            }
        } else if (key == "OPTIONS") {
            // Collect all arguments until the next keyword
            ast::ASTNodeList option_args;
            i++;
            while (i < args.size()) {
                auto str_result = get_string_value(args[i].get());
                if (str_result.has_value()) {
                    const auto& str = str_result.value();
                    // Check if this is a CPM keyword
                    if (str == "DOWNLOAD_ONLY" || str == "EXCLUDE_FROM_ALL" || str == "SYSTEM" ||
                        str == "NO_CACHE") {
                        i--; // Back up one
                        break;
                    }
                }
                option_args.push_back(args[i]->clone());
                i++;
            }
            auto options_result = parse_options_block(*package, option_args);
            if (!options_result.has_value()) {
                return Result<std::unique_ptr<ast::CPMAddPackage>, ParseError>{
                    std::in_place_index<1>, options_result.error()};
            }
        }
    }

    return Ok<std::unique_ptr<ast::CPMAddPackage>, ParseError>(std::move(package));
}

Result<std::unique_ptr<ast::CPMFindPackage>, ParseError>
CPMParser::parse_cpm_find_package(const ast::ASTNodeList& args) {
    if (args.empty()) {
        return Err<ParseError, std::unique_ptr<ast::CPMFindPackage>>(
            ParseError("CPMFindPackage requires arguments"));
    }

    std::string name;
    SourceLocation loc = args[0]->location();

    // First argument should be the package name
    auto name_result = get_string_value(args[0].get());
    if (!name_result.has_value()) {
        // Try NAME keyword
        auto* name_node = find_argument(args, "NAME");
        if (name_node) {
            for (size_t i = 0; i < args.size() - 1; ++i) {
                if (args[i].get() == name_node) {
                    name_result = get_string_value(args[i + 1].get());
                    break;
                }
            }
        }
    }

    if (!name_result.has_value() || name_result.value().empty()) {
        return Err<ParseError, std::unique_ptr<ast::CPMFindPackage>>(
            ParseError("CPMFindPackage requires package name"));
    }

    name = name_result.value();
    auto package = std::make_unique<ast::CPMFindPackage>(loc, name);

    // Parse additional arguments
    for (size_t i = 1; i < args.size(); ++i) {
        auto key_result = get_string_value(args[i].get());
        if (!key_result.has_value()) {
            continue;
        }

        const auto& key = key_result.value();

        if (key == "VERSION" && i + 1 < args.size()) {
            auto value_result = get_string_value(args[i + 1].get());
            if (value_result.has_value()) {
                package->set_version(value_result.value());
                i++;
            }
        } else if (key == "GITHUB_REPOSITORY" && i + 1 < args.size()) {
            auto value_result = get_string_value(args[i + 1].get());
            if (value_result.has_value()) {
                package->set_github_repository(value_result.value());
                i++;
            }
        } else if (key == "GIT_TAG" && i + 1 < args.size()) {
            auto value_result = get_string_value(args[i + 1].get());
            if (value_result.has_value()) {
                package->set_git_tag(value_result.value());
                i++;
            }
        } else if (key == "COMPONENTS") {
            // Collect component names
            i++;
            while (i < args.size()) {
                auto comp_result = get_string_value(args[i].get());
                if (!comp_result.has_value()) {
                    break;
                }
                const auto& comp = comp_result.value();
                // Stop if we hit another keyword
                if (comp == "REQUIRED" || comp == "QUIET" || comp == "OPTIONAL") {
                    i--;
                    break;
                }
                package->add_component(comp);
                i++;
            }
        }
    }

    return Ok<std::unique_ptr<ast::CPMFindPackage>, ParseError>(std::move(package));
}

Result<std::unique_ptr<ast::CPMUsePackageLock>, ParseError>
CPMParser::parse_cpm_use_package_lock(const ast::ASTNodeList& args) {
    if (args.empty()) {
        return Err<ParseError, std::unique_ptr<ast::CPMUsePackageLock>>(
            ParseError("CPMUsePackageLock requires a file path"));
    }

    auto path_result = get_string_value(args[0].get());
    if (!path_result.has_value() || path_result.value().empty()) {
        return Err<ParseError, std::unique_ptr<ast::CPMUsePackageLock>>(
            ParseError("CPMUsePackageLock requires a valid file path"));
    }

    return Ok<std::unique_ptr<ast::CPMUsePackageLock>, ParseError>(
        std::make_unique<ast::CPMUsePackageLock>(args[0]->location(), path_result.value()));
}

Result<std::unique_ptr<ast::CPMDeclarePackage>, ParseError>
CPMParser::parse_cpm_declare_package(const ast::ASTNodeList& args) {
    if (args.empty()) {
        return Err<ParseError, std::unique_ptr<ast::CPMDeclarePackage>>(
            ParseError("CPMDeclarePackage requires arguments"));
    }

    std::string name;
    SourceLocation loc = args[0]->location();

    // Find NAME argument
    auto* name_node = find_argument(args, "NAME");
    if (name_node) {
        for (size_t i = 0; i < args.size() - 1; ++i) {
            if (args[i].get() == name_node) {
                auto name_result = get_string_value(args[i + 1].get());
                if (name_result.has_value()) {
                    name = name_result.value();
                    break;
                }
            }
        }
    }

    if (name.empty()) {
        return Err<ParseError, std::unique_ptr<ast::CPMDeclarePackage>>(
            ParseError("CPMDeclarePackage requires NAME"));
    }

    auto package = std::make_unique<ast::CPMDeclarePackage>(loc, name);

    // Parse remaining arguments
    for (size_t i = 0; i < args.size(); ++i) {
        auto key_result = get_string_value(args[i].get());
        if (!key_result.has_value()) {
            continue;
        }

        const auto& key = key_result.value();

        if (key == "VERSION" && i + 1 < args.size()) {
            auto value_result = get_string_value(args[i + 1].get());
            if (value_result.has_value()) {
                package->set_version(value_result.value());
                i++;
            }
        } else if (key == "GITHUB_REPOSITORY" && i + 1 < args.size()) {
            auto value_result = get_string_value(args[i + 1].get());
            if (value_result.has_value()) {
                package->set_github_repository(value_result.value());
                i++;
            }
        } else if (key == "GIT_REPOSITORY" && i + 1 < args.size()) {
            auto value_result = get_string_value(args[i + 1].get());
            if (value_result.has_value()) {
                package->set_git_repository(value_result.value());
                i++;
            }
        }
    }

    return Ok<std::unique_ptr<ast::CPMDeclarePackage>, ParseError>(std::move(package));
}

Result<ast::CPMVersion, ParseError>
CPMParser::parse_version_string(const std::string& version_str) {
    ast::CPMVersion version;

    // Check for exact version with @
    std::smatch match;
    if (std::regex_match(version_str, match, VERSION_EXACT_REGEX)) {
        version.version = match[1].str();
        version.exact = true;
    }
    // Check for minimum version with >=
    else if (std::regex_match(version_str, match, VERSION_MIN_REGEX)) {
        version.version = match[1].str();
        version.exact = false;
    }
    // Git tag/branch reference or plain version
    else {
        version.version = version_str;
        version.exact = false;

        // If it looks like a git ref, store it as git_tag
        if (version_str.find('/') != std::string::npos ||
            version_str.find('-') != std::string::npos ||
            version_str.length() == 40) { // SHA-1 hash
            version.git_tag = version_str;
        }
    }

    return Result<ast::CPMVersion, ParseError>{version};
}

Result<void, ParseError> CPMParser::parse_options_block(ast::CPMAddPackage& package,
                                                        const ast::ASTNodeList& options) {
    // OPTIONS are passed as CMake cache entries
    // Format: "VAR_NAME VALUE" or "VAR_NAME:TYPE VALUE"

    for (size_t i = 0; i < options.size(); ++i) {
        auto opt_result = get_string_value(options[i].get());
        if (!opt_result.has_value()) {
            continue;
        }

        const auto& opt_str = opt_result.value();

        // Try to parse as "KEY VALUE" or "KEY:TYPE VALUE"
        auto space_pos = opt_str.find(' ');
        if (space_pos != std::string::npos) {
            auto key = opt_str.substr(0, space_pos);
            auto value = opt_str.substr(space_pos + 1);

            // Remove :TYPE suffix if present
            auto colon_pos = key.find(':');
            if (colon_pos != std::string::npos) {
                key = key.substr(0, colon_pos);
            }

            package.add_option(key, value);
        } else if (i + 1 < options.size()) {
            // Try next element as value
            auto val_result = get_string_value(options[i + 1].get());
            if (val_result.has_value()) {
                package.add_option(opt_str, val_result.value());
                i++; // Skip value
            }
        }
    }

    return Result<void, ParseError>{};
}

bool CPMParser::is_github_shorthand(const std::string& str) const {
    return std::regex_match(str, GITHUB_SHORTHAND_REGEX);
}

Result<std::pair<std::string, std::string>, ParseError>
CPMParser::parse_github_shorthand(const std::string& shorthand) {
    std::smatch match;
    if (!std::regex_match(shorthand, match, GITHUB_SHORTHAND_REGEX)) {
        return Err<ParseError, std::pair<std::string, std::string>>(
            ParseError(fmt::format("Invalid CPM shorthand syntax: {}", shorthand)));
    }

    std::string owner = match[1].str();
    std::string repo = match[2].str();
    std::string version = match[3].str(); // May be empty

    std::string repo_spec = fmt::format("{}/{}", owner, repo);
    return Result<std::pair<std::string, std::string>, ParseError>{
        std::make_pair(repo_spec, version)};
}

Result<std::string, ParseError> CPMParser::get_string_value(const ast::ASTNode* node) const {
    if (!node) {
        return Err<ParseError, std::string>(ParseError("Null node"));
    }

    if (auto* str_lit = dynamic_cast<const ast::StringLiteral*>(node)) {
        return Ok<std::string, ParseError>(std::string(str_lit->value()));
    } else if (auto* ident = dynamic_cast<const ast::Identifier*>(node)) {
        return Ok<std::string, ParseError>(std::string(ident->name()));
    }

    return Err<ParseError, std::string>(ParseError("Expected string literal or identifier"));
}

const ast::ASTNode* CPMParser::find_argument(const ast::ASTNodeList& args,
                                             const std::string& name) const {
    for (const auto& arg : args) {
        auto str_result = get_string_value(arg.get());
        if (str_result.has_value() && str_result.value() == name) {
            return arg.get();
        }
    }
    return nullptr;
}

std::vector<const ast::ASTNode*> CPMParser::collect_arguments_after(const ast::ASTNodeList& args,
                                                                    const std::string& key) const {
    std::vector<const ast::ASTNode*> result;
    bool found_key = false;

    for (const auto& arg : args) {
        if (found_key) {
            result.push_back(arg.get());
        } else {
            auto str_result = get_string_value(arg.get());
            if (str_result.has_value() && str_result.value() == key) {
                found_key = true;
            }
        }
    }

    return result;
}

} // namespace finch::parser
