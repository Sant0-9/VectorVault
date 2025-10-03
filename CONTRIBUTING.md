# Contributing to VectorVault

Thank you for considering contributing to VectorVault! This document outlines the process and guidelines.

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone git@github.com:YOUR_USERNAME/VectorVault.git`
3. Add upstream remote: `git remote add upstream git@github.com:Sant0-9/VectorVault.git`
4. Create a branch: `git checkout -b feature/your-feature-name`

## Development Setup

### Prerequisites
- CMake 3.22+
- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- Git

### Building
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### Running Tests
```bash
cd build && ctest --output-on-failure
```

## Code Style

### C++ Standards
- Use C++20 features where appropriate
- Follow the existing code style (enforced by `.clang-format`)
- Run `cmake --build build --target format` before committing

### Naming Conventions
- Classes: `PascalCase`
- Functions: `snake_case`
- Variables: `snake_case`
- Constants: `UPPER_CASE`
- Private members: `trailing_underscore_`

### Code Quality
- Enable all warnings
- Run static analysis: `cmake --build build --target tidy`
- Write unit tests for new features
- Maintain test coverage above 60%

## Commit Guidelines

### Commit Messages
Follow conventional commits format:

```
type(scope): brief description

Detailed explanation of changes if needed

- Bullet points for specific changes
- Reference issues with #123
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `style`: Formatting, no code change
- `refactor`: Code restructuring
- `perf`: Performance improvement
- `test`: Adding tests
- `chore`: Maintenance tasks

**Examples:**
```
feat(hnsw): add filtered search support

Implement metadata predicates for filtered nearest neighbor search.
Supports equality, range, and IN operators.

- Add FilterPredicate class
- Extend HNSWIndex::search() with filter parameter
- Add unit tests for filtered search

Closes #42
```

## Pull Request Process

1. **Sync with upstream**
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Ensure quality**
   - All tests pass
   - Code is formatted
   - No compiler warnings
   - Documentation updated

3. **Create PR**
   - Use a descriptive title
   - Fill out the PR template
   - Link related issues
   - Add screenshots/benchmarks if applicable

4. **Review process**
   - Address reviewer feedback
   - Keep commits clean (squash if needed)
   - Maintain CI passing

## Testing

### Writing Tests
- Place unit tests in `tests/`
- Use GoogleTest framework
- Test file naming: `test_<component>.cpp`
- Aim for 80%+ coverage for new code

### Test Structure
```cpp
#include <gtest/gtest.h>
#include "vectorvault/your_header.hpp"

class YourComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }
};

TEST_F(YourComponentTest, DescriptiveTestName) {
    // Arrange
    // Act
    // Assert
}
```

## Benchmarking

When making performance changes:

1. Run benchmarks before changes
2. Run benchmarks after changes
3. Include results in PR description

```bash
./build/vectorvault_bench --mode=query --N=100000 --d=768 --Q=1000
```

## Documentation

- Update README.md for user-facing changes
- Add docstrings for public APIs
- Update CHANGELOG.md
- Include examples for new features

## Bug Reports

Use GitHub Issues with:
- Clear title
- Reproduction steps
- Expected vs actual behavior
- Environment details (OS, compiler, version)
- Stack trace if applicable

## Feature Requests

Open an issue with:
- Use case description
- Proposed API/interface
- Alternative solutions considered
- Willingness to implement

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow
- No harassment or discrimination

## Questions?

- Open a discussion on GitHub
- Check existing issues and docs
- Reach out to maintainers

Thank you for contributing to VectorVault!
