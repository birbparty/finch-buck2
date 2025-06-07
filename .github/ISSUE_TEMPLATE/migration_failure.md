---
name: Migration Failure
about: Report a CMake to Buck2 migration that failed or produced incorrect results
title: '[MIGRATION] '
labels: ['migration-failure']
assignees: ''
---

## 🔄 Migration Failure Report

### 📁 Project Information

**Project Type:**
- [ ] Single executable
- [ ] Static library
- [ ] Shared library
- [ ] Header-only library
- [ ] Multi-target project
- [ ] Complex project with subdirectories

**Project Size:**
- [ ] Small (< 10 source files)
- [ ] Medium (10-100 source files)
- [ ] Large (100+ source files)

### 📦 Dependencies

**Package Manager Usage:**
- [ ] None
- [ ] CPM.cmake
- [ ] Conan
- [ ] vcpkg
- [ ] find_package() only
- [ ] Git submodules
- [ ] Other: ___________

**Key Dependencies:**
List the main libraries your project depends on:
-
-
-

### 🔧 CMake Configuration

**CMake Version:** [e.g. 3.20.0]

**Please share your CMakeLists.txt** (or a simplified version that reproduces the issue):

<details>
<summary>CMakeLists.txt</summary>

```cmake
# Paste your CMakeLists.txt here
# You can simplify it to the essential parts that cause the migration to fail
```

</details>

**Additional CMake files** (if relevant):
- [ ] Custom .cmake modules
- [ ] Config files
- [ ] Toolchain files

### 🎯 Migration Attempt

**finch-buck2 command used:**
```bash
# The exact command you ran
finch-buck2 migrate /path/to/project
```

**Migration output/error:**
```
# Paste the complete output from finch-buck2 here
```

### 💥 What Went Wrong

**Migration result:**
- [ ] Complete failure (no Buck2 files generated)
- [ ] Partial success (some files missing or incorrect)
- [ ] Generated files but build fails
- [ ] Build succeeds but incorrect behavior
- [ ] Performance issues

**Specific issues observed:**
- [ ] Missing dependencies in Buck2 BUILD files
- [ ] Incorrect target names
- [ ] Wrong source file paths
- [ ] Missing compiler flags
- [ ] CPM dependencies not handled
- [ ] Custom CMake functions not converted
- [ ] Include paths incorrect
- [ ] Other: ___________

### 🎯 Expected Buck2 Output

**If you know what the Buck2 BUILD file should look like, please share:**

```python
# Your expected Buck2 BUILD file content
```

### 🔍 Manual Workaround

**Have you found a manual workaround?**
- [ ] Yes, and it was simple
- [ ] Yes, but it required significant changes
- [ ] No, still stuck
- [ ] Haven't tried manual migration

If yes, please describe what you had to change:

### 🌍 Platform Details

**Target Platform(s):**
- [ ] Linux (x86_64)
- [ ] Linux (ARM)
- [ ] macOS (Intel)
- [ ] macOS (Apple Silicon)
- [ ] Windows (x64)
- [ ] Windows (ARM)
- [ ] Embedded systems
- [ ] Cross-compilation
- [ ] Dreamcast 🎮
- [ ] Other: ___________

**Compiler:**
- [ ] GCC (version: _____)
- [ ] Clang (version: _____)
- [ ] MSVC (version: _____)
- [ ] Cross-compiler (which: _____)

### 📋 Environment

**finch-buck2 version:** [e.g. v0.1.0 or commit hash]
**Buck2 version:** [e.g. 2023.01.15.00]
**Operating System:** [e.g. Ubuntu 20.04, macOS 12.0, Windows 11]

### 🔗 Related Information

**Similar issues:**
- Link to any related issues or discussions
- Reference documentation that might be relevant

**Project repository** (if public):
- Link to your project repository if you can share it

### ✅ Additional Context

**Special requirements or constraints:**
- Any special build requirements
- Custom toolchains or build environments
- Specific Buck2 configuration needs
- Corporate/enterprise requirements

**Timeline:**
- [ ] This is blocking me immediately
- [ ] This is blocking my team
- [ ] This is planned for future migration
- [ ] This is exploratory/research

**Willingness to help debug:**
- [ ] I can provide more details if needed
- [ ] I can test potential fixes
- [ ] I can share the full project privately
- [ ] I can contribute to the fix if guided
