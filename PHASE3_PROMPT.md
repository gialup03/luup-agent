# Phase 3 Implementation Prompt: Built-in Tools

I'm working on luup-agent, a cross-platform C library for LLM inference with multi-agent 
support and tool calling.

## Current Status
- Phase 0 (Repository Setup) is COMPLETE ✅
- Phase 1 (Model Layer) is COMPLETE ✅ 
- Phase 2 (Agent Layer) is COMPLETE ✅ - Just finished and pushed to GitHub
- All code is committed to main branch (commit 58b1e3d)

## Phase 2 Accomplishments
✅ Complete agent layer with conversation management
✅ JSON-based tool calling system (tool_calling.cpp)
✅ Tool registration and execution
✅ Multi-turn conversation support (context_manager.cpp)
✅ Streaming generation with callbacks
✅ History serialization to JSON
✅ Comprehensive unit tests (11 test cases, all passing)
✅ ~666 lines of new production code
✅ Build system fully working on macOS (Metal backend)

Project Location: /Users/gluppi/projects/luup-agent

## Next Task: Implement Phase 3 - Built-in Tools

Focus on implementing three built-in tools that extend agent capabilities:

### 1. Todo List Tool (src/builtin_tools/todo_list.cpp)
- Add, list, update, and delete todo items
- Mark items as complete/incomplete
- Optional file persistence (JSON format)
- In-memory storage as default
- Tool schema for LLM to understand

### 2. Notes Tool (src/builtin_tools/notes.cpp)
- Create, read, update, delete notes
- Simple key-value storage (title -> content)
- Optional file persistence (JSON format)
- List all notes
- Search notes by title

### 3. Auto-Summarization (src/builtin_tools/summarization.cpp)
- Automatically summarize conversation history when context fills
- Replace old messages with summary to free space
- Preserve recent messages for context
- Trigger at configurable threshold (e.g., 80% context usage)
- Use the model itself to generate summaries

### API Functions to Implement (from luup_agent.h)

These functions are already declared but stubbed:

```c
// Enable built-in todo list tool
luup_error_t luup_agent_enable_builtin_todo(
    luup_agent* agent,
    const char* storage_path  // NULL for memory-only
);

// Enable built-in notes tool
luup_error_t luup_agent_enable_builtin_notes(
    luup_agent* agent,
    const char* storage_path  // NULL for memory-only
);

// Enable built-in auto-summarization
luup_error_t luup_agent_enable_builtin_summarization(
    luup_agent* agent
);
```

## Key Files to Reference

### API Specification
- `include/luup_agent.h` - C API with built-in tool declarations (lines 284-313)

### Implementation Stubs (Need to Complete)
- `src/builtin_tools/todo_list.cpp` - Currently empty stub
- `src/builtin_tools/notes.cpp` - Currently empty stub
- `src/builtin_tools/summarization.cpp` - Currently empty stub

### Reference Implementations
- `src/core/agent.cpp` - Agent implementation (shows how to register tools)
- `src/core/tool_calling.cpp` - Tool execution system (reference for tool schemas)
- `src/core/context_manager.cpp` - History management (needed for summarization)
- `src/core/internal.h` - Internal APIs

### Testing
- `tests/unit/test_tools.cpp` - Currently has placeholder tests, needs expansion

### Documentation
- `IMPLEMENTATION_PLAN.md` - Phase 3 details (lines 246-260)
- `DEVELOPMENT.md` - Updated through Phase 2
- `PHASE2_COMPLETE.md` - Reference for Phase 2 patterns

## Implementation Requirements

### 1. Todo List Tool

**Functionality:**
- Store todo items with: id, title, description, completed status, created_at
- Operations: add, list, update, complete, delete
- JSON persistence (optional)
- Auto-increment IDs

**Tool Schema:**
```json
{
  "name": "todo",
  "description": "Manage todo list items",
  "parameters": {
    "type": "object",
    "properties": {
      "action": {
        "type": "string",
        "enum": ["add", "list", "update", "complete", "delete"]
      },
      "id": {"type": "number"},
      "title": {"type": "string"},
      "description": {"type": "string"}
    },
    "required": ["action"]
  }
}
```

**Example Usage:**
```c
luup_agent_enable_builtin_todo(agent, "todos.json");
char* response = luup_agent_generate(agent, "Add a todo: finish the report");
// LLM calls todo tool -> Creates item -> Returns confirmation
```

### 2. Notes Tool

**Functionality:**
- Store notes with: id, title, content, created_at, updated_at
- Operations: create, read, list, update, delete, search
- JSON persistence (optional)
- Support multi-line content

**Tool Schema:**
```json
{
  "name": "notes",
  "description": "Manage notes and information",
  "parameters": {
    "type": "object",
    "properties": {
      "action": {
        "type": "string",
        "enum": ["create", "read", "list", "update", "delete", "search"]
      },
      "title": {"type": "string"},
      "content": {"type": "string"}
    },
    "required": ["action"]
  }
}
```

### 3. Auto-Summarization

**Functionality:**
- Monitor context window usage
- Trigger summarization at threshold (default 80%)
- Generate summary using the model
- Replace old messages with summary message
- Preserve system prompt and recent messages

**Implementation Steps:**
1. Hook into generation pipeline (check context before generate)
2. When threshold reached:
   - Extract messages to summarize (keep last N messages)
   - Create summarization prompt
   - Call llama_backend_generate() to get summary
   - Replace old messages with single summary message
3. Continue with normal generation

**Configuration:**
- Threshold percentage (default 0.8)
- Number of recent messages to preserve (default 5)
- Summary prompt template

## Technical Considerations

### Data Structures
- Use C++ classes/structs internally (private to .cpp files)
- Store todo/note data in std::vector or std::map
- JSON serialization with nlohmann/json (already available)

### File Persistence
- Load from file on enable (if exists)
- Auto-save after each modification
- Handle file I/O errors gracefully
- Use JSON format for human readability

### Tool Integration
- Use `luup_agent_register_tool()` to register each built-in tool
- Implement tool callback functions
- Parse JSON parameters using nlohmann/json
- Return JSON responses

### Error Handling
- Use `luup_set_error()` for error reporting
- Validate all parameters
- Handle missing files gracefully
- Return proper error codes

## Success Criteria

Phase 3 will be complete when:
- ✅ Todo list tool fully functional with persistence
- ✅ Notes tool fully functional with persistence
- ✅ Auto-summarization working and triggered automatically
- ✅ All three tools can be enabled via API functions
- ✅ Unit tests for each tool (add to test_tools.cpp)
- ✅ Integration tests showing tools working with agents
- ✅ File persistence working correctly
- ✅ Error handling for all edge cases
- ✅ Build passes on macOS (and ideally other platforms)
- ✅ Documentation updated (DEVELOPMENT.md, PHASE3_COMPLETE.md)

## Build & Test

```bash
cd /Users/gluppi/projects/luup-agent
./build.sh  # Or: mkdir build && cd build && cmake .. && make -j4
cd build && ctest  # Run all tests
```

## Example Test Cases to Add

```cpp
TEST_CASE("Todo list operations", "[tools][todo]") {
    // Test add, list, complete, delete
}

TEST_CASE("Todo list persistence", "[tools][todo]") {
    // Test save and load from file
}

TEST_CASE("Notes operations", "[tools][notes]") {
    // Test create, read, update, delete
}

TEST_CASE("Auto-summarization", "[tools][summarization]") {
    // Test that summarization triggers at threshold
}
```

## Implementation Tips

1. **Start with todo_list.cpp**: Simpler than notes, good for establishing patterns
2. **Use existing tool_calling patterns**: Reference how tools are registered in agent.cpp
3. **JSON format for storage**: Easy to read, debug, and extend
4. **Test incrementally**: Build -> test -> iterate
5. **Summarization is complex**: May need to extend context_manager.cpp utilities

## Important Context

- Error handling system is in place (use luup_set_error/luup_clear_error)
- nlohmann/json is already integrated and working
- Tool registration system is proven (Phase 2)
- Agent layer handles tool execution automatically
- File I/O is standard C++, no special libraries needed

Please begin Phase 3 implementation, starting with the todo list tool as it's the 
simplest and will establish patterns for the others.

---

## Alternative: Phase 4 Prompt (Python Bindings - PRIORITY)

If you'd prefer to work on **Phase 4 (Python Bindings)** instead, which is marked as 
PRIORITY for v0.1, let me know and I'll provide that prompt instead. Python bindings 
would involve:
- ctypes/cffi wrappers for the C API
- Pythonic Model and Agent classes
- Async/await support
- Type hints and mypy compatibility
- pytest test suite
- pip package setup

Both phases are valuable - Phase 3 extends agent capabilities, while Phase 4 makes 
the library accessible to the ML/AI Python community.

