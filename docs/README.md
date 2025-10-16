# luup-agent Documentation

Complete documentation for luup-agent library.

## User Documentation

### Getting Started
- [Quick Start Guide](quickstart.md) - Get up and running in 5 minutes
- [API Reference](api_reference.md) - Complete C API documentation
- [Tool Calling Guide](tool_calling_guide.md) - How to add custom tools

### Language Bindings
- [Python Bindings](../bindings/python/README.md) - Python package documentation

### Examples
- [C Examples](../examples/) - Basic chat and tool calling in C
- [Python Examples](../bindings/python/examples/) - Python usage examples

## Developer Documentation

### Development Guides
- [Development Guide](development/DEVELOPMENT.md) - How to contribute and develop
- [Implementation Plan](development/IMPLEMENTATION_PLAN.md) - Original design and architecture
- [Project Structure](development/PROJECT_STRUCTURE.md) - Codebase organization

### Phase Completion Reports
- [Phase 1: Model Layer](phases/PHASE1_COMPLETE.md) - llama.cpp backend integration
- [Phase 2: Agent Layer](phases/PHASE2_COMPLETE.md) - Tool calling and conversation management
- [Phase 4: Python Bindings](../bindings/python/PHASE4_COMPLETE.md) - Python package implementation

## Project Organization

```
luup-agent/
├── README.md                  # Main project README
├── LICENSE                    # MIT License
├── CONTRIBUTING.md            # How to contribute
│
├── docs/                      # Documentation (you are here)
│   ├── README.md              # This file - documentation index
│   ├── quickstart.md          # Quick start guide
│   ├── api_reference.md       # C API reference
│   ├── tool_calling_guide.md  # Tool calling guide
│   ├── development/           # Developer guides
│   │   ├── DEVELOPMENT.md
│   │   ├── IMPLEMENTATION_PLAN.md
│   │   └── PROJECT_STRUCTURE.md
│   └── phases/                # Phase completion reports
│       ├── PHASE1_COMPLETE.md
│       └── PHASE2_COMPLETE.md
│
├── include/                   # Public C headers
│   └── luup_agent.h
│
├── src/                       # C library source code
│   ├── core/                  # Core agent/model implementation
│   ├── backends/              # Model backends (llama.cpp, APIs)
│   └── builtin_tools/         # Built-in tools
│
├── tests/                     # C unit tests
│   └── unit/
│
├── examples/                  # C example programs
│
└── bindings/                  # Language bindings
    └── python/                # Python package
        ├── README.md          # Python documentation
        ├── PHASE4_COMPLETE.md # Python implementation report
        ├── luup_agent/        # Python package source
        ├── tests/             # Python tests
        └── examples/          # Python examples
```

## Quick Links

### For Users
- 🚀 [Get Started](quickstart.md)
- 📚 [API Reference](api_reference.md)
- 🐍 [Python Bindings](../bindings/python/README.md)
- 💡 [Examples](../examples/)

### For Developers
- 🛠️ [Development Guide](development/DEVELOPMENT.md)
- 📋 [Contributing](../CONTRIBUTING.md)
- 🏗️ [Architecture](development/IMPLEMENTATION_PLAN.md)

### For Project Management
- ✅ [Phase 1 Complete](phases/PHASE1_COMPLETE.md) - Model Layer
- ✅ [Phase 2 Complete](phases/PHASE2_COMPLETE.md) - Agent Layer
- ✅ [Phase 4 Complete](../bindings/python/PHASE4_COMPLETE.md) - Python Bindings

## Version

Current version: **0.1.0-dev**

Status: Active development, v0.1 release candidate

## Support

- **Issues**: https://github.com/gialup03/luup-agent/issues
- **Discussions**: https://github.com/gialup03/luup-agent/discussions
- **License**: MIT

