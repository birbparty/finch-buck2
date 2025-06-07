# Contributing to finch-buck2 🐦

Thank you for your interest in contributing to finch-buck2! This project aims to make CMake to Buck2 migration seamless and accessible to everyone, and we welcome contributions from developers of all backgrounds and experience levels.

## 🌟 Ways to Contribute

We appreciate all forms of contribution:

- 🐛 **Bug Reports** - Help us identify and fix issues
- 💡 **Feature Requests** - Suggest new capabilities or improvements
- 📝 **Documentation** - Improve guides, examples, and API docs
- 🧪 **Testing** - Test migrations with different project configurations
- 💻 **Code** - Implement new features or fix bugs
- 🎯 **Migration Examples** - Share real-world migration cases
- 🗣️ **Community** - Help others in discussions and issues

## 🚀 Getting Started

### Prerequisites

- **C++17 compatible compiler** (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake 3.15+** (for building the project itself)
- **Buck2** (for testing migrations)
- **Git** for version control

### Development Setup

1. **Fork and Clone**

   ```bash
   git clone https://github.com/YOUR_USERNAME/finch-buck2.git
   cd finch-buck2
   ```

2. **Build the Project**

   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make -j$(nproc)
   ```

3. **Run Tests**

   ```bash
   ctest --verbose
   ```

### Project Structure

```
finch-buck2/
├── include/finch/     # Public API headers
├── src/              # Implementation (.cpp files)
├── test/             # Unit and integration tests
├── examples/         # Example migrations and test cases
├── docs/             # Documentation and guides
├── tools/            # Utility scripts
└── .github/          # GitHub templates and workflows
```

## 📋 Development Guidelines

### Code Style

We follow modern C++17 best practices:

- **Formatting**: Use `clang-format` with the project's `.clang-format` config
- **Naming**:
  - Classes: `PascalCase` (e.g., `MigrationEngine`)
  - Functions/variables: `snake_case` (e.g., `parse_cmake_file`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_DEPTH`)
- **Headers**: Use `#pragma once` for header guards
- **Includes**: Group system headers, then third-party, then project headers
- **Documentation**: Use Doxygen-style comments for public APIs

### Code Quality

- Write unit tests for new functionality
- Ensure all tests pass before submitting
- Keep functions focused and single-purpose
- Use RAII and smart pointers appropriately
- Handle errors gracefully with clear error messages

### Commit Messages

Use conventional commit format:

```
type(scope): brief description

Detailed explanation if needed

Fixes #123
```

**Types:**

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `test`: Adding or modifying tests
- `refactor`: Code refactoring
- `perf`: Performance improvements
- `chore`: Maintenance tasks

**Examples:**

```
feat(parser): add support for CMake target_link_libraries

fix(migration): handle CPM dependencies with version ranges

docs(readme): update installation instructions
```

## 🔄 Pull Request Process

### Before Submitting

1. **Create a feature branch** from `main`
2. **Write tests** for your changes
3. **Update documentation** if needed
4. **Run the full test suite** and ensure it passes
5. **Format your code** with `clang-format`

### PR Checklist

- [ ] Tests added/updated and passing
- [ ] Documentation updated if needed
- [ ] Code follows project style guidelines
- [ ] Commit messages follow conventional format
- [ ] PR description explains the change and motivation
- [ ] Related issues are referenced

### Review Process

1. **Automated checks** must pass (CI, tests, formatting)
2. **Code review** by maintainers
3. **Integration testing** with various CMake projects
4. **Merge** once approved

## 🐛 Reporting Issues

### Bug Reports

When reporting bugs, please include:

- **finch-buck2 version** you're using
- **Operating system** and version
- **CMake version** of the source project
- **Buck2 version** for the target
- **Minimal reproduction case** (CMakeLists.txt snippet)
- **Expected vs actual behavior**
- **Error messages** or logs

### Feature Requests

For feature requests, please describe:

- **Use case**: What problem does this solve?
- **Proposed solution**: How should it work?
- **Alternatives considered**: Other approaches you've thought about
- **Examples**: Concrete examples of the feature in use

### Migration Failures

If finch-buck2 fails to migrate your project:

- **Share the CMakeLists.txt** (or simplified version)
- **Describe the build setup** (dependencies, special configurations)
- **Include error output** from finch-buck2
- **Expected Buck2 output** if you know what it should be

## 🤝 Code of Conduct

We are committed to providing a welcoming and inclusive environment for all contributors. Please be respectful and constructive in all interactions.

### Our Standards

- **Be respectful** of differing viewpoints and experiences
- **Be collaborative** and help others learn and grow
- **Be inclusive** and welcoming to newcomers
- **Focus on what's best** for the community and project

### Unacceptable Behavior

- Harassment, discrimination, or inappropriate language
- Personal attacks or trolling
- Publishing others' private information
- Any conduct that would be inappropriate in a professional setting

## ❓ Getting Help

- **GitHub Discussions** - For general questions and ideas
- **Issues** - For bug reports and feature requests
- **Documentation** - Check the `docs/` directory
- **Examples** - Look in `examples/` for usage patterns

## 🏆 Recognition

Contributors are recognized in our [README.md](README.md) and release notes. We appreciate everyone who helps make finch-buck2 better!

---

Thank you for contributing to finch-buck2! Together, we're making C++ build system migration easier for everyone. 🐦✨
