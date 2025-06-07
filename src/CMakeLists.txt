# src/CMakeLists.txt Build configuration for buck2-cpp-cpm-core library

# Add source files
target_sources(
  buck2-cpp-cpm-core
  PRIVATE dummy.cpp # Temporary file until real implementation is added
          # Core logging system
          core/logging.cpp
          core/logging_helpers.cpp
          core/otel_integration.cpp
          # Parser lexer system
          parser/lexer/source_buffer.cpp
          parser/lexer/token.cpp
          parser/lexer/lexer.cpp
          # Parser system
          parser/parser.cpp
          parser/parser_tokens.cpp
          parser/parser_arguments.cpp
          parser/parser_control_flow.cpp
          parser/parser_errors.cpp
          parser/cpm_parser.cpp
          parser/ast/clone_impl.cpp
          # Analyzer system
          analyzer/evaluation_context.cpp
          analyzer/cmake_evaluator.cpp
          # CLI system
          cli/application.cpp
          cli/migration_pipeline.cpp
          cli/progress_reporter.cpp
          # Generator system
          generator/generator.cpp
          generator/target_mapper.cpp
          generator/rule_templates.cpp
          generator/starlark_writer.cpp
          # Testing utilities
          testing/test_project.cpp)

# Link dependencies
target_link_libraries(
  buck2-cpp-cpm-core PUBLIC fmt::fmt spdlog::spdlog nlohmann_json::nlohmann_json
                    CLI11::CLI11)

# Set compile features
target_compile_features(buck2-cpp-cpm-core PUBLIC cxx_std_20)

# Platform-specific settings
if(WIN32)
  target_compile_definitions(buck2-cpp-cpm-core PRIVATE BUCK2_CPM_PLATFORM_WINDOWS)
elseif(APPLE)
  target_compile_definitions(buck2-cpp-cpm-core PRIVATE BUCK2_CPM_PLATFORM_MACOS)
elseif(UNIX)
  target_compile_definitions(buck2-cpp-cpm-core PRIVATE BUCK2_CPM_PLATFORM_LINUX)
endif()

# Export compile commands for this target
set_target_properties(buck2-cpp-cpm-core PROPERTIES EXPORT_COMPILE_COMMANDS ON)
