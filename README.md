# 🐦 finch-buck2

**Transform CMake projects to Buck2 with confidence - A CPM-aware migration tool**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/std/the-standard)

---

## 🚀 What is finch-buck2?

finch-buck2 is a powerful migration tool that automatically converts CMake projects to Buck2, with special support for CPM.cmake package manager. Born from the flight-core team's need to modernize build systems across diverse platforms—from cutting-edge servers to retro gaming consoles like the Sega Dreamcast—finch-buck2 makes build system migration painless and reliable.

## ✨ Features

- ✅ **Smart CMake Analysis** - Understands complex CMake configurations and dependencies
- ✅ **CPM.cmake Support** - First-class support for CPM package manager migrations
- ✅ **Buck2 Best Practices** - Generates clean, maintainable Buck2 BUILD files
- ✅ **Cross-Platform Ready** - Works with projects targeting everything from embedded systems to modern platforms
- ✅ **C++17 Compatible** - Maintains compatibility with a wide range of toolchains and targets
- ✅ **Dependency Resolution** - Intelligently maps CMake dependencies to Buck2 equivalents
- 🔧 **Configuration Preservation** - Maintains your project's build configurations and options
- 🔧 **Incremental Migration** - Supports gradual migration of large projects
- 🔧 **Validation Tools** - Verifies that migrated builds produce equivalent results

*Legend: ✅ Implemented | 🔧 In Development*

## 🎯 Why finch-buck2?

### The Problem
Migrating from CMake to Buck2 is traditionally a manual, error-prone process that can take weeks or months for complex projects. The challenge becomes even greater when dealing with CPM.cmake dependencies or projects targeting specialized platforms.

### The Solution
finch-buck2 automates this migration process, handling the complexity of:
- Parsing CMake syntax and understanding project structure
- Converting CPM package declarations to Buck2 dependencies
- Generating appropriate BUILD files with correct dependency graphs
- Maintaining compatibility across diverse build targets

## 🚀 Quick Start

```bash
# Coming soon! Installation and usage examples will be added as development progresses.
```

## 📦 Installation

*Installation instructions will be provided once the first release is available.*

## 📚 Usage

```bash
# Basic usage example (in development)
finch-buck2 migrate /path/to/cmake/project

# With CPM support
finch-buck2 migrate --enable-cpm /path/to/cmake/project

# Validate migration
finch-buck2 validate /path/to/migrated/project
```

*Detailed usage documentation is coming soon.*

## 🏗️ Project Structure

```
finch-buck2/
├── include/finch/     # Public API headers
├── src/              # Implementation source files
├── test/             # Unit tests and integration tests
├── examples/         # Example CMake→Buck2 migrations
├── docs/             # Documentation and guides
└── tools/            # Utility scripts and tools
```

## 🤝 Contributing

We welcome contributions from the community! Whether you're interested in:
- Adding support for new CMake features
- Improving Buck2 code generation
- Testing with different project configurations
- Writing documentation
- Reporting bugs or suggesting features

Please see our [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to get started.

## 🎮 Platform Support

finch-buck2 is designed to work with projects targeting a wide range of platforms, from modern cloud infrastructure to embedded systems and retro gaming platforms. The C++17 standard ensures compatibility with diverse toolchains while providing modern language features.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

This project was initiated by the flight-core team within the birbparty organization to address the critical need for automated build system migration tools in the C++ ecosystem.

---

*🐦 Ready to migrate? Let finch-buck2 handle the complexity while you focus on building amazing software.*
