/**
 * @file tool_calling.cpp
 * @brief Tool calling example using luup-agent
 * 
 * This example demonstrates:
 * - Registering custom tools
 * - Tool callback implementation
 * - Automatic tool invocation by the agent
 * 
 * Note: This is a stub that will work once Phase 2 is complete
 */

#include <luup_agent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Example tool: Get current weather
char* get_weather_callback(const char* params_json, void* user_data) {
    printf("  [Tool called: get_weather with params: %s]\n", params_json);
    
    // In a real implementation, you would:
    // 1. Parse params_json to extract the city
    // 2. Call a weather API
    // 3. Return formatted JSON result
    
    // For this example, return mock data
    const char* result = "{\"temperature\": 72, \"condition\": \"sunny\", \"humidity\": 45}";
    return strdup(result);
}

// Example tool: Calculate
char* calculate_callback(const char* params_json, void* user_data) {
    printf("  [Tool called: calculate with params: %s]\n", params_json);
    
    // Mock calculation result
    const char* result = "{\"result\": 42}";
    return strdup(result);
}

int main(int argc, char** argv) {
    printf("luup-agent Tool Calling Example\n");
    printf("Version: %s\n\n", luup_version());
    
    if (argc < 2) {
        printf("Usage: %s <path-to-model.gguf>\n", argv[0]);
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
    
    printf("Creating model...\n");
    luup_model* model = luup_model_create_local(&model_config);
    if (!model) {
        fprintf(stderr, "Error: %s\n", luup_get_last_error());
        return 1;
    }
    
    // Create agent with tool calling enabled
    luup_agent_config agent_config = {
        .model = model,
        .system_prompt = "You are a helpful assistant with access to tools. "
                        "Use the get_weather tool to check weather and "
                        "the calculate tool for math operations.",
        .temperature = 0.7f,
        .max_tokens = 512,
        .enable_tool_calling = true,  // Enable tool calling
        .enable_history_management = true
    };
    
    printf("Creating agent...\n");
    luup_agent* agent = luup_agent_create(&agent_config);
    if (!agent) {
        fprintf(stderr, "Error: %s\n", luup_get_last_error());
        luup_model_destroy(model);
        return 1;
    }
    
    // Register weather tool
    luup_tool weather_tool = {
        .name = "get_weather",
        .description = "Get current weather for a city",
        .parameters_json = 
            "{"
            "  \"type\": \"object\","
            "  \"properties\": {"
            "    \"city\": {"
            "      \"type\": \"string\","
            "      \"description\": \"The city name\""
            "    },"
            "    \"units\": {"
            "      \"type\": \"string\","
            "      \"enum\": [\"celsius\", \"fahrenheit\"],"
            "      \"description\": \"Temperature units\""
            "    }"
            "  },"
            "  \"required\": [\"city\"]"
            "}"
    };
    
    printf("Registering weather tool...\n");
    if (luup_agent_register_tool(agent, &weather_tool, get_weather_callback, nullptr) != LUUP_SUCCESS) {
        fprintf(stderr, "Error registering tool: %s\n", luup_get_last_error());
    }
    
    // Register calculator tool
    luup_tool calc_tool = {
        .name = "calculate",
        .description = "Perform mathematical calculations",
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
    
    printf("Registering calculator tool...\n");
    if (luup_agent_register_tool(agent, &calc_tool, calculate_callback, nullptr) != LUUP_SUCCESS) {
        fprintf(stderr, "Error registering tool: %s\n", luup_get_last_error());
    }
    
    printf("\nTools registered successfully!\n\n");
    printf("==========================================\n");
    printf("Try these example queries:\n");
    printf("- What's the weather in Seattle?\n");
    printf("- Calculate 15 * 28\n");
    printf("- What's the weather in Tokyo and is it warmer than London?\n");
    printf("==========================================\n\n");
    
    // Test queries
    const char* test_queries[] = {
        "What's the weather in Seattle?",
        "Calculate 42 * 13",
        nullptr
    };
    
    for (int i = 0; test_queries[i] != nullptr; i++) {
        printf("You: %s\n", test_queries[i]);
        printf("Assistant: ");
        fflush(stdout);
        
        char* response = luup_agent_generate(agent, test_queries[i]);
        if (response) {
            printf("%s\n\n", response);
            luup_free_string(response);
        } else {
            fprintf(stderr, "Error: %s\n\n", luup_get_last_error());
        }
    }
    
    // Cleanup
    luup_agent_destroy(agent);
    luup_model_destroy(model);
    
    printf("Example complete!\n");
    return 0;
}

