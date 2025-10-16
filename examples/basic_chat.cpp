/**
 * @file basic_chat.cpp
 * @brief Basic chat example using luup-agent
 * 
 * This example demonstrates:
 * - Creating a local model
 * - Creating an agent with a system prompt
 * - Generating responses
 * 
 * Note: This is a stub that will work once Phase 1 is complete
 */

#include <luup_agent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    printf("luup-agent Basic Chat Example\n");
    printf("Version: %s\n\n", luup_version());
    
    // Check for model path argument
    if (argc < 2) {
        printf("Usage: %s <path-to-model.gguf>\n", argv[0]);
        printf("Example: %s models/qwen-0.5b.gguf\n", argv[0]);
        return 1;
    }
    
    const char* model_path = argv[1];
    
    // Create model configuration
    luup_model_config model_config = {
        .path = model_path,
        .gpu_layers = -1,  // Auto-detect and use all available GPU
        .context_size = 2048,
        .threads = 0,  // Auto-detect CPU threads
        .api_key = nullptr,
        .api_base_url = nullptr
    };
    
    printf("Creating model from: %s\n", model_path);
    luup_model* model = luup_model_create_local(&model_config);
    if (!model) {
        fprintf(stderr, "Error creating model: %s\n", luup_get_last_error());
        return 1;
    }
    
    printf("Model created successfully\n");
    
    // Get model info
    luup_model_info info;
    if (luup_model_get_info(model, &info) == LUUP_SUCCESS) {
        printf("Backend: %s\n", info.backend);
        printf("Device: %s\n", info.device);
        printf("Context size: %d\n", info.context_size);
        printf("GPU layers loaded: %d\n", info.gpu_layers_loaded);
    }
    
    // Warmup model
    printf("\nWarming up model...\n");
    if (luup_model_warmup(model) != LUUP_SUCCESS) {
        fprintf(stderr, "Warning: Model warmup failed: %s\n", luup_get_last_error());
    }
    
    // Create agent
    luup_agent_config agent_config = {
        .model = model,
        .system_prompt = "You are a helpful AI assistant. Be concise and friendly.",
        .temperature = 0.7f,
        .max_tokens = 512,
        .enable_tool_calling = false,  // No tools for basic chat
        .enable_history_management = true
    };
    
    printf("Creating agent...\n");
    luup_agent* agent = luup_agent_create(&agent_config);
    if (!agent) {
        fprintf(stderr, "Error creating agent: %s\n", luup_get_last_error());
        luup_model_destroy(model);
        return 1;
    }
    
    printf("Agent created successfully\n\n");
    printf("Chat started! Type 'quit' to exit.\n");
    printf("==========================================\n\n");
    
    // Simple chat loop
    char user_input[1024];
    while (true) {
        printf("You: ");
        fflush(stdout);
        
        if (!fgets(user_input, sizeof(user_input), stdin)) {
            break;
        }
        
        // Remove trailing newline
        size_t len = strlen(user_input);
        if (len > 0 && user_input[len-1] == '\n') {
            user_input[len-1] = '\0';
        }
        
        // Check for quit
        if (strcmp(user_input, "quit") == 0 || strcmp(user_input, "exit") == 0) {
            break;
        }
        
        // Skip empty input
        if (strlen(user_input) == 0) {
            continue;
        }
        
        // Generate response
        printf("Assistant: ");
        fflush(stdout);
        
        char* response = luup_agent_generate(agent, user_input);
        if (response) {
            printf("%s\n\n", response);
            luup_free_string(response);
        } else {
            fprintf(stderr, "Error generating response: %s\n\n", luup_get_last_error());
        }
    }
    
    printf("\nGoodbye!\n");
    
    // Cleanup
    luup_agent_destroy(agent);
    luup_model_destroy(model);
    
    return 0;
}

