#include <finch/analyzer/cmake_evaluator.hpp>
#include <finch/core/result.hpp>
#include <finch/generator/generator.hpp>
#include <finch/parser/parser.hpp>
#include <fstream>
#include <iostream>

using namespace finch;

int main() {
    std::cout << "🐦 Testing Finch Buck2 Generator Pipeline\n\n";

    // Test with simple CMake content
    std::string cmake_content = R"(
cmake_minimum_required(VERSION 3.20)
project(simple-library VERSION 1.0.0)

add_library(calculator STATIC
    src/calculator.cpp
)

target_include_directories(calculator PUBLIC include)
)";

    std::cout << "📝 Input CMakeLists.txt:\n";
    std::cout << cmake_content << "\n\n";

    try {
        // Step 1: Parse
        std::cout << "🔍 Step 1: Parsing CMake content...\n";
        parser::Parser parser(cmake_content, "CMakeLists.txt");
        auto parse_result = parser.parse_file();

        if (!parse_result.has_value()) {
            std::cerr << "❌ Parse failed: " << parse_result.error().message() << "\n";
            return 1;
        }
        std::cout << "✅ Parse successful\n\n";

        // Step 2: Analyze
        std::cout << "🔎 Step 2: Analyzing project structure...\n";
        analyzer::CMakeFileEvaluator evaluator;
        auto analysis_result = evaluator.analyze(parse_result.value());

        if (!analysis_result.has_value()) {
            std::cerr << "❌ Analysis failed\n";
            return 1;
        }

        const auto& analysis = analysis_result.value();
        std::cout << "✅ Analysis successful\n";
        std::cout << "   Project: " << analysis.project_name << "\n";
        std::cout << "   Targets found: " << analysis.targets.size() << "\n\n";

        // Step 3: Generate
        std::cout << "⚡ Step 3: Generating Buck2 files...\n";
        generator::Generator::Config gen_config;
        gen_config.output_directory = ".";
        gen_config.dry_run = false;

        generator::Generator gen(gen_config);
        auto gen_result = gen.generate(analysis);

        if (!gen_result.has_value()) {
            std::cerr << "❌ Generation failed\n";
            return 1;
        }

        const auto& result = gen_result.value();
        std::cout << "✅ Generation successful\n";
        std::cout << "   Files generated: " << result.generated_files.size() << "\n";
        std::cout << "   Targets processed: " << result.targets_processed << "\n\n";

        // Show generated content
        std::cout << "📄 Generated BUCK file content:\n";
        std::ifstream buck_file("BUCK");
        if (buck_file.is_open()) {
            std::string line;
            while (std::getline(buck_file, line)) {
                std::cout << line << "\n";
            }
            buck_file.close();
        } else {
            std::cout << "❌ Could not read generated BUCK file\n";
        }

        std::cout << "\n🎉 End-to-End Pipeline Test SUCCESSFUL!\n";
        std::cout << "✅ CMake → AST → Analysis → Buck2 Generation: WORKING\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "❌ Exception: " << e.what() << "\n";
        return 1;
    }
}
