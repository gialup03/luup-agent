# Quick Start Guide

Get started with luup-agent in 5 minutes.

## Installation

### Option 1: Build from Source

```bash
# Clone the repository with submodules
git clone --recursive https://github.com/gialup03/luup-agent.git
cd luup-agent

# Create build directory
mkdir build && cd build

# Configure (will auto-detect GPU backend)
cmake ..

# Build
cmake --build . --config Release

# Optional: Install
sudo cmake --install .
```

### Option 2: Use Pre-built Binaries

Download pre-built binaries for your platform from the [releases page](https://github.com/gialup03/luup-agent/releases).

## Your First Agent

### 1. Choose Your Backend

luup-agent supports two backends:

**Option A: Local Models (GGUF)**
- No internet required
- Full privacy and control
- Requires model download (~600MB-5GB)

**Option B: Remote APIs (OpenAI-compatible)**
- No model download needed
- Access to powerful cloud models
- Requires API key and internet

### 2. Get a Model

#### For Local Backend

Download a GGUF model. For testing, we recommend starting with a small model:

```bash
# Create models directory
mkdir models

# Download TinyLlama (1.1B parameters, ~600MB)
wget https://huggingface.co/TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF/resolve/main/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf -O models/tinyllama.gguf
```

#### For Remote Backend

Get an API key from your provider:
- **OpenAI**: https://platform.openai.com/api-keys
- **Ollama** (local): No key needed, runs at `http://localhost:11434/v1`
- **OpenRouter**: https://openrouter.ai/keys

### 3. Basic Chat Example (C)

#### Using Local Model

```c
#include <luup_agent.h>
#include <stdio.h>

int main() {
    // Create local model
    luup_model_config model_config = {
        .path = "models/tinyllama.gguf",
        .gpu_layers = -1,  // Use all GPU layers available
        .context_size = 2048,
        .threads = 0  // Auto-detect
    };
    
    luup_model* model = luup_model_create_local(&model_config);
    if (!model) {
        fprintf(stderr, "Error: %s\n", luup_get_last_error());
        return 1;
    }
    
    // Warmup (optional but recommended for local models)
    luup_model_warmup(model);
```

#### Using Remote API

```c
#include <luup_agent.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Get API key from environment
    const char* api_key = getenv("OPENAI_API_KEY");
    if (!api_key) {
        fprintf(stderr, "Please set OPENAI_API_KEY environment variable\n");
        return 1;
    }
    
    // Create remote model
    luup_model_config model_config = {
        .path = "gpt-3.5-turbo",  // Model name
        .api_key = api_key,
        .api_base_url = "https://api.openai.com/v1",  // Optional
        .context_size = 4096
    };
    
    luup_model* model = luup_model_create_remote(&model_config);
    if (!model) {
        fprintf(stderr, "Error: %s\n", luup_get_last_error());
        return 1;
    }
    
    // Create agent
    luup_agent_config agent_config = {
        .model = model,
        .system_prompt = "You are a helpful assistant.",
        .temperature = 0.7f,
        .max_tokens = 512,
        .enable_tool_calling = false,
        .enable_history_management = true
    };
    
    luup_agent* agent = luup_agent_create(&agent_config);
    
    // Generate response
    char* response = luup_agent_generate(agent, "Hello! Who are you?");
    printf("Assistant: %s\n", response);
    luup_free_string(response);
    
    // Cleanup
    luup_agent_destroy(agent);
    luup_model_destroy(model);
    
    return 0;
}
```

### 4. Compile and Run

#### For Local Models

```bash
# Compile
gcc -o my_agent my_agent.c -lluup_agent

# Run
./my_agent
```

#### For Remote APIs

```bash
# Compile
gcc -o my_agent my_agent.c -lluup_agent

# Set API key and run
export OPENAI_API_KEY="sk-..."
./my_agent
```

**Using Ollama (Local Server):**

```bash
# Start Ollama server
ollama serve

# In another terminal
export API_ENDPOINT="http://localhost:11434/v1"
export API_KEY="ollama"  # Any value works for local Ollama
./my_agent
```

## Using Streaming

For responsive UIs, use streaming generation:

```c
void on_token(const char* token, void* user_data) {
    printf("%s", token);
    fflush(stdout);
}

luup_agent_generate_stream(agent, "Tell me a story", on_token, NULL);
```

## Adding Tools

Make your agent more capable by adding tools:

```c
// Define a tool callback
char* get_time_callback(const char* params_json, void* user_data) {
    time_t now = time(NULL);
    char* result = malloc(256);
    snprintf(result, 256, "{\"time\": \"%s\"}", ctime(&now));
    return result;
}

// Register the tool
luup_tool time_tool = {
    .name = "get_current_time",
    .description = "Get the current time",
    .parameters_json = "{\"type\": \"object\", \"properties\": {}}"
};

luup_agent_register_tool(agent, &time_tool, get_time_callback, NULL);

// Agent will now call the tool when appropriate
char* response = luup_agent_generate(agent, "What time is it?");
```

## Multiple Agents, One Model

Share a model across multiple agents for memory efficiency:

```c
luup_model* model = luup_model_create_local(&config);

// Create multiple agents with different personalities
luup_agent* agent1 = luup_agent_create(&(luup_agent_config){
    .model = model,
    .system_prompt = "You are a friendly assistant."
});

luup_agent* agent2 = luup_agent_create(&(luup_agent_config){
    .model = model,
    .system_prompt = "You are a technical expert."
});

// Both agents share the same model in memory
```

## Remote API Examples

### Python with Remote API

```python
from luup_agent import Model, Agent

# Using OpenAI
model = Model.from_remote(
    endpoint="https://api.openai.com/v1",
    api_key="sk-...",
    context_size=4096
)

# Or using Ollama (local)
model = Model.from_remote(
    endpoint="http://localhost:11434/v1",
    api_key="ollama",
    context_size=2048
)

agent = Agent(model, system_prompt="You are a helpful assistant.")
response = agent.generate("Hello!")
print(response)
```

### Supported Remote Providers

- **OpenAI**: `https://api.openai.com/v1` (gpt-4, gpt-3.5-turbo, etc.)
- **Ollama**: `http://localhost:11434/v1` (local server, any model)
- **OpenRouter**: `https://openrouter.ai/api/v1` (access to many models)
- **Any OpenAI-compatible API**: Custom endpoints welcome!

## Next Steps

- Read the [API Reference](api_reference.md) for complete documentation
- Learn about [Tool Calling](tool_calling_guide.md) in depth
- See [Remote API Guide](phases/PHASE5_COMPLETE.md) for advanced usage
- Check out the [examples](../examples/) directory
- Join our [community discussions](https://github.com/gialup03/luup-agent/discussions)

## Troubleshooting

### Model not loading

- Verify the GGUF file exists and is not corrupted
- Check that you have enough RAM (model size + ~1GB)
- Try with `gpu_layers = 0` to force CPU-only mode

### Slow inference

- Increase `gpu_layers` to offload more to GPU
- Use a smaller/quantized model
- Enable model warmup with `luup_model_warmup()`

### Out of memory

- Reduce `context_size`
- Use a smaller model
- Reduce `gpu_layers` to free GPU memory

## Platform-Specific Notes

### macOS

- Metal backend is automatically used on Apple Silicon
- Best performance on M1/M2/M3 chips
- May need to allow app in System Preferences on first run

### Windows

- CUDA backend auto-detected if NVIDIA drivers installed
- ROCm backend for AMD GPUs
- Vulkan as universal fallback

### Linux

- CUDA and ROCm auto-detected
- May need to install GPU drivers separately
- CPU backend always available as fallback

