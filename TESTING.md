# Ad-hoc Testing with Interactive CLI

Quick guide for testing luup-agent with small models using the interactive CLI.

## Quick Start

### 1. Build and Run

The easiest way to get started:

```bash
./test_cli.sh path/to/your-model.gguf
```

### 2. Manual Build (if preferred)

```bash
mkdir -p build && cd build
cmake .. -DLUUP_BUILD_EXAMPLES=ON
cmake --build . --target interactive_cli
./examples/interactive_cli path/to/your-model.gguf
```

## Usage

### Basic Chat

```bash
./test_cli.sh models/qwen-0.5b.gguf
```

Then just type your messages:
```
You: Hello! How are you?
Assistant: I'm doing great, thanks for asking! How can I help you today?

You: Tell me a joke
Assistant: Why did the programmer quit his job? Because he didn't get arrays! 😄
```

### With Options

```bash
# Disable tool calling (simpler, faster)
./test_cli.sh models/tiny-llama.gguf --no-tools

# Adjust temperature for more creative/deterministic output
./test_cli.sh models/phi-2.gguf --temp 0.9

# Larger context window
./test_cli.sh models/qwen-0.5b.gguf --ctx 4096
```

### Interactive Commands

While chatting, you can use these commands:

- `/help` - Show available commands
- `/clear` - Clear conversation history
- `/history` - View full conversation history (JSON format)
- `/quit` or `/exit` - Exit the program

## Built-in Test Tools

When tool calling is enabled (default), the agent has access to:

1. **calculator** - Perform basic math calculations
   - Try: "What's 42 * 13?"
   
2. **get_time** - Get current date and time
   - Try: "What time is it?"

These are simple mock tools perfect for testing if your model can do function calling.

## Recommended Small Models

For quick testing, these models work well:

1. **Qwen2-0.5B** (~300MB) - Fast and capable
   ```bash
   # Download from HuggingFace
   huggingface-cli download Qwen/Qwen2-0.5B-Instruct-GGUF qwen2-0_5b-instruct-q4_k_m.gguf
   ```

2. **TinyLlama-1.1B** (~600MB) - Good for basic testing
   ```bash
   huggingface-cli download TheBloke/TinyLlama-1.1B-Chat-v1.0-GGUF tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf
   ```

3. **Phi-2** (~1.6GB) - Better quality, still fast
   ```bash
   huggingface-cli download TheBloke/phi-2-GGUF phi-2.Q4_K_M.gguf
   ```

## Tips for Testing

1. **Start Small**: Use the smallest model first to verify everything works
2. **Watch GPU Usage**: The CLI will show which backend (Metal/CUDA/CPU) is being used
3. **Test Tool Calling**: Try asking questions that would benefit from tools
4. **Clear History**: Use `/clear` if the model starts repeating or behaving oddly
5. **Temperature**: Lower (0.1-0.3) for focused/deterministic, higher (0.8-1.2) for creative

## Example Session

```
====================================
  luup-agent Interactive CLI
  Version: 0.1.0
====================================

Loading model: models/qwen-0.5b.gguf
  Context size: 2048
  Temperature: 0.70
  Tools: enabled

Model loaded successfully!
  Backend: llama.cpp
  Device: Metal
  GPU layers: 32

Warming up model... done!

Registering tools...
  ✓ calculator
  ✓ get_time

====================================
Ready! Type your message or /help for commands.
====================================

You: What's 15 * 28?

  [Tool: calculator called with {"expression":"15 * 28"}]
Assistant: 15 multiplied by 28 equals 420.

You: What time is it?

  [Tool: get_time called]
Assistant: The current time is Thu Oct 16 14:23:45 2025.

You: /clear
✓ History cleared

You: /quit

👋 Goodbye!
```

## Troubleshooting

### Model not loading
- Make sure the path to the GGUF file is correct
- Check that you have enough memory (models need ~2-4x their file size in RAM)

### Slow performance
- Try with `--no-tools` to disable tool calling overhead
- Use a smaller model
- Check if GPU acceleration is working (should show "Metal" or "CUDA" as device)

### Build fails
- Make sure you have CMake 3.18+ and a C++17 compiler
- Try: `cd build && rm -rf * && cmake .. -DLUUP_BUILD_EXAMPLES=ON`

## Next Steps

Once you've tested the basic functionality:

1. Try registering your own custom tools (see `examples/tool_calling.cpp`)
2. Experiment with different system prompts
3. Test with different model sizes and quantizations
4. Build more complex agent behaviors

## Questions?

Check the main documentation:
- [API Reference](docs/api_reference.md)
- [Tool Calling Guide](docs/tool_calling_guide.md)
- [Quick Start Guide](docs/quickstart.md)

