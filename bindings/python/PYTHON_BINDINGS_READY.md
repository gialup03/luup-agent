# 🐍 Python Bindings Complete!

## Quick Start

```python
from luup_agent import Model, Agent

# Create model
model = Model.from_local("models/qwen2-0.5b-instruct-q4_k_m.gguf", gpu_layers=-1)

# Create agent with tools
agent = Agent(model, system_prompt="You are helpful.")

@agent.tool(description="Add two numbers")
def add(a: int, b: int) -> int:
    return a + b

# Generate response
response = agent.generate("What is 5 + 3?")
print(response)
```

## What's Included

✅ **Complete Python package** (`bindings/python/luup_agent/`)
- Pythonic Model and Agent classes
- `@agent.tool()` decorator
- Async/await streaming support
- Full type hints
- Context managers
- Exception-based error handling

✅ **28 test cases** (`bindings/python/tests/`)
- Model creation and management
- Agent functionality
- Tool registration and calling

✅ **3 complete examples** (`bindings/python/examples/`)
- `basic_chat.py` - Simple chat loop
- `tool_calling.py` - Custom tools
- `async_streaming.py` - Real-time streaming

✅ **Full documentation** (`bindings/python/README.md`)

## Installation

```bash
# In project root
./build.sh  # Build C library first

# Install Python package
cd bindings/python
python3 -m venv venv
source venv/bin/activate
pip install -e ".[dev]"
```

## Verification

```bash
cd bindings/python

# Quick verification (no model needed)
python3 verify_bindings.py

# Runtime test (needs model)
python3 test_basic_usage.py

# Full test suite
pytest tests/ -v
```

## Run Examples

```bash
cd bindings/python
python3 examples/basic_chat.py ../models/qwen2-0.5b-instruct-q4_k_m.gguf
python3 examples/tool_calling.py ../models/qwen2-0.5b-instruct-q4_k_m.gguf
python3 examples/async_streaming.py ../models/qwen2-0.5b-instruct-q4_k_m.gguf
```

## Features

### Context Managers
```python
with Model.from_local("model.gguf") as model:
    with Agent(model) as agent:
        response = agent.generate("Hello")
```

### Streaming
```python
# Sync
for token in agent.generate_stream("Hello"):
    print(token, end='', flush=True)

# Async
async for token in agent.generate_async("Hello"):
    print(token, end='', flush=True)
```

### Tool Decorator
```python
@agent.tool(description="Get weather")
def get_weather(city: str, units: str = "celsius") -> dict:
    return {"temp": 72, "condition": "sunny"}
```

## Stats

- **~1,120 lines** of package code
- **~530 lines** of tests (28 test cases)
- **~460 lines** of examples
- **~690 lines** of documentation
- **Total: ~2,800 lines**

## Documentation

- **Python README**: `bindings/python/README.md`
- **Phase 4 Complete**: `bindings/python/PHASE4_COMPLETE.md`
- **C API Reference**: `docs/api_reference.md`

## Status

✅ **Phase 4 COMPLETE** - All success criteria met!

---

For more details, see `bindings/python/README.md`

