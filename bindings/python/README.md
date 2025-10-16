# luup-agent Python Bindings

Python bindings for luup-agent, a cross-platform C library for LLM inference with multi-agent support and tool calling.

## Features

- 🚀 **Fast Local Inference**: Powered by llama.cpp with GPU acceleration (Metal/CUDA/ROCm/Vulkan)
- 🔧 **Tool Calling**: Easy-to-use `@agent.tool()` decorator for function registration
- 🔄 **Streaming**: Real-time token-by-token generation with sync and async APIs
- 🎯 **Type Safe**: Full type hints for IDE support and mypy
- 🐍 **Pythonic**: Context managers, decorators, exceptions (not error codes)
- 🌐 **Remote APIs**: Support for OpenAI-compatible APIs
- 💬 **History Management**: Automatic conversation history tracking

## Installation

### From Source (Development)

First, build the C library:

```bash
cd /path/to/luup-agent
./build.sh  # or build.bat on Windows
```

Then install the Python package in development mode:

```bash
cd bindings/python
pip install -e ".[dev]"
```

### Requirements

- Python 3.8+
- Built luup-agent C library (see main README)
- GGUF model file for local inference

## Quick Start

### Basic Chat

```python
from luup_agent import Model, Agent

# Create a local model
model = Model.from_local(
    "models/qwen2-0.5b-instruct-q4_k_m.gguf",
    gpu_layers=-1  # Use all available GPU
)

# Create an agent
agent = Agent(
    model,
    system_prompt="You are a helpful assistant.",
    temperature=0.7
)

# Generate response
response = agent.generate("Hello!")
print(response)
```

### Tool Calling

```python
from luup_agent import Model, Agent

model = Model.from_local("models/qwen2-0.5b-instruct-q4_k_m.gguf")
agent = Agent(model, system_prompt="You are a helpful assistant with tools.")

# Register a tool with decorator
@agent.tool(description="Get current weather for a city")
def get_weather(city: str, units: str = "celsius") -> dict:
    """Get weather data for a city."""
    # Your weather API call here
    return {
        "city": city,
        "temperature": 72,
        "condition": "sunny",
        "units": units
    }

# Agent will automatically call the tool when appropriate
response = agent.generate("What's the weather in Paris?")
print(response)
```

### Streaming

```python
# Synchronous streaming
for token in agent.generate_stream("Tell me a story"):
    print(token, end='', flush=True)

# Asynchronous streaming
import asyncio

async def stream_example():
    async for token in agent.generate_async("Tell me a story"):
        print(token, end='', flush=True)

asyncio.run(stream_example())
```

### Context Managers

```python
# Automatic resource cleanup
with Model.from_local("model.gguf") as model:
    with Agent(model) as agent:
        response = agent.generate("Hello")
        print(response)
# Resources automatically freed
```

## API Reference

### Model

Create and manage LLM models:

```python
# Local model
model = Model.from_local(
    path="model.gguf",
    gpu_layers=-1,      # -1 = auto, 0 = CPU only, N = specific count
    context_size=2048,  # Context window size
    threads=0,          # 0 = auto-detect
)

# Remote API model
model = Model.from_remote(
    endpoint="https://api.openai.com/v1",
    api_key="sk-...",
    context_size=4096,
)

# Get model info
info = model.get_info()
print(f"Backend: {info['backend']}")
print(f"Device: {info['device']}")

# Warmup (reduces first-token latency)
model.warmup()
```

### Agent

Create and interact with AI agents:

```python
agent = Agent(
    model=model,
    system_prompt="You are a helpful assistant.",
    temperature=0.7,        # 0.0 to 2.0
    max_tokens=512,         # 0 = no limit
    enable_tool_calling=True,
    enable_history=True,
)

# Generate responses
response = agent.generate("Hello")

# Streaming
for token in agent.generate_stream("Tell me a story"):
    print(token, end='')

# Async streaming
async for token in agent.generate_async("Tell me a story"):
    print(token, end='')

# History management
agent.add_message("user", "Hello")
agent.add_message("assistant", "Hi there!")
history = agent.get_history()
agent.clear_history()

# Built-in tools
agent.enable_builtin_todo("todos.json")
agent.enable_builtin_notes("notes.json")
agent.enable_builtin_summarization()
```

### Tool Decorator

Register functions as tools:

```python
# Basic usage
@agent.tool(description="Add two numbers")
def add(a: int, b: int) -> int:
    return a + b

# Custom name
@agent.tool(name="custom_name", description="Custom tool")
def my_func():
    return "result"

# Explicit schema
@agent.tool(
    name="calc",
    description="Calculate",
    schema={
        "type": "object",
        "properties": {
            "x": {"type": "number"},
            "y": {"type": "number"}
        },
        "required": ["x", "y"]
    }
)
def calculate(x: float, y: float) -> float:
    return x + y
```

## Exception Handling

Python bindings use exceptions instead of error codes:

```python
from luup_agent import (
    LuupError,           # Base exception
    ModelNotFoundError,  # Model file not found
    InferenceError,      # Generation failed
    ToolNotFoundError,   # Tool not registered
    # ... and more
)

try:
    model = Model.from_local("nonexistent.gguf")
except ModelNotFoundError as e:
    print(f"Model not found: {e}")

try:
    response = agent.generate("Hello")
except InferenceError as e:
    print(f"Generation failed: {e}")
```

## Examples

See the `examples/` directory for complete examples:

- `basic_chat.py` - Simple chat loop
- `tool_calling.py` - Tool registration and usage
- `async_streaming.py` - Async/await streaming

Run examples:

```bash
python examples/basic_chat.py models/qwen2-0.5b-instruct-q4_k_m.gguf
python examples/tool_calling.py models/qwen2-0.5b-instruct-q4_k_m.gguf
python examples/async_streaming.py models/qwen2-0.5b-instruct-q4_k_m.gguf
```

## Development

### Setup Development Environment

```bash
# Install with dev dependencies
pip install -e ".[dev]"

# Run tests
pytest tests/ -v

# Run tests with coverage
pytest tests/ --cov=luup_agent --cov-report=html

# Type checking
mypy luup_agent/

# Format code
black luup_agent/ tests/ examples/
ruff check luup_agent/ tests/ examples/
```

### Project Structure

```
bindings/python/
├── luup_agent/           # Package source
│   ├── __init__.py       # Package exports
│   ├── _native.py        # ctypes bindings
│   ├── model.py          # Model class
│   ├── agent.py          # Agent class
│   └── exceptions.py     # Exception classes
├── tests/                # Test suite
│   ├── conftest.py       # Pytest fixtures
│   ├── test_model.py
│   ├── test_agent.py
│   └── test_tools.py
├── examples/             # Example scripts
├── setup.py              # Package setup
├── pyproject.toml        # Modern Python packaging
└── README.md             # This file
```

## Platform Support

- **macOS**: ✅ Tested (Metal GPU acceleration)
- **Linux**: ✅ Supported (CUDA/ROCm/Vulkan)
- **Windows**: ✅ Supported (CUDA/Vulkan)

## Performance Tips

1. **Use GPU acceleration**: Set `gpu_layers=-1` for best performance
2. **Warmup the model**: Call `model.warmup()` before first generation
3. **Adjust context size**: Smaller = faster, larger = more context
4. **Use streaming**: Better UX for long responses
5. **Reuse models**: Share one model across multiple agents

## Troubleshoading

### Library Not Found

If you get "Failed to load luup-agent library":

1. Make sure the C library is built: `./build.sh`
2. Check that `build/libluup_agent.dylib` (or `.so`/`.dll`) exists
3. Set `LUUP_LIBRARY_PATH` environment variable:
   ```bash
   export LUUP_LIBRARY_PATH=/path/to/luup-agent/build
   ```

### Model Not Found

Make sure the model file path is correct and the file exists:

```python
from pathlib import Path
model_path = Path("models/qwen2-0.5b-instruct-q4_k_m.gguf")
assert model_path.exists(), f"Model not found: {model_path}"
```

### Import Errors

Make sure you're importing from the correct package:

```python
# Correct
from luup_agent import Model, Agent

# Incorrect
from luup_agent.model import Model  # Don't do this
```

## License

MIT License - see LICENSE file in the main repository.

## Contributing

See CONTRIBUTING.md in the main repository for guidelines.

## Links

- **Main Repository**: https://github.com/gialup03/luup-agent
- **Documentation**: See `docs/` directory in main repo
- **Issues**: https://github.com/gialup03/luup-agent/issues

