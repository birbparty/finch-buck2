# Pull Request

## 📋 Description

**What does this PR do?**
Provide a brief description of the changes in this pull request.

**Related Issue(s):**

- Fixes #(issue number)
- Relates to #(issue number)

## 🔄 Type of Change

Please mark the relevant option(s):

- [ ] 🐛 Bug fix (non-breaking change which fixes an issue)
- [ ] ✨ New feature (non-breaking change which adds functionality)
- [ ] 💥 Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] 📚 Documentation update
- [ ] 🧪 Test improvements
- [ ] 🔧 Code refactoring (no functional changes)
- [ ] ⚡ Performance improvement
- [ ] 🎨 Code style/formatting changes
- [ ] 🏗️ Build/CI changes
- [ ] 🧹 Chore (maintenance, dependency updates, etc.)

## 🧪 Testing

**How has this been tested?**

- [ ] Unit tests added/updated
- [ ] Integration tests added/updated
- [ ] Manual testing performed
- [ ] Existing tests pass
- [ ] No tests needed for this change

**Test scenarios covered:**

- [ ] Simple CMake projects
- [ ] Complex multi-target projects
- [ ] CPM.cmake dependencies
- [ ] Cross-platform compatibility
- [ ] Edge cases and error handling

**Commands run for testing:**

```bash
# List the commands you used to test your changes
# For example:
# mkdir build && cd build && cmake .. && make
# ./finch-buck2 migrate ../test-project
# buck2 build //...
```

## 📁 Migration Testing (if applicable)

If this PR affects migration functionality, please test with:

- [ ] Header-only libraries
- [ ] Static libraries
- [ ] Shared libraries
- [ ] Executables
- [ ] Projects with CPM dependencies
- [ ] Projects with find_package()
- [ ] Multi-target projects
- [ ] Cross-compilation projects

**Test projects used:**
-

-
-

## 📚 Documentation

- [ ] Documentation updated (if needed)
- [ ] README.md updated (if needed)
- [ ] CONTRIBUTING.md updated (if needed)
- [ ] Code comments added/updated for complex logic
- [ ] API documentation updated (if applicable)

## ✅ Checklist

**Before submitting this PR, please ensure:**

### Code Quality

- [ ] My code follows the project's style guidelines
- [ ] I have performed a self-review of my own code
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix is effective or that my feature works
- [ ] New and existing unit tests pass locally with my changes

### Standards

- [ ] Code is C++17 compatible
- [ ] No memory leaks introduced
- [ ] Error handling is appropriate
- [ ] Thread safety considered (if applicable)

### Git Hygiene

- [ ] My commits follow the conventional commit format
- [ ] I have rebased my branch on the latest main
- [ ] My branch has a clear, descriptive name
- [ ] Commit messages are clear and descriptive

### Integration

- [ ] I have tested the changes on multiple platforms (if applicable)
- [ ] The changes work with different CMake versions
- [ ] The changes work with different Buck2 versions
- [ ] Backward compatibility maintained (if applicable)

## 🔍 Code Review Focus Areas

**Please pay special attention to:**

- [ ] Algorithm correctness
- [ ] Error handling
- [ ] Performance implications
- [ ] Memory management
- [ ] Thread safety
- [ ] API design
- [ ] Security considerations

## 📊 Performance Impact

- [ ] No performance impact
- [ ] Performance improved
- [ ] Performance degraded (justified by functionality gains)
- [ ] Performance impact unknown/needs testing

**If there's a performance impact, please describe:**

## 🔀 Migration Compatibility

For changes affecting migration logic:

- [ ] Maintains compatibility with existing CMake patterns
- [ ] Handles edge cases gracefully
- [ ] Provides meaningful error messages
- [ ] Works with both simple and complex projects

## 📸 Screenshots/Examples

**Before/After comparisons (if applicable):**

**Example usage:**

```bash
# Show example commands or output demonstrating the change
```

## 🤔 Questions for Reviewers

**Specific questions or areas where you'd like feedback:**
-

-
-

## 🚀 Deployment Notes

**Any special considerations for deployment:**

- [ ] No special deployment considerations
- [ ] Database changes required
- [ ] Configuration changes required
- [ ] Documentation updates required
- [ ] User migration guide needed

---

**Additional Notes:**
Add any other notes for reviewers here.
