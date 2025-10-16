/**
 * @file builtin_tools_demo.cpp
 * @brief Demonstration of built-in tools with opt-out design
 * 
 * This example shows:
 * - Default behavior: built-in tools enabled
 * - Opt-out design: creating lightweight agent without built-in tools
 * - Manual tool registration with persistent storage
 * - Using todo, notes, and summarization tools
 */

#include <luup_agent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_separator(const char* title) {
    printf("\n========================================\n");
    printf("%s\n", title);
    printf("========================================\n\n");
}

int main(int argc, char** argv) {
    printf("luup-agent Built-in Tools Demo\n");
    printf("Version: %s\n\n", luup_version());
    
    if (argc < 2) {
        printf("Usage: %s <path-to-model.gguf>\n", argv[0]);
        printf("\nThis demo shows the opt-out design for built-in tools.\n");
        return 1;
    }
    
    const char* model_path = argv[1];
    
    // Create model
    luup_model_config model_config = {
        .path = model_path,
        .gpu_layers = -1,
        .context_size = 2048,
        .threads = 0,
        .api_key = nullptr,
        .api_base_url = nullptr
    };
    
    printf("Loading model...\n");
    luup_model* model = luup_model_create_local(&model_config);
    if (!model) {
        fprintf(stderr, "Error: %s\n", luup_get_last_error());
        return 1;
    }
    
    // Get model info
    luup_model_info info;
    if (luup_model_get_info(model, &info) == LUUP_SUCCESS) {
        printf("Model loaded:\n");
        printf("  Backend: %s\n", info.backend);
        printf("  Device: %s\n", info.device);
        printf("  GPU Layers: %d\n", info.gpu_layers_loaded);
        printf("  Context Size: %d\n", info.context_size);
    }
    
    // ========================================
    // Example 1: Default behavior (tools enabled)
    // ========================================
    print_separator("Example 1: Default Agent (Built-in Tools Enabled)");
    
    luup_agent_config default_config = {
        .model = model,
        .system_prompt = 
            "You are a helpful assistant with built-in productivity tools. "
            "You have access to a todo list, notes system, and auto-summarization. "
            "Help the user manage their tasks and information effectively.",
        .temperature = 0.7f,
        .max_tokens = 512,
        .enable_tool_calling = true,
        .enable_history_management = true,
        .enable_builtin_tools = true  // Default: tools enabled
    };
    
    printf("Creating agent with built-in tools enabled...\n");
    luup_agent* agent_with_tools = luup_agent_create(&default_config);
    if (!agent_with_tools) {
        fprintf(stderr, "Error: %s\n", luup_get_last_error());
        luup_model_destroy(model);
        return 1;
    }
    
    printf("Agent created successfully with all built-in tools!\n");
    printf("\nTry asking:\n");
    printf("  - \"Add a todo to finish the project report\"\n");
    printf("  - \"Create a note about the meeting with tags: work, important\"\n");
    printf("  - \"Show me my todos\"\n");
    
    // Example query
    const char* query1 = "Add a todo to finish the project report by Friday";
    printf("\nUser: %s\n", query1);
    printf("Assistant: ");
    fflush(stdout);
    
    char* response1 = luup_agent_generate(agent_with_tools, query1);
    if (response1) {
        printf("%s\n", response1);
        luup_free_string(response1);
    } else {
        fprintf(stderr, "Error: %s\n", luup_get_last_error());
    }
    
    // ========================================
    // Example 2: Lightweight agent (opt-out)
    // ========================================
    print_separator("Example 2: Lightweight Agent (No Built-in Tools)");
    
    luup_agent_config light_config = {
        .model = model,
        .system_prompt = 
            "You are a simple assistant focused on answering questions directly "
            "without additional tools.",
        .temperature = 0.7f,
        .max_tokens = 512,
        .enable_tool_calling = false,  // Disable tool calling entirely
        .enable_history_management = true,
        .enable_builtin_tools = false  // Opt-out: no built-in tools
    };
    
    printf("Creating lightweight agent without built-in tools...\n");
    luup_agent* agent_lightweight = luup_agent_create(&light_config);
    if (!agent_lightweight) {
        fprintf(stderr, "Error: %s\n", luup_get_last_error());
        luup_agent_destroy(agent_with_tools);
        luup_model_destroy(model);
        return 1;
    }
    
    printf("Lightweight agent created successfully!\n");
    printf("This agent has no tools and is more resource-efficient.\n");
    
    const char* query2 = "What is 2 + 2?";
    printf("\nUser: %s\n", query2);
    printf("Assistant: ");
    fflush(stdout);
    
    char* response2 = luup_agent_generate(agent_lightweight, query2);
    if (response2) {
        printf("%s\n", response2);
        luup_free_string(response2);
    } else {
        fprintf(stderr, "Error: %s\n", luup_get_last_error());
    }
    
    // ========================================
    // Example 3: Manual tool registration with storage
    // ========================================
    print_separator("Example 3: Manual Tool Registration with Persistent Storage");
    
    luup_agent_config manual_config = {
        .model = model,
        .system_prompt = 
            "You are a task management assistant with persistent storage. "
            "Help users manage their todos and notes, which are saved to disk.",
        .temperature = 0.7f,
        .max_tokens = 512,
        .enable_tool_calling = true,
        .enable_history_management = true,
        .enable_builtin_tools = false  // Start without tools
    };
    
    printf("Creating agent without default tools...\n");
    luup_agent* agent_manual = luup_agent_create(&manual_config);
    if (!agent_manual) {
        fprintf(stderr, "Error: %s\n", luup_get_last_error());
        luup_agent_destroy(agent_with_tools);
        luup_agent_destroy(agent_lightweight);
        luup_model_destroy(model);
        return 1;
    }
    
    printf("Manually registering tools with persistent storage...\n");
    
    // Register todo tool with file storage
    if (luup_agent_enable_builtin_todo(agent_manual, "demo_todos.json") != LUUP_SUCCESS) {
        fprintf(stderr, "Failed to enable todo tool: %s\n", luup_get_last_error());
    } else {
        printf("  ✓ Todo list enabled (storage: demo_todos.json)\n");
    }
    
    // Register notes tool with file storage
    if (luup_agent_enable_builtin_notes(agent_manual, "demo_notes.json") != LUUP_SUCCESS) {
        fprintf(stderr, "Failed to enable notes tool: %s\n", luup_get_last_error());
    } else {
        printf("  ✓ Notes enabled (storage: demo_notes.json)\n");
    }
    
    // Register summarization
    if (luup_agent_enable_builtin_summarization(agent_manual) != LUUP_SUCCESS) {
        fprintf(stderr, "Failed to enable summarization: %s\n", luup_get_last_error());
    } else {
        printf("  ✓ Auto-summarization enabled\n");
    }
    
    printf("\nAgent now has persistent storage for todos and notes!\n");
    printf("Data will be saved to demo_todos.json and demo_notes.json\n");
    
    const char* query3 = "List all my current todos";
    printf("\nUser: %s\n", query3);
    printf("Assistant: ");
    fflush(stdout);
    
    char* response3 = luup_agent_generate(agent_manual, query3);
    if (response3) {
        printf("%s\n", response3);
        luup_free_string(response3);
    } else {
        fprintf(stderr, "Error: %s\n", luup_get_last_error());
    }
    
    // ========================================
    // Cleanup
    // ========================================
    print_separator("Cleanup");
    
    printf("Destroying agents...\n");
    luup_agent_destroy(agent_with_tools);
    luup_agent_destroy(agent_lightweight);
    luup_agent_destroy(agent_manual);
    
    printf("Destroying model...\n");
    luup_model_destroy(model);
    
    printf("\nDemo complete!\n");
    printf("\nKey Takeaways:\n");
    printf("  1. Built-in tools are enabled by default (opt-out design)\n");
    printf("  2. Set enable_builtin_tools=false for lightweight agents\n");
    printf("  3. Manual registration allows persistent storage configuration\n");
    printf("  4. Three tools available: todo, notes, and auto-summarization\n");
    
    return 0;
}

