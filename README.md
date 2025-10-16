# luup-agent

A cross-platform C library for LLM inference with multi-agent support and tool calling.

## Features

- **Two-Layer Architecture**: Separate model inference from agent behavior
- **Cross-Platform GPU Support**: Metal (macOS), CUDA, ROCm, Vulkan, CPU fallback
- **Tool Calling**: JSON-schema based function calling like OpenAI
- **Model Sharing**: Multiple agents can efficiently share one loaded model
- **Streaming Support**: Token-by-token generation for responsive UIs
- **Multiple Backends**: Local models (llama.cpp/GGUF) and remote APIs (OpenAI-compatible)

## Quick Start

### Building from Source

**Prerequisites:**
- CMake 3.18+
- C++17 compatible compiler
- Git with submodules support

**Clone and Build:**

```bash
git clone --recursive https://github.com/gialup03/luup-agent.git
cd luup-agent
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Basic Usage (C API)

```c
#include <luup_agent.h>

// Create and warmup model
luup_model_config model_config = {
    .path = "models/qwen-0.5b.gguf",
    .gpu_layers = -1,  // Auto-detect and use all available GPU
    .context_size = 2048,
    .threads = 4
};

luup_model* model = luup_model_create_local(&model_config);
luup_model_warmup(model);

// Create agent
luup_agent_config agent_config = {
    .model = model,
    .system_prompt = "You are a helpful assistant.",
    .temperature = 0.7,
    .max_tokens = 512,
    .enable_tool_calling = true
};

luup_agent* agent = luup_agent_create(&agent_config);

// Generate response
char* response = luup_agent_generate(agent, "Hello, how are you?");
printf("%s\n", response);
luup_free_string(response);

// Cleanup
luup_agent_destroy(agent);
luup_model_destroy(model);
```

### Tool Calling Example

```c
// Define a tool callback
char* get_weather_callback(const char* params_json, void* user_data) {
    // Parse params_json to get city
    // ... call weather API ...
    return strdup("{\"temperature\": 72, \"condition\": \"sunny\"}");
}

// Register the tool
luup_tool tool = {
    .name = "get_weather",
    .description = "Get current weather for a city",
    .parameters_json = "{\"type\":\"object\",\"properties\":{\"city\":{\"type\":\"string\"}}}"
};

luup_agent_register_tool(agent, &tool, get_weather_callback, NULL);

// Agent will now call the tool when appropriate
char* response = luup_agent_generate(agent, "What's the weather in Seattle?");
```

### Streaming Generation

```c
void token_callback(const char* token, void* user_data) {
    printf("%s", token);
    fflush(stdout);
}

luup_agent_generate_stream(agent, "Tell me a story", token_callback, NULL);
```

## Language Bindings

### Python (Coming in v0.1)

```python
from luup_agent import Model, Agent

# Create and warmup model
model = Model.from_local("model.gguf", gpu_layers=-1)
model.warmup()

# Create agent
agent = Agent(model, system_prompt="You are a helpful assistant.")

# Generate response
response = agent.generate("Hello!")
print(response)

# Async streaming
async for token in agent.generate_async("Tell me a story"):
    print(token, end='', flush=True)

# Tool registration with decorator
@agent.tool(description="Get current weather")
def get_weather(city: str) -> dict:
    return {"temperature": 72, "condition": "sunny"}
```

### C# (Unity) (Coming in v0.2)

```csharp
var model = Model.FromLocal("model.gguf", gpuLayers: -1);
model.Warmup();

var agent = new Agent(model, "You are a game NPC.");
agent.RegisterTool("attack", "Deal damage to player", schema, (params) => {
    // Handle tool call
    return "{\"success\": true}";
});

agent.GenerateStream("What should I do?", token => Debug.Log(token));
```

## Platform Support

| Platform | Backend | Status |
|----------|---------|--------|
| macOS (Apple Silicon) | Metal | ✅ Supported |
| macOS (Intel) | CPU | ✅ Supported |
| Windows | CUDA | ✅ Supported |
| Windows | ROCm | ✅ Supported |
| Windows | Vulkan | ✅ Supported |
| Linux | CUDA | ✅ Supported |
| Linux | ROCm | ✅ Supported |
| Linux | CPU | ✅ Supported |

## Architecture

```
┌─────────────────────────────────────┐
│         Agent Layer                 │
│  ┌──────────┐  ┌──────────────┐    │
│  │ Agent 1  │  │   Agent 2    │    │
│  │ System   │  │   System     │    │
│  │ Prompt   │  │   Prompt     │    │
│  │ + Tools  │  │   + Tools    │    │
│  └────┬─────┘  └──────┬───────┘    │
└───────┼────────────────┼────────────┘
        │                │
        └────────┬───────┘
                 │ (shared)
        ┌────────▼────────┐
        │  Model Layer    │
        │  ┌───────────┐  │
        │  │  llama.cpp│  │
        │  │  or API   │  │
        │  └─────┬─────┘  │
        └────────┼────────┘
                 │
        ┌────────▼────────┐
        │   GPU Backend   │
        │ Metal/CUDA/etc  │
        └─────────────────┘
```

## Documentation

- [API Reference](docs/api_reference.md) - Complete C API documentation
- [Quick Start Guide](docs/quickstart.md) - Get started in 5 minutes
- [Tool Calling Guide](docs/tool_calling_guide.md) - How to add custom tools
- [Examples](examples/) - Sample code for various use cases

## Building Options

```bash
# Build with tests
cmake -DLUUP_BUILD_TESTS=ON ..

# Build with examples
cmake -DLUUP_BUILD_EXAMPLES=ON ..

# Build with language bindings
cmake -DLUUP_BUILD_BINDINGS=ON ..

# Force specific backend
cmake -DGGML_METAL=ON ..      # macOS Metal
cmake -DGGML_CUDA=ON ..       # NVIDIA CUDA
cmake -DGGML_ROCM=ON ..       # AMD ROCm
cmake -DGGML_VULKAN=ON ..     # Vulkan (universal)
```

## Performance

Typical performance on M1 Max (Metal backend):
- First token: ~50ms (after warmup)
- Throughput: 40-60 tokens/second
- Memory: ~2GB for 7B model, <100MB per agent

## License

MIT License - see [LICENSE](LICENSE) for details.

## Contributing

Contributions welcome! Please see our [contributing guidelines](CONTRIBUTING.md).

## Roadmap

### ✅ Completed (v0.1-dev)
- [x] **Phase 0**: Repository setup and build system
- [x] **Phase 1**: Model layer with llama.cpp backend
- [x] **Phase 2**: Agent layer with tool calling
  - [x] Multi-turn conversation management
  - [x] Tool registration and execution
  - [x] Streaming generation API
  - [x] History serialization

### 🚧 In Progress
- [ ] **Phase 3**: Built-in tools (todo, notes, auto-summarization)
- [ ] **Phase 4**: Python bindings (PRIORITY - v0.1)

### 📋 Planned
- [ ] Remote API support (OpenAI-compatible)
- [ ] C# bindings for Unity (v0.2)
- [ ] Multi-agent messaging
- [ ] Unreal Engine plugin

## Support

- GitHub Issues: https://github.com/gialup03/luup-agent/issues
- Discussions: https://github.com/gialup03/luup-agent/discussions

## Acknowledgments

Built on top of:
- [llama.cpp](https://github.com/ggerganov/llama.cpp) - LLM inference
- [nlohmann/json](https://github.com/nlohmann/json) - JSON parsing
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) - HTTP client

