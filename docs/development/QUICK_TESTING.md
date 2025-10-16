# Quick Testing Guide

## Ad-hoc Testing with Small Models

For quick interactive testing during development, use the existing examples with a small model.

### Download a Small Model

```bash
mkdir -p models
cd models
curl -L -o qwen2-0.5b.gguf "https://huggingface.co/Qwen/Qwen2-0.5B-Instruct-GGUF/resolve/main/qwen2-0_5b-instruct-q4_k_m.gguf"
```

### Build and Test

```bash
# Build examples
mkdir -p build && cd build
cmake .. -DLUUP_BUILD_EXAMPLES=ON
cmake --build . --config Release

# Test basic chat (no tools)
./examples/basic_chat ../models/qwen2-0.5b.gguf

# Test tool calling
./examples/tool_calling ../models/qwen2-0.5b.gguf
```

### Interactive Testing

For interactive sessions, modify `examples/basic_chat.cpp` to add a simple loop:

```cpp
char user_input[1024];
while (true) {
    printf("You: ");
    if (!fgets(user_input, sizeof(user_input), stdin)) break;
    
    // Remove newline
    user_input[strcspn(user_input, "\n")] = 0;
    if (strcmp(user_input, "quit") == 0) break;
    
    char* response = luup_agent_generate(agent, user_input);
    printf("Assistant: %s\n\n", response);
    luup_free_string(response);
}
```

## Key Fixes Applied (Branch: fix/tokenization-api)

### 1. Tokenization API (Critical)
**Issue**: `llama_tokenize()` returns negative values to indicate buffer size needed  
**Fix**: Check for negative return, resize buffer, and retry

```cpp
int n = llama_tokenize(vocab, text, len, nullptr, 0, true, true);
if (n < 0) {
    tokens.resize(-n);
    n = llama_tokenize(vocab, text, len, tokens.data(), tokens.size(), true, true);
}
```

### 2. Chat Template Format (Critical)
**Issue**: Generic "System:/User:/Assistant:" format doesn't work with modern models  
**Fix**: Use ChatML format with `<|im_start|>` and `<|im_end|>` tokens

```cpp
// Before:
"System: " + prompt + "\n\nUser: " + msg + "\n\nAssistant: "

// After:
"<|im_start|>system\n" + prompt + "<|im_end|>\n" +
"<|im_start|>user\n" + msg + "<|im_end|>\n" +
"<|im_start|>assistant\n"
```

### 3. Tool Schema Insertion
**Issue**: Searching for "User:" in ChatML formatted prompts fails  
**Fix**: Search for `<|im_start|>user` instead

### 4. JSON Parsing for Tool Calls
**Issue**: Regex fails on nested JSON objects  
**Fix**: Proper brace-matching parser that handles nesting

### 5. Optional Tool Parameters
**Issue**: Tools without parameters (like `get_time`) were ignored  
**Fix**: Make `parameters` field optional, default to `{}`

## Testing Recommendations

**Model Size vs Capability:**
- **Qwen2-0.5B (379MB)**: Fast but limited reasoning, basic chat works
- **Qwen2-1.5B (1GB)**: Better for testing tool calling
- **Phi-3-mini (2.3GB)**: Good quality for development testing

**Common Issues:**
- Context overflow: Use smaller context or `/clear` command
- Tool calling fails: Models <1B struggle with function calling
- Chinese responses: Ensure system prompt specifies "Always respond in English"

## Suppressing llama.cpp Logs

For clean output during development:

```cpp
// At program start
freopen("/dev/null", "w", stderr);  // Unix/Mac
// or
freopen("NUL", "w", stderr);  // Windows
```

This hides Metal/CUDA shader compilation and other verbose logs.

