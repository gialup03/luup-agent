/**
 * @file remote_api_demo.cpp
 * @brief Remote API Backend Demo
 * 
 * Demonstrates using luup-agent with OpenAI-compatible remote APIs:
 * - Creating remote models with API endpoints
 * - Basic text generation
 * - Streaming responses
 * - Custom endpoints (Ollama, OpenRouter, etc.)
 * 
 * Usage:
 *   export OPENAI_API_KEY="sk-..."
 *   ./remote_api_demo
 * 
 * Or with custom endpoint:
 *   export API_ENDPOINT="https://api.openrouter.ai/api/v1"
 *   export API_KEY="sk-..."
 *   ./remote_api_demo
 */

#include <luup_agent.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// Streaming callback
void stream_callback(const char* token, void* user_data) {
    printf("%s", token);
    fflush(stdout);
}

// Error callback for diagnostics
void error_callback(luup_error_t code, const char* msg, void* user_data) {
    fprintf(stderr, "Error [%d]: %s\n", code, msg);
}

int main() {
    printf("=== luup-agent Remote API Demo ===\n\n");
    
    // Set up error handling
    luup_set_error_callback(error_callback, nullptr);
    
    // Get API key from environment
    const char* api_key = std::getenv("OPENAI_API_KEY");
    if (!api_key) {
        api_key = std::getenv("API_KEY");
    }
    
    if (!api_key) {
        fprintf(stderr, "Error: Please set OPENAI_API_KEY or API_KEY environment variable\n");
        fprintf(stderr, "Example: export OPENAI_API_KEY=\"sk-...\"\n");
        return 1;
    }
    
    // Get custom endpoint from environment (optional)
    const char* api_endpoint = std::getenv("API_ENDPOINT");
    if (!api_endpoint) {
        api_endpoint = "https://api.openai.com/v1";
    }
    
    printf("API Endpoint: %s\n", api_endpoint);
    printf("API Key: %s...\n\n", std::string(api_key).substr(0, 10).c_str());
    
    // =========================================================================
    // Example 1: Create Remote Model
    // =========================================================================
    printf("--- Example 1: Creating Remote Model ---\n");
    
    luup_model_config config = {0};
    config.path = "gpt-3.5-turbo";  // Model name for remote APIs
    config.api_key = api_key;
    config.api_base_url = api_endpoint;
    config.context_size = 4096;
    config.gpu_layers = 0;  // Ignored for remote models
    config.threads = 0;     // Ignored for remote models
    
    luup_model* model = luup_model_create_remote(&config);
    if (!model) {
        fprintf(stderr, "Failed to create remote model: %s\n", luup_get_last_error());
        return 1;
    }
    
    printf("✓ Remote model created successfully\n\n");
    
    // Get model info
    luup_model_info info;
    if (luup_model_get_info(model, &info) == LUUP_SUCCESS) {
        printf("Model Information:\n");
        printf("  Backend:       %s\n", info.backend);
        printf("  Device:        %s\n", info.device);
        printf("  Context Size:  %d tokens\n", info.context_size);
        printf("\n");
    }
    
    // =========================================================================
    // Example 2: Basic Chat Agent
    // =========================================================================
    printf("--- Example 2: Basic Chat Agent ---\n");
    
    luup_agent_config agent_config = {0};
    agent_config.model = model;
    agent_config.system_prompt = "You are a helpful AI assistant.";
    agent_config.temperature = 0.7f;
    agent_config.max_tokens = 150;
    agent_config.enable_tool_calling = false;
    agent_config.enable_history_management = true;
    agent_config.enable_builtin_tools = false;  // Disable built-in tools for remote demo
    
    luup_agent* agent = luup_agent_create(&agent_config);
    if (!agent) {
        fprintf(stderr, "Failed to create agent: %s\n", luup_get_last_error());
        luup_model_destroy(model);
        return 1;
    }
    
    printf("✓ Agent created successfully\n\n");
    
    // Generate response
    printf("User: Tell me a short joke about programming.\n");
    printf("Assistant: ");
    fflush(stdout);
    
    char* response = luup_agent_generate(agent, "Tell me a short joke about programming.");
    if (response) {
        printf("%s\n\n", response);
        luup_free_string(response);
    } else {
        fprintf(stderr, "Generation failed: %s\n", luup_get_last_error());
    }
    
    // =========================================================================
    // Example 3: Streaming Generation
    // =========================================================================
    printf("--- Example 3: Streaming Generation ---\n");
    
    printf("User: Write a haiku about artificial intelligence.\n");
    printf("Assistant: ");
    fflush(stdout);
    
    luup_error_t result = luup_agent_generate_stream(
        agent,
        "Write a haiku about artificial intelligence.",
        stream_callback,
        nullptr
    );
    
    if (result == LUUP_SUCCESS) {
        printf("\n\n✓ Streaming completed successfully\n\n");
    } else {
        fprintf(stderr, "\nStreaming failed: %s\n", luup_get_last_error());
    }
    
    // =========================================================================
    // Example 4: Multi-turn Conversation
    // =========================================================================
    printf("--- Example 4: Multi-turn Conversation ---\n");
    
    // First turn
    printf("User: What is the capital of France?\n");
    printf("Assistant: ");
    fflush(stdout);
    
    response = luup_agent_generate(agent, "What is the capital of France?");
    if (response) {
        printf("%s\n\n", response);
        luup_free_string(response);
    }
    
    // Second turn (context is maintained)
    printf("User: What is its population?\n");
    printf("Assistant: ");
    fflush(stdout);
    
    response = luup_agent_generate(agent, "What is its population?");
    if (response) {
        printf("%s\n\n", response);
        luup_free_string(response);
    }
    
    // =========================================================================
    // Example 5: Custom Endpoint (Ollama Local Server)
    // =========================================================================
    printf("--- Example 5: Custom Endpoint Support ---\n");
    printf("Note: For Ollama or other custom endpoints, set:\n");
    printf("  export API_ENDPOINT=\"http://localhost:11434/v1\"\n");
    printf("  export API_KEY=\"ollama\"  # Any value works for local Ollama\n");
    printf("  ./remote_api_demo\n\n");
    
    // =========================================================================
    // Cleanup
    // =========================================================================
    printf("--- Cleanup ---\n");
    
    luup_agent_destroy(agent);
    printf("✓ Agent destroyed\n");
    
    luup_model_destroy(model);
    printf("✓ Model destroyed\n");
    
    printf("\n=== Demo Complete ===\n");
    return 0;
}

