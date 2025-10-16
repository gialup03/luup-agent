# luup-agent: Multi-Agent LLM Library

## Project Overview

**luup-agent** is a cross-platform C library that provides LLM inference with tool calling for application developers. It separates concerns into two layers:

- **Model Layer**: Abstraction over local models (llama.cpp) and remote APIs (OpenAI-compatible)
- **Agent Layer**: Specialized AI agents with system prompts, tool calling, and conversation management

**GitHub Repository**: https://github.com/gialup03/luup-agent

## Core Architecture

### Two-Layer Design

```
Models (Inference Backend)
  ├── Local models via llama.cpp (GGUF format)
  │   └── Auto-detect GPU: Metal, CUDA, ROCm, Vulkan, CPU
  └── Remote models via API (OpenAI-compatible)

Agents (Behavior + Tools)
  ├── References a Model
  ├── System prompt defines role
  ├── Tool calling with JSON schemas
  ├── Conversation history management
  └── Built-in utilities (todo, notes, summarization)
```

### Key Design Decisions

1. **Cross-platform via llama.cpp**: Single codebase supports Mac Metal, CUDA, ROCm, Vulkan
2. **Model sharing**: Multiple agents can share one loaded model (memory efficiency)
3. **Streaming support**: Token-by-token output for responsive UIs
4. **Language bindings**: C API with wrappers for C#, Python, C++
5. **Tool calling**: Standard function calling pattern like OpenAI

## Technical Stack

- **Core**: C++ using llama.cpp
- **Backends**: Metal (Mac), CUDA (NVIDIA), ROCm (AMD), Vulkan (Universal), CPU
- **Models**: GGUF format (universal), Remote APIs
- **Build**: CMake with platform detection
- **Bindings**: C API + language-specific wrappers

## Implementation Phases

### Phase 1: Core C API (Model + Agent Layers)

**Files to Create:**

- `include/luup_agent.h` - Public C API
- `src/core/model.cpp` - Model abstraction
- `src/core/agent.cpp` - Agent implementation
- `src/core/tool_calling.cpp` - Tool execution system
- `src/backends/local_llama.cpp` - llama.cpp integration
- `src/backends/remote_api.cpp` - HTTP API client

**Core API Structure:**

```c
// Model Layer
typedef struct luup_model luup_model;
luup_model* luup_model_create_local(config);
luup_model* luup_model_create_remote(config);
void luup_model_warmup(model);

// Agent Layer
typedef struct luup_agent luup_agent;
luup_agent* luup_agent_create(config);
void luup_agent_register_tool(agent, tool, callback);
void luup_agent_generate_stream(agent, prompt, callback);
```

**Key Features:**

- Platform auto-detection (Metal/CUDA/ROCm/Vulkan/CPU)
- GPU layer offloading (configurable)
- Context window management
- Temperature/max_tokens configuration
- Model pre-warming for fast first token

### Phase 2: C# Bindings for Unity

**Files to Create:**

- `bindings/csharp/LuupAgent.cs` - Main wrapper
- `bindings/csharp/Model.cs` - Model class
- `bindings/csharp/Agent.cs` - Agent class
- `bindings/csharp/Tool.cs` - Tool definition helpers

**Unity Integration:**

- P/Invoke for native calls
- Managed delegates for callbacks
- Thread-safe streaming
- Unity coroutine support
- Asset bundle packaging

**Example Usage:**

```csharp
var model = Model.FromLocal("model.gguf", gpuLayers: -1);
model.Warmup();

var agent = new Agent(model, "You are a dungeon master...");
agent.RegisterTool("damage_player", description, schema, callback);
agent.GenerateStream(prompt, onToken);
```

### Phase 3: Built-in Tools

**Files to Create:**

- `src/builtin_tools/todo_list.cpp` - Task tracking
- `src/builtin_tools/notes.cpp` - Note taking/retrieval
- `src/builtin_tools/summarization.cpp` - Auto context compression
- `src/builtin_tools/memory_bank.cpp` - Long-term semantic memory

**Features:**

- Opt-in activation per agent
- Persistent storage (JSON)
- Semantic search for memory
- Automatic summarization when context fills

### Phase 4: Additional Bindings

**Python:**

- `bindings/python/luup_agent.py` - ctypes wrapper
- pip package distribution

**C++ Header-Only:**

- `bindings/cpp/luup_agent.hpp` - RAII wrappers
- Modern C++ API

**Unreal Engine:**

- `bindings/unreal/LuupAgentPlugin/` - UE5 plugin
- Blueprint support

### Phase 5: Advanced Features

**Multi-Agent Messaging:**

- Agents can call other agents as tools
- Message passing system
- Collaborative reasoning

**MCP Support:**

- Model Context Protocol integration
- External tool servers
- Standard protocol compliance

**Distributed Agents:**

- Network serialization
- Remote agent calling
- Load balancing

## Build System

### CMakeLists.txt Structure

```cmake
project(luup-agent)

# Platform detection
if(APPLE)
    set(GGML_METAL ON)
elseif(WIN32)
    find_package(CUDAToolkit)
    if(CUDAToolkit_FOUND)
        set(GGML_CUDA ON)
    endif()
endif()

# Core library
add_library(luup_agent SHARED
    src/core/model.cpp
    src/core/agent.cpp
    src/core/tool_calling.cpp
    src/backends/local_llama.cpp
    src/backends/remote_api.cpp
)

target_link_libraries(luup_agent PRIVATE llama)
```

### Distribution

**Binary Packages:**

- `luup-agent-mac-arm64.dylib` (Metal)
- `luup-agent-mac-x64.dylib` (CPU)
- `luup-agent-windows-cuda.dll` (NVIDIA)
- `luup-agent-windows-rocm.dll` (AMD)
- `luup-agent-windows-vulkan.dll` (Universal)
- `luup-agent-linux-cuda.so`
- `luup-agent-linux-rocm.so`

**Unity Package:**

- Native binaries for all platforms
- C# wrapper scripts
- Example scenes
- Documentation

**Unreal Plugin:**

- C++ wrapper
- Blueprint nodes
- Example project

## Testing Strategy

### Unit Tests

- Model loading (local + remote)
- Tool registration and execution
- Context management
- Streaming callbacks

### Integration Tests

- Multi-agent scenarios
- Tool calling chains
- Memory limits
- Error recovery

### Platform Tests

- Mac Metal backend
- CUDA backend
- ROCm backend
- Vulkan backend
- CPU fallback

### Game Engine Tests

- Unity integration
- Unreal integration
- Performance benchmarks

## Documentation

### Developer Docs

- API reference (Doxygen)
- Quick start guide
- Tool calling tutorial
- Multi-agent patterns

### Example Projects

- Text adventure (Python)
- Unity RPG with multiple agents
- Unreal dungeon crawler
- Custom C++ game integration

## Performance Targets

- **First token latency**: < 100ms after warmup
- **Throughput**: 20-50 tokens/sec on target hardware
- **Memory overhead**: < 500MB per agent (excluding model)
- **Model sharing**: Support 10+ agents on one model
- **Startup time**: < 5 seconds for model loading

## Deployment

### Package Managers

- Unity Asset Store
- Unreal Marketplace
- pip (Python package)
- Homebrew (Mac)
- vcpkg (Windows/Linux)

### Licensing

- MIT License (permissive)
- Commercial use allowed
- Attribution required

## Milestones

**v0.1 (MVP)** - 2-3 months

- Core C API
- llama.cpp integration
- Basic tool calling
- C# bindings
- Unity example

**v0.2** - +1 month

- Built-in tools (todo, notes)
- Auto summarization
- Python bindings
- Extended examples

**v0.3** - +2 months

- Multi-agent messaging
- MCP support
- Unreal plugin
- Production hardening

**v1.0** - +2 months

- Full documentation
- Asset store packages
- Performance optimization
- Stable API guarantee

## Success Criteria

- Works on Mac (Metal), Windows (CUDA/ROCm), Linux (CUDA/ROCm)
- Multiple agents share one model efficiently
- Streaming works smoothly in Unity/Unreal
- Tool calling is reliable (>95% success rate)
- Easy to integrate (< 100 lines of code for basic usage)
- Active community (GitHub stars, issues, contributions)

## Risks & Mitigations

**Risk**: llama.cpp API changes

- **Mitigation**: Pin to stable version, abstract interface

**Risk**: Platform-specific GPU bugs

- **Mitigation**: Extensive testing, fallback to CPU

**Risk**: Memory management in C bindings

- **Mitigation**: Clear ownership model, RAII wrappers

**Risk**: Tool calling parsing failures

- **Mitigation**: Robust JSON parsing, retry logic, validation

## Next Steps

1. Set up GitHub repository structure
2. Implement core C API (model + agent)
3. Integrate llama.cpp with platform detection
4. Build C# bindings with Unity example
5. Test on Mac Metal and Windows CUDA
6. Document API and create tutorials
7. Release v0.1 for community feedback
