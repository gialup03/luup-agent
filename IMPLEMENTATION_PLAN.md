# luup-agent: Multi-Agent LLM Library - Implementation Plan

## Project Overview

**luup-agent** is a cross-platform C library that provides LLM inference with tool calling for application developers. It separates concerns into two layers:

- **Model Layer**: Abstraction over local models (llama.cpp) and remote APIs (OpenAI-compatible)
- **Agent Layer**: Specialized AI agents with system prompts, tool calling, and conversation management

**GitHub Repository**: https://github.com/gialup03/luup-agent

## Implementation Details

### Dependency Management

**Core Dependencies:**

- **llama.cpp**: Git submodule + CMake FetchContent fallback
  - Pin to stable tag (e.g., `b1000` or latest stable release)
  - Allow system installation via `find_package(llama CONFIG)`
  - Auto-detect backend: Metal, CUDA, ROCm, Vulkan, CPU

- **nlohmann/json**: Header-only JSON library (v3.11.3)
  - FetchContent from GitHub or single-header include
  - Used for tool schemas and API communication

- **cpp-httplib**: Header-only HTTP client (v0.14.0)
  - Used for remote API support (OpenAI-compatible endpoints)
  - Single-header include, no external dependencies

- **Catch2**: Testing framework (v3.x)
  - FetchContent for dependency management
  - Modern C++ testing with BDD support

### Project Structure

```
luup-agent/
├── CMakeLists.txt
├── README.md
├── LICENSE (MIT)
├── .gitignore
├── .gitmodules
├── include/
│   └── luup_agent.h              # Public C API
├── src/
│   ├── core/
│   │   ├── model.cpp             # Model abstraction
│   │   ├── agent.cpp             # Agent implementation
│   │   ├── tool_calling.cpp      # Tool execution system
│   │   ├── context_manager.cpp   # Conversation history
│   │   └── error_handling.cpp    # Error management
│   ├── backends/
│   │   ├── local_llama.cpp       # llama.cpp integration
│   │   └── remote_api.cpp        # HTTP API client
│   └── builtin_tools/
│       ├── todo_list.cpp
│       ├── notes.cpp
│       └── summarization.cpp
├── tests/
│   ├── unit/
│   │   ├── test_model.cpp
│   │   ├── test_agent.cpp
│   │   └── test_tools.cpp
│   └── integration/
│       ├── test_local_inference.cpp
│       └── test_remote_api.cpp
├── bindings/
│   ├── python/                   # PRIORITY
│   │   ├── luup_agent/
│   │   │   ├── __init__.py
│   │   │   ├── model.py
│   │   │   ├── agent.py
│   │   │   └── tools.py
│   │   ├── setup.py
│   │   └── pyproject.toml
│   ├── csharp/
│   │   ├── LuupAgent.cs
│   │   ├── Model.cs
│   │   ├── Agent.cs
│   │   └── Tool.cs
│   └── cpp/
│       └── luup_agent.hpp
├── examples/
│   ├── basic_chat.cpp
│   ├── tool_calling.cpp
│   ├── python/
│   │   ├── basic_chat.py
│   │   ├── tool_calling.py
│   │   └── jupyter_demo.ipynb
│   └── unity_integration/
├── external/
│   └── llama.cpp/                # Git submodule
└── docs/
    ├── api_reference.md
    ├── quickstart.md
    └── tool_calling_guide.md
```

### Error Handling Pattern

**C API Error Model:**

```c
// Error codes
typedef enum {
    LUUP_SUCCESS = 0,
    LUUP_ERROR_INVALID_PARAM = -1,
    LUUP_ERROR_OUT_OF_MEMORY = -2,
    LUUP_ERROR_MODEL_NOT_FOUND = -3,
    LUUP_ERROR_INFERENCE_FAILED = -4,
    LUUP_ERROR_TOOL_NOT_FOUND = -5,
    LUUP_ERROR_JSON_PARSE_FAILED = -6,
    LUUP_ERROR_HTTP_FAILED = -7,
    LUUP_ERROR_BACKEND_INIT_FAILED = -8
} luup_error_t;

// Get last error message (thread-local)
const char* luup_get_last_error();

// Set error callback for diagnostics
typedef void (*luup_error_callback_t)(luup_error_t code, const char* msg, void* user_data);
void luup_set_error_callback(luup_error_callback_t callback, void* user_data);
```

### Enhanced C API Specification

Complete API with memory management, error handling, and streaming support:

```c
// Model Layer API
typedef struct luup_model luup_model;
typedef struct {
    const char* path;
    int gpu_layers;                // -1 for auto, 0 for CPU only
    int context_size;
    int threads;
    const char* api_key;           // For remote models
    const char* api_base_url;
} luup_model_config;

luup_model* luup_model_create_local(const luup_model_config* config);
luup_model* luup_model_create_remote(const luup_model_config* config);
luup_error_t luup_model_warmup(luup_model* model);
void luup_model_destroy(luup_model* model);

// Agent Layer API
typedef struct luup_agent luup_agent;
typedef struct {
    luup_model* model;
    const char* system_prompt;
    float temperature;
    int max_tokens;
    bool enable_tool_calling;
    bool enable_history_management;
} luup_agent_config;

luup_agent* luup_agent_create(const luup_agent_config* config);

// Tool registration
typedef char* (*luup_tool_callback_t)(const char* params_json, void* user_data);
luup_error_t luup_agent_register_tool(
    luup_agent* agent,
    const luup_tool* tool,
    luup_tool_callback_t callback,
    void* user_data
);

// Generation with streaming
typedef void (*luup_stream_callback_t)(const char* token, void* user_data);
luup_error_t luup_agent_generate_stream(
    luup_agent* agent,
    const char* user_message,
    luup_stream_callback_t callback,
    void* user_data
);

char* luup_agent_generate(luup_agent* agent, const char* user_message);
void luup_agent_destroy(luup_agent* agent);
void luup_free_string(char* str);
```

### Build System

CMake configuration with platform detection and dependency management:

- Auto-detect GPU backends (Metal, CUDA, ROCm, Vulkan)
- Use system-installed llama.cpp if available, otherwise build from submodule
- FetchContent for header-only dependencies
- Build shared library with version info
- Optional tests, examples, and bindings

## Implementation Phases

### Phase 0: Repository Setup (Week 1) ✅ COMPLETE

**Initial files:**

- README.md with project overview and build instructions
- LICENSE (MIT)
- .gitignore for C++/CMake
- CMakeLists.txt root configuration
- include/luup_agent.h with complete API and documentation
- Empty implementation stubs in src/
- Simple example in examples/basic_chat.cpp

### Phase 1: Model Layer (Weeks 2-4)

**Focus:** Local model loading and basic inference

- Implement error handling system
- Integrate llama.cpp with platform auto-detection
- Model loading from GGUF files
- GPU backend selection (Metal/CUDA/ROCm/Vulkan/CPU)
- Basic inference without tools
- Unit tests for model operations
- Simple CLI example

**Key files:**

- src/core/error_handling.cpp
- src/backends/local_llama.cpp
- tests/unit/test_model.cpp

### Phase 2: Agent Layer (Weeks 5-8)

**Focus:** Tool calling and conversation management

- Agent creation with system prompts
- Conversation history management
- Tool calling with JSON schema validation
- Streaming token generation
- Remote API support (OpenAI-compatible)
- Integration tests with real models
- Tool calling examples

**Key files:**

- src/core/agent.cpp
- src/core/tool_calling.cpp
- src/core/context_manager.cpp
- src/backends/remote_api.cpp
- tests/integration/test_tool_calling.cpp

### Phase 3: Built-in Tools (Weeks 9-10)

**Focus:** Pre-built agent capabilities

- Todo list tool with persistence
- Notes tool with retrieval
- Auto-summarization when context fills
- Semantic memory (optional)
- Examples using built-in tools

**Key files:**

- src/builtin_tools/todo_list.cpp
- src/builtin_tools/notes.cpp
- src/builtin_tools/summarization.cpp

### Phase 4: Python Bindings (Weeks 11-12) 🎯 PRIORITY

**Focus:** Pythonic ML/AI workflow integration

This is the priority binding for v0.1 to serve the ML/AI community.

**Tasks:**

- [ ] Create Python package structure
- [ ] ctypes/cffi wrappers for C API
- [ ] Pythonic Model class with context managers
- [ ] Pythonic Agent class with decorator support
- [ ] Async/await support for streaming generation
- [ ] Type hints (PEP 484) and mypy compatibility
- [ ] Exception-based error handling (not error codes)
- [ ] Integration with Python logging module
- [ ] Comprehensive docstrings (Google/NumPy style)
- [ ] pytest test suite
- [ ] setup.py and pyproject.toml for pip
- [ ] Jupyter notebook examples
- [ ] Sphinx documentation

**Python API Design Goals:**

```python
# Pythonic interface, not just C wrapper
from luup_agent import Model, Agent

# Clean initialization with defaults
model = Model.from_local("model.gguf", gpu_layers=-1)
agent = Agent(model, system_prompt="You are helpful")

# Sync generation
response = agent.generate("Hello")

# Async streaming (Python-native)
async for token in agent.generate_async("Tell me a story"):
    print(token, end='', flush=True)

# Context managers for resource management
with Model.from_local("model.gguf") as model:
    with Agent(model) as agent:
        response = agent.generate("Hello")

# Tool registration with decorators
@agent.tool(description="Get current weather")
def get_weather(city: str, units: str = "celsius") -> dict:
    """Get weather for a city."""
    return {"temperature": 72, "condition": "sunny"}

# Conversation management
with agent.conversation() as conv:
    conv.add_message("user", "Hello")
    response = conv.generate()
    conv.add_message("assistant", response)

# Exception handling (Pythonic)
try:
    agent = Agent(model)
    response = agent.generate("Hello")
except ModelNotFoundError as e:
    print(f"Model error: {e}")
except InferenceError as e:
    print(f"Inference failed: {e}")
```

**Key files:**

- bindings/python/luup_agent/__init__.py
- bindings/python/luup_agent/model.py
- bindings/python/luup_agent/agent.py
- bindings/python/luup_agent/tools.py
- bindings/python/luup_agent/exceptions.py
- bindings/python/setup.py
- bindings/python/pyproject.toml
- examples/python/basic_chat.py
- examples/python/tool_calling.py
- examples/python/async_streaming.py
- examples/python/jupyter_demo.ipynb
- bindings/python/tests/test_model.py
- bindings/python/tests/test_agent.py

### Phase 5: Additional Bindings (Weeks 13-14)

**Focus:** C# for game engines and C++ wrapper

- C# P/Invoke wrappers for Unity
- Managed classes (Model, Agent, Tool)
- Unity example scene
- C++ RAII wrapper (header-only)
- Unreal Engine plugin (optional)
- Package distribution setup

**Key files:**

- bindings/csharp/LuupAgent.cs
- bindings/csharp/Model.cs
- bindings/csharp/Agent.cs
- bindings/cpp/luup_agent.hpp
- examples/unity_integration/

## Testing Strategy

**Unit Tests (Catch2 for C++, pytest for Python):**

- Model creation and destruction
- Tool registration and execution
- Context management
- Error handling
- Python binding correctness

**Integration Tests:**

- End-to-end inference with small models
- Multi-turn conversations
- Tool calling chains
- Memory limit handling
- Python async/await patterns

**Platform Tests (CI/CD):**

- GitHub Actions matrix: macOS, Ubuntu, Windows
- Test Metal, CUDA, CPU backends
- Automated testing on each commit
- Python 3.8, 3.9, 3.10, 3.11, 3.12 compatibility

## Performance Targets

- First token latency: < 100ms after warmup
- Throughput: 20-50 tokens/sec on target hardware
- Memory overhead: < 500MB per agent (excluding model)
- Model sharing: Support 10+ agents on one model
- Startup time: < 5 seconds for model loading
- Python overhead: < 5% compared to C API

## Documentation

**Developer docs:**

- API reference (Doxygen for C, Sphinx for Python)
- Quick start guide (5-minute setup)
- Tool calling tutorial
- Multi-agent patterns
- Python-specific guides

**Example projects:**

- Basic CLI chat (C and Python)
- Tool calling demo (C and Python)
- Jupyter notebook tutorials
- Unity RPG integration
- Python ML pipeline integration

## Distribution

**Binary packages:**

- luup-agent-mac-metal.dylib
- luup-agent-windows-cuda.dll
- luup-agent-linux.so

**Package managers:**

- **PyPI (pip)** - Priority for v0.1
- Unity Asset Store (v0.2)
- NuGet (C#, v0.2)
- vcpkg (C++, future)
- Homebrew (macOS, future)

## Milestones

**v0.1 (MVP)** - 3 months

- Core C API with model and agent layers
- Local inference via llama.cpp
- Basic tool calling
- **Python bindings with pip package** 🎯
- Jupyter notebook examples
- pytest test suite

**v0.2** - +1 month

- Built-in tools
- Remote API support
- C# bindings for Unity
- Unity example project
- Extended documentation

**v0.3** - +2 months

- Multi-agent messaging
- Unreal Engine plugin
- Performance optimization
- Advanced Python features

**v1.0** - +2 months

- Stable API
- Production ready
- Full documentation
- Community packages

## Success Criteria

- Works on Mac (Metal), Windows (CUDA), Linux (CUDA/CPU)
- Multiple agents efficiently share one model
- Streaming works smoothly in Python async/await
- Streaming works smoothly in game engines
- Tool calling >95% reliability
- Python integration requires <20 lines of code
- C integration requires <100 lines of code
- Active Python ML/AI community engagement
- Active Unity/game dev community engagement

## Risks & Mitigations

**llama.cpp API changes:** Pin to stable version, abstract interface

**Platform-specific bugs:** Extensive testing, CPU fallback

**Memory management:** Clear ownership model, RAII wrappers, Python GC integration

**Tool calling failures:** Robust JSON parsing, retry logic

**Python GIL issues:** Release GIL during inference, async support

**Python packaging complexity:** Test on multiple platforms, use cibuildwheel

## Implementation Todos

### Phase 0 ✅
- [x] Create GitHub repository with initial structure
- [x] Write complete luup_agent.h C API header
- [x] Add llama.cpp submodule reference
- [x] Configure CMake with FetchContent

### Phase 1 (In Progress)
- [ ] Implement error handling system
- [ ] Implement local model loading via llama.cpp
- [ ] Write unit tests for model layer

### Phase 2
- [ ] Implement agent layer with conversation management
- [ ] Implement tool calling system
- [ ] Add streaming token generation
- [ ] Implement remote API support

### Phase 3
- [ ] Implement built-in tools (todo, notes, summarization)

### Phase 4 - Python Bindings 🎯
- [ ] Create Python package structure
- [ ] Implement ctypes wrappers
- [ ] Create Pythonic Model class
- [ ] Create Pythonic Agent class
- [ ] Add async/await support
- [ ] Write pytest test suite
- [ ] Create Jupyter examples
- [ ] Package for PyPI

### Phase 5
- [ ] Create C# P/Invoke bindings
- [ ] Build Unity example
- [ ] Create C++ header-only wrapper

