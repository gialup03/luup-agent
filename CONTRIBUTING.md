# Contributing to luup-agent

Thank you for your interest in contributing to luup-agent!

## How to Contribute

### Reporting Bugs

- Check existing issues first
- Use the bug report template
- Include:
  - OS and version
  - GPU backend being used
  - Minimal reproduction code
  - Error messages and logs

### Suggesting Features

- Open an issue with the feature request template
- Describe the use case
- Explain how it fits with the project goals

### Pull Requests

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Update documentation
7. Commit with clear messages
8. Push to your fork
9. Open a Pull Request

## Development Setup

```bash
# Clone with submodules
git clone --recursive https://github.com/gialup03/luup-agent.git
cd luup-agent

# Build
mkdir build && cd build
cmake -DLUUP_BUILD_TESTS=ON ..
cmake --build .

# Run tests
ctest
```

## Code Style

- C++17 standard
- Follow existing code style
- Use meaningful variable names
- Add comments for complex logic
- Document public APIs with Doxygen

## Testing

- Write unit tests for new features
- Ensure existing tests pass
- Test on multiple platforms if possible
- Include integration tests for major features

## Documentation

- Update API docs in header files
- Update markdown docs in `docs/`
- Add examples for new features
- Keep README.md current

## Commit Messages

- Use present tense ("Add feature" not "Added feature")
- First line: brief summary (50 chars or less)
- Blank line, then detailed description if needed
- Reference issues and PRs

Example:
```
Add streaming support for remote models

- Implement SSE parsing for OpenAI API
- Add streaming callback support
- Update tests and documentation

Fixes #123
```

## Areas Needing Help

- Platform testing (especially Windows ROCm, Linux)
- Example projects (games, apps)
- Language bindings (Python, C++, Unreal)
- Documentation improvements
- Performance optimization
- Bug fixes

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

## Questions?

Open a discussion on GitHub or reach out in our community channels.

