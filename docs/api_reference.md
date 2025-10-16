# API Reference

Complete C API documentation for luup-agent.

## Table of Contents

- [Error Handling](#error-handling)
- [Model Layer](#model-layer)
- [Agent Layer](#agent-layer)
- [Tool Calling](#tool-calling)
- [Memory Management](#memory-management)
- [Version Information](#version-information)

## Error Handling

All functions that can fail return either `NULL` (for pointers) or `luup_error_t` (for operations).

### Error Codes

```c
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
```

### Getting Error Messages

```c
const char* luup_get_last_error(void);
```

Returns the last error message for the current thread. The returned string is valid until the next luup API call in the same thread.

**Example:**
```c
luup_model* model = luup_model_create_local(&config);
if (!model) {
    fprintf(stderr, "Error: %s\n", luup_get_last_error());
}
```

### Error Callbacks

```c
typedef void (*luup_error_callback_t)(luup_error_t code, const char* msg, void* user_data);
void luup_set_error_callback(luup_error_callback_t callback, void* user_data);
```

Set a global error callback for diagnostics and logging.

**Example:**
```c
void error_handler(luup_error_t code, const char* msg, void* data) {
    fprintf(stderr, "[ERROR %d] %s\n", code, msg);
}

luup_set_error_callback(error_handler, NULL);
```

## Model Layer

### Types

```c
typedef struct luup_model luup_model;  // Opaque handle
```

### Configuration

```c
typedef struct {
    const char* path;           // Path to GGUF file or API URL
    int gpu_layers;             // -1: auto, 0: CPU only, N: specific count
    int context_size;           // Context window size (default: 2048)
    int threads;                // CPU threads (0: auto-detect)
    const char* api_key;        // For remote models
    const char* api_base_url;   // Custom API endpoint
} luup_model_config;
```

### Functions

#### Create Local Model

```c
luup_model* luup_model_create_local(const luup_model_config* config);
```

Creates a model using the local llama.cpp backend.

**Parameters:**
- `config`: Model configuration

**Returns:** Model handle or `NULL` on error

**Example:**
```c
luup_model_config config = {
    .path = "models/llama-7b.gguf",
    .gpu_layers = -1,
    .context_size = 2048,
    .threads = 4
};

luup_model* model = luup_model_create_local(&config);
```

#### Create Remote Model

```c
luup_model* luup_model_create_remote(const luup_model_config* config);
```

Creates a model using a remote API endpoint (OpenAI-compatible).

**Example:**
```c
luup_model_config config = {
    .path = "gpt-3.5-turbo",
    .api_key = "sk-...",
    .api_base_url = "https://api.openai.com/v1"
};

luup_model* model = luup_model_create_remote(&config);
```

#### Warmup Model

```c
luup_error_t luup_model_warmup(luup_model* model);
```

Pre-warms the model by running a dummy inference. Reduces first-token latency.

#### Get Model Info

```c
typedef struct {
    const char* backend;        // "llama.cpp", "openai", etc.
    const char* device;         // "Metal", "CUDA", "ROCm", "Vulkan", "CPU"
    int gpu_layers_loaded;      // Actual GPU layers
    size_t memory_usage;        // Bytes
    int context_size;           // Context window
} luup_model_info;

luup_error_t luup_model_get_info(luup_model* model, luup_model_info* out_info);
```

#### Destroy Model

```c
void luup_model_destroy(luup_model* model);
```

Frees all resources associated with the model.

## Agent Layer

### Types

```c
typedef struct luup_agent luup_agent;  // Opaque handle
```

### Configuration

```c
typedef struct {
    luup_model* model;                  // Model to use (can be shared)
    const char* system_prompt;          // Agent's role
    float temperature;                  // 0.0 to 2.0 (default: 0.7)
    int max_tokens;                     // Max generation length
    bool enable_tool_calling;           // Enable tools (default: true)
    bool enable_history_management;     // Auto-manage history (default: true)
} luup_agent_config;
```

### Functions

#### Create Agent

```c
luup_agent* luup_agent_create(const luup_agent_config* config);
```

Creates a new agent.

**Example:**
```c
luup_agent_config config = {
    .model = model,
    .system_prompt = "You are a helpful assistant.",
    .temperature = 0.7f,
    .max_tokens = 512,
    .enable_tool_calling = true,
    .enable_history_management = true
};

luup_agent* agent = luup_agent_create(&config);
```

#### Generate Response (Blocking)

```c
char* luup_agent_generate(luup_agent* agent, const char* user_message);
```

Generates a complete response. Returns `NULL` on error.

**Must free result with `luup_free_string()`.**

#### Generate Response (Streaming)

```c
typedef void (*luup_stream_callback_t)(const char* token, void* user_data);

luup_error_t luup_agent_generate_stream(
    luup_agent* agent,
    const char* user_message,
    luup_stream_callback_t callback,
    void* user_data
);
```

Generates response token-by-token.

**Example:**
```c
void on_token(const char* token, void* data) {
    printf("%s", token);
    fflush(stdout);
}

luup_agent_generate_stream(agent, "Hello!", on_token, NULL);
```

#### History Management

```c
luup_error_t luup_agent_add_message(luup_agent* agent, const char* role, const char* content);
luup_error_t luup_agent_clear_history(luup_agent* agent);
char* luup_agent_get_history_json(luup_agent* agent);
```

Manually manage conversation history.

#### Destroy Agent

```c
void luup_agent_destroy(luup_agent* agent);
```

## Tool Calling

### Tool Definition

```c
typedef struct {
    const char* name;               // Unique tool name
    const char* description;        // Human-readable description
    const char* parameters_json;    // JSON schema
} luup_tool;
```

### Tool Callback

```c
typedef char* (*luup_tool_callback_t)(const char* params_json, void* user_data);
```

Tool callbacks receive JSON parameters and return JSON results.

**Must return dynamically allocated string (will be freed by library).**

### Register Tool

```c
luup_error_t luup_agent_register_tool(
    luup_agent* agent,
    const luup_tool* tool,
    luup_tool_callback_t callback,
    void* user_data
);
```

**Example:**
```c
char* weather_callback(const char* params, void* data) {
    // Parse params, call API, return JSON
    return strdup("{\"temp\": 72, \"condition\": \"sunny\"}");
}

luup_tool tool = {
    .name = "get_weather",
    .description = "Get weather for a city",
    .parameters_json = "{\"type\":\"object\",\"properties\":{\"city\":{\"type\":\"string\"}}}"
};

luup_agent_register_tool(agent, &tool, weather_callback, NULL);
```

### Built-in Tools

```c
luup_error_t luup_agent_enable_builtin_todo(luup_agent* agent, const char* storage_path);
luup_error_t luup_agent_enable_builtin_notes(luup_agent* agent, const char* storage_path);
luup_error_t luup_agent_enable_builtin_summarization(luup_agent* agent);
```

Enable pre-built tools for common tasks.

## Memory Management

```c
void luup_free_string(char* str);
```

Frees strings returned by the library. Use this for:
- `luup_agent_generate()` results
- `luup_agent_get_history_json()` results
- Tool callback return values (after library processes them)

**Important:** Don't use regular `free()` on library-allocated strings.

## Version Information

```c
const char* luup_version(void);
void luup_version_components(int* major, int* minor, int* patch);
```

**Example:**
```c
printf("luup-agent version: %s\n", luup_version());

int major, minor, patch;
luup_version_components(&major, &minor, &patch);
printf("Version: %d.%d.%d\n", major, minor, patch);
```

## Thread Safety

- **Model handles** are thread-safe for read operations
- **Agent handles** are NOT thread-safe - use one agent per thread
- **Error messages** are thread-local
- **Callbacks** are executed on the calling thread

## Best Practices

1. **Always check return values** for `NULL` or error codes
2. **Free strings** returned by the library
3. **Warmup models** for better first-token latency
4. **Share models** across agents when possible
5. **Use streaming** for responsive UIs
6. **Set error callbacks** for better debugging

