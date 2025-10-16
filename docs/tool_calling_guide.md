# Tool Calling Guide

Learn how to extend agents with custom tools.

## What are Tools?

Tools (also called "function calling") allow agents to perform actions beyond text generation. When enabled, the agent can decide to call tools to get information or perform tasks.

## Basic Tool Registration

### 1. Define Your Tool

A tool needs three things:
- **Name**: Unique identifier
- **Description**: Tells the agent what the tool does
- **Parameters**: JSON schema defining expected inputs

```c
luup_tool weather_tool = {
    .name = "get_weather",
    .description = "Get current weather for a city",
    .parameters_json = 
        "{"
        "  \"type\": \"object\","
        "  \"properties\": {"
        "    \"city\": {"
        "      \"type\": \"string\","
        "      \"description\": \"City name\""
        "    },"
        "    \"units\": {"
        "      \"type\": \"string\","
        "      \"enum\": [\"celsius\", \"fahrenheit\"],"
        "      \"default\": \"celsius\""
        "    }"
        "  },"
        "  \"required\": [\"city\"]"
        "}"
};
```

### 2. Implement the Callback

```c
char* weather_callback(const char* params_json, void* user_data) {
    // 1. Parse the JSON parameters
    // For this example, we'll use simple string operations
    // In production, use a proper JSON library
    
    printf("Tool called with: %s\n", params_json);
    
    // 2. Perform the actual work
    // (call API, database query, etc.)
    
    // 3. Return result as JSON string
    // IMPORTANT: Must use malloc/strdup - library will free it
    return strdup(
        "{"
        "  \"temperature\": 72,"
        "  \"condition\": \"sunny\","
        "  \"humidity\": 45"
        "}"
    );
}
```

### 3. Register with Agent

```c
luup_agent_register_tool(agent, &weather_tool, weather_callback, NULL);
```

### 4. Agent Will Call Automatically

```c
char* response = luup_agent_generate(agent, "What's the weather in Seattle?");
// Agent will:
// 1. Recognize it needs weather info
// 2. Call your tool with city="Seattle"
// 3. Incorporate the result into its response
printf("%s\n", response);
// Output: "The weather in Seattle is currently sunny with a temperature of 72°F..."
```

## JSON Schema for Parameters

Tool parameters follow JSON Schema specification:

### Simple Types

```json
{
  "type": "object",
  "properties": {
    "count": {
      "type": "integer",
      "description": "Number of items"
    },
    "enabled": {
      "type": "boolean"
    },
    "ratio": {
      "type": "number"
    },
    "name": {
      "type": "string"
    }
  },
  "required": ["count"]
}
```

### Enums (Choice)

```json
{
  "type": "object",
  "properties": {
    "size": {
      "type": "string",
      "enum": ["small", "medium", "large"],
      "description": "Item size"
    }
  }
}
```

### Arrays

```json
{
  "type": "object",
  "properties": {
    "tags": {
      "type": "array",
      "items": {
        "type": "string"
      },
      "description": "List of tags"
    }
  }
}
```

### Nested Objects

```json
{
  "type": "object",
  "properties": {
    "location": {
      "type": "object",
      "properties": {
        "city": {"type": "string"},
        "country": {"type": "string"}
      },
      "required": ["city"]
    }
  }
}
```

## Complete Example: Calculator Tool

```c
#include <luup_agent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Simple expression evaluator (in production, use a proper parser)
double evaluate_expression(const char* expr) {
    // This is a simplified example
    // In production, use a proper expression parser
    double result = 0.0;
    sscanf(expr, "%lf", &result);
    return result;
}

char* calculate_callback(const char* params_json, void* user_data) {
    // In production, use a proper JSON library like nlohmann/json
    // For this example, we'll extract the expression with simple parsing
    
    // Find "expression": "..." in JSON
    const char* expr_start = strstr(params_json, "\"expression\":");
    if (!expr_start) {
        return strdup("{\"error\": \"Missing expression parameter\"}");
    }
    
    // Skip to the value
    expr_start = strchr(expr_start, ':');
    expr_start = strchr(expr_start, '"');
    if (!expr_start) {
        return strdup("{\"error\": \"Invalid expression format\"}");
    }
    expr_start++; // Skip opening quote
    
    const char* expr_end = strchr(expr_start, '"');
    if (!expr_end) {
        return strdup("{\"error\": \"Invalid expression format\"}");
    }
    
    // Extract expression
    size_t expr_len = expr_end - expr_start;
    char* expression = (char*)malloc(expr_len + 1);
    strncpy(expression, expr_start, expr_len);
    expression[expr_len] = '\0';
    
    // Evaluate
    double result = evaluate_expression(expression);
    free(expression);
    
    // Return JSON result
    char* result_json = (char*)malloc(256);
    snprintf(result_json, 256, "{\"result\": %.2f}", result);
    return result_json;
}

int main() {
    // Create model and agent...
    
    // Define calculator tool
    luup_tool calc_tool = {
        .name = "calculate",
        .description = "Evaluate a mathematical expression",
        .parameters_json =
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"expression\": {"
            "      \"type\": \"string\","
            "      \"description\": \"Mathematical expression to evaluate\""
            "    }"
            "  },"
            "  \"required\": [\"expression\"]"
            "}"
    };
    
    // Register tool
    luup_agent_register_tool(agent, &calc_tool, calculate_callback, NULL);
    
    // Test it
    char* response = luup_agent_generate(agent, "What is 15 * 28?");
    printf("%s\n", response);
    luup_free_string(response);
    
    return 0;
}
```

## User Data Pattern

Pass custom data to callbacks:

```c
typedef struct {
    sqlite3* db;
    const char* api_key;
    void* cache;
} ToolContext;

char* database_tool_callback(const char* params, void* user_data) {
    ToolContext* ctx = (ToolContext*)user_data;
    
    // Use ctx->db for database queries
    // Use ctx->api_key for API calls
    // Use ctx->cache for caching
    
    return strdup("{\"result\": \"...\"}");
}

int main() {
    ToolContext ctx = {
        .db = my_database,
        .api_key = "...",
        .cache = my_cache
    };
    
    luup_agent_register_tool(agent, &tool, database_tool_callback, &ctx);
}
```

## Error Handling in Tools

Return error information in JSON:

```c
char* tool_callback(const char* params, void* user_data) {
    // Validate parameters
    if (!validate_params(params)) {
        return strdup("{\"error\": \"Invalid parameters\", \"code\": 400}");
    }
    
    // Try operation
    int result = perform_operation();
    if (result < 0) {
        return strdup("{\"error\": \"Operation failed\", \"code\": 500}");
    }
    
    // Success
    char* json = malloc(256);
    snprintf(json, 256, "{\"success\": true, \"data\": %d}", result);
    return json;
}
```

The agent will see the error and can respond appropriately to the user.

## Best Practices

1. **Clear Descriptions**: Help the agent understand when to use your tool
2. **Validate Parameters**: Check inputs before processing
3. **Return Valid JSON**: Always return well-formed JSON
4. **Handle Errors Gracefully**: Return error info in JSON format
5. **Use strdup/malloc**: Library expects heap-allocated return values
6. **Thread Safety**: Ensure callbacks are thread-safe if using multiple threads
7. **Performance**: Keep tool execution fast to avoid blocking generation

## Built-in Tools

luup-agent provides some useful built-in tools:

### Todo List

```c
luup_agent_enable_builtin_todo(agent, "todos.json");
```

Gives agent ability to manage todo items.

### Notes

```c
luup_agent_enable_builtin_notes(agent, "notes.json");
```

Gives agent ability to store and retrieve notes.

### Auto-Summarization

```c
luup_agent_enable_builtin_summarization(agent);
```

Automatically summarizes conversation when context fills.

## Advanced: Async Tools

For long-running operations, consider using async patterns:

```c
// Store pending operations
typedef struct {
    int id;
    bool completed;
    char* result;
} AsyncOperation;

char* start_async_operation(const char* params, void* user_data) {
    // Start operation in background
    int op_id = start_background_task(params);
    
    // Return operation ID
    char* json = malloc(128);
    snprintf(json, 128, "{\"operation_id\": %d, \"status\": \"pending\"}", op_id);
    return json;
}

char* check_operation_status(const char* params, void* user_data) {
    // Extract operation ID from params
    // Check status
    // Return current status or result
}
```

## Next Steps

- Review [API Reference](api_reference.md) for complete details
- Check [examples/tool_calling.cpp](../examples/tool_calling.cpp)
- Experiment with different tool combinations

